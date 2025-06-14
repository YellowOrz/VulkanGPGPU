# 将十六进制字符串转换为十进制字符串
# 参数：
#   hex_string: 十六进制字符串
#   decimal_string: 输出的十进制字符串
#   delimiter: 分隔符
function(hex_to_decimal hex_string output_string delimiter)
  string(REPLACE "${delimiter}" ";" hex_list ${hex_string})
  # 遍历每个十六进制值，转成十进制

  set(decimal_string "")
  foreach(hex_val IN LISTS hex_list)
    math(EXPR decimal_val ${hex_val})
    string(APPEND decimal_string "${decimal_val}${delimiter}")
  endforeach()
  # 去掉最后一个逗号和空格
  string(REGEX REPLACE "${delimiter}$" "" decimal_string ${decimal_string})

  set(${output_string} "${decimal_string}" PARENT_SCOPE)
endfunction()

# 将SPIR-V文件转换为vector<uint32_t>
# 参数：
#   spv_file: SPIR-V文件路径
#   output_vec: 输出的vector<uint32_t>
#   use_decimal: 是否使用十进制
#   is_big_endian: 是否是大端序
function(spv_to_vector spv_file output_vec use_decimal is_big_endian)
  file(READ ${spv_file} hex_string HEX)
  # 判断大小端，并添加0x和逗号
  if(is_big_endian)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])" "0x\\1\\2\\3\\4, " hex_string ${hex_string})
  else()
    string(REGEX REPLACE "([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])" "0x\\4\\3\\2\\1, " hex_string ${hex_string})
  endif()
  # 去掉最后一个逗号和空格
  string(REGEX REPLACE ", $" "" hex_string ${hex_string})

  # 从十六进制转十进制
  if(use_decimal)
    hex_to_decimal(${hex_string} decimal_string ", ")
    set(${output_vec} ${decimal_string} PARENT_SCOPE)
  else ()
    set(${output_vec} ${hex_string} PARENT_SCOPE)
  endif()
endfunction()

# 将指定路径下所有.comp文件转存到一个hpp中
# 参数：
#   shader_dir: 包含.comp文件的目录。会递归查找。要求.comp文件的文件名不重复。
#   output_hpp: 输出的hpp文件路径
#   namespace: 输出hpp文件中的命名空间
#   variable_name: 输出hpp文件中的变量
#   use_decimal: 是否使用十进制存储shader
#   is_big_endian: 是否是大端序
function(vulkan_shader_compile shader_dir output_hpp namespace variable_name use_decimal is_big_endian)
  if(is_big_endian)
    message(STATUS "Interpreting shader in big endian.")
  else()
    message(STATUS "Interpreting shader in little endian.")
  endif()

  # 查找glslangValidator可执行路径
  find_program(GLS_LANG_VALIDATOR_PATH NAMES glslangValidator)
  if(GLS_LANG_VALIDATOR_PATH STREQUAL "GLS_LANG_VALIDATOR_PATH-NOTFOUND")
      message(FATAL_ERROR "glslangValidator not found.")
      return()
  endif()

  # 保证hpp的路径存在
  get_filename_component(hpp_dir ${output_hpp} DIRECTORY)
  if(NOT EXISTS "${hpp_dir}")
    file(MAKE_DIRECTORY ${hpp_dir})
  endif()

  # 写hpp的开头
  file(WRITE ${output_hpp} "#pragma once\n")
  file(APPEND ${output_hpp} "#include <unordered_map>\n")
  file(APPEND ${output_hpp} "#include <vector>\n")
  file(APPEND ${output_hpp} "#include <string>\n")
  file(APPEND ${output_hpp} "namespace ${namespace} {\n")
  file(APPEND ${output_hpp} "std::unordered_map<std::string, std::vector<uint32_t>> ${variable_name} = {\n")

  # 获取所有着色器文件
  file(GLOB_RECURSE  comp_files "${shader_dir}/*.comp")
  list(LENGTH comp_files num_comp)
  message(STATUS "Number of .comp files: ${num_comp}")

  # 遍历每个comp
  set(first_file TRUE)
  foreach(comp_file IN LISTS comp_files)
    get_filename_component(comp_name ${comp_file} NAME_WE)
    # .comp => .spv
    set(spv_file "${comp_file}.spv")
    execute_process(  # NOTE: 下面COMMAND后面的内容不能放到一个大双引号里
            COMMAND ${GLS_LANG_VALIDATOR_PATH} -V ${comp_file} -o ${spv_file}
            RESULT_VARIABLE result
    )
    if(NOT ${result} EQUAL 0)
      message(FATAL_ERROR "Failed to compile compute shader ${comp_file} to SPIR-V ${spv_file}")
    endif ()
    # .spv转vector<uint32_t>
    spv_to_vector(${spv_file} spv_vector ${use_decimal} ${is_big_endian})
    # 写入头文件
    if(NOT first_file)
      file(APPEND ${output_hpp} ",\n")
    endif()
    set(first_file FALSE)
    file(APPEND ${output_hpp} "  {\"${comp_name}\", {${spv_vector}}}")
  endforeach()

  # 写hpp的结尾
  file(APPEND ${output_hpp} "\n};\n") # unordered_map的结尾
  file(APPEND ${output_hpp} "}\n")  # namespace的结尾
endfunction()

# 检查输入参数是否为空
if(${INPUT_SHADER_DIR} STREQUAL "")
    message(FATAL_ERROR "No input file dir provided via 'INPUT_SHADER_DIR'.")
endif()

if(${OUTPUT_HEADER_FILE} STREQUAL "")
    message(FATAL_ERROR "No output file path provided via 'OUTPUT_HEADER_FILE'.")
endif()

 if(${HEADER_NAMESPACE} STREQUAL "")
     message(FATAL_ERROR "No header namespace provided via 'HEADER_NAMESPACE'.")
 endif()

if(${VARIABLE_NAME} STREQUAL "")
    message(FATAL_ERROR "No variable name provided via 'VARIABLE_NAME'.")
endif()

option(USE_DECIMAL "Use decimal representation for shader code" ON)
option(IS_BIG_ENDIAN "Use big endian byte order" OFF)

# 调用函数进行编译
vulkan_shader_compile(${INPUT_SHADER_DIR} ${OUTPUT_HEADER_FILE} ${HEADER_NAMESPACE} ${VARIABLE_NAME} ${USE_DECIMAL} ${IS_BIG_ENDIAN})