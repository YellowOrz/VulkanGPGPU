#include <iostream>
#include <fstream>
#include <vulkan/vulkan.hpp>

using namespace std;

int main() {
  //! 准备instance
  vk::ApplicationInfo app_info;
  app_info.setPApplicationName("VulkanCompute");
  app_info.setApplicationVersion(1);
  app_info.setApiVersion(VK_API_VERSION_1_1);

  const vector<const char *> layer_names = {"VK_LAYER_KHRONOS_validation"};
  vk::InstanceCreateInfo instance_info;
  instance_info.setPApplicationInfo(&app_info)
      .setPEnabledLayerNames(layer_names)
      .setEnabledExtensionCount(0);

  vk::Instance instance = vk::createInstance(instance_info);
  //! 准备physical device
  vk::PhysicalDevice physical_device = instance.enumeratePhysicalDevices().front();
  vk::PhysicalDeviceProperties phy_device_prop = physical_device.getProperties();
  cout << "[INFO] Device name: " << phy_device_prop.deviceName << endl;
  cout << "[INFO] Vulkan version: " << phy_device_prop.apiVersion << endl;
  cout << "[INFO] Max compute shared memory size:"
       << phy_device_prop.limits.maxComputeSharedMemorySize << endl;
  //! 准备queue family prop
  vector<vk::QueueFamilyProperties> queue_family_props = physical_device.getQueueFamilyProperties();
  int compute_queue_idx = -1;
  for (int i = 0; i < queue_family_props.size(); i++) {
    if (queue_family_props[i].queueFlags & vk::QueueFlagBits::eCompute) {
      compute_queue_idx = i;
      break;
    }
  }
  if (compute_queue_idx == -1) {
    cout << "[ERROR] No compute queue family found!" << endl;
    return 1;
  } else
    cout << "[INFO] Compute queue family index: " << compute_queue_idx << endl;
  //! 准备device
  vk::DeviceQueueCreateInfo queue_create_info;
  float priority = 1;
  queue_create_info.setQueueFamilyIndex(uint32_t(compute_queue_idx)).setQueueCount(1).setQueuePriorities(priority);
  vk::DeviceCreateInfo device_create_info;
  device_create_info.setQueueCreateInfos(queue_create_info);
  vk::Device device = physical_device.createDevice(device_create_info);
  //! 准备buffer
  const int element_num = 1024;
  const int byte_size = element_num * sizeof(float);
  vector<uint32_t> queue_indices = {uint32_t(compute_queue_idx)};
  vk::BufferCreateInfo buffer_create_info;
  buffer_create_info.setSize(byte_size)
      .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
      .setSharingMode(vk::SharingMode::eExclusive)
      .setQueueFamilyIndices(queue_indices);
  vk::Buffer buffer_in1 = device.createBuffer(buffer_create_info);
  vk::Buffer buffer_in2 = device.createBuffer(buffer_create_info);
  vk::Buffer buffer_out = device.createBuffer(buffer_create_info);
  //! 找到合适内存类型
  vk::PhysicalDeviceMemoryProperties phy_mem_req = physical_device.getMemoryProperties();
  int mem_type_idx = -1;
  vk::DeviceSize mem_heap_size = 0;
  for (int i = 0; i < phy_mem_req.memoryTypeCount; i++) {
    vk::MemoryType mem_type = phy_mem_req.memoryTypes[i];
    if ((mem_type.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) &&
        (mem_type.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent)) {
      mem_type_idx = i;
      mem_heap_size = phy_mem_req.memoryHeaps[mem_type.heapIndex].size;
      break;
    }
  }
  if (mem_type_idx == -1) {
    cout << "[ERROR] No suitable memory type found!" << endl;
    return 1;
  } else {
    cout << "[INFO] Memory type index: " << mem_type_idx << endl;
    cout << "[INFO] Memory heap size: " << mem_heap_size << endl;
  }
  //! 分配内存
  vk::MemoryRequirements mem_in1_req = device.getBufferMemoryRequirements(buffer_in1);
  vk::MemoryRequirements mem_in2_req = device.getBufferMemoryRequirements(buffer_in2);
  vk::MemoryRequirements mem_out_req = device.getBufferMemoryRequirements(buffer_out);
  vk::MemoryAllocateInfo mem_in1_alloc_info(mem_in1_req.size, mem_type_idx);
  vk::MemoryAllocateInfo mem_in2_alloc_info(mem_in2_req.size, mem_type_idx);
  vk::MemoryAllocateInfo mem_out_alloc_info(mem_out_req.size, mem_type_idx);
  vk::DeviceMemory mem_in1 = device.allocateMemory(mem_in1_alloc_info);
  vk::DeviceMemory mem_in2 = device.allocateMemory(mem_in2_alloc_info);
  vk::DeviceMemory mem_out = device.allocateMemory(mem_out_alloc_info);
  //! 初始化内存
  float *array1 = (float *)malloc(byte_size);
  float *array2 = (float *)malloc(byte_size);
  for (int i = 0; i < element_num; i++) {
    array1[i] = i;
    array2[i] = i * 2;
  }
  void *mem_in1_ptr = device.mapMemory(mem_in1, 0, mem_in1_req.size);
  memcpy(mem_in1_ptr, array1, byte_size);
  device.unmapMemory(mem_in1);
  void *mem_in2_ptr = device.mapMemory(mem_in2, 0, mem_in2_req.size);
  memcpy(mem_in2_ptr, array2, byte_size);
  device.unmapMemory(mem_in2);
  device.bindBufferMemory(buffer_in1, mem_in1, 0);
  device.bindBufferMemory(buffer_in2, mem_in2, 0);
  device.bindBufferMemory(buffer_out, mem_out, 0);
  //! 创建shader module
  string spv_filename = "sum_two_array.spv";
  ifstream file(spv_filename, ios::binary | ios::ate);
  if (!file.is_open()) {
    cout << "[ERROR] Failed to open shader file!" << endl;
    return 1;
  }
  auto code_size = file.tellg();
  vector<char> shader_content(code_size);
  file.seekg(0);
  file.read(shader_content.data(), code_size);
  vk::ShaderModuleCreateInfo shader_module_create_info;
  shader_module_create_info.setCodeSize(code_size).setPCode(reinterpret_cast<uint32_t*>(shader_content.data()));
  vk::ShaderModule shader_module = device.createShaderModule(shader_module_create_info);
  //! 创建descriptor set layout
  vector<vk::DescriptorSetLayoutBinding> des_set_layout_bindings = {
    {0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
    {1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
    {2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute} // TODO: 数组长度，uniform变量
  };
  vk::DescriptorSetLayoutCreateInfo des_set_layout_create_info;
  des_set_layout_create_info.setBindings(des_set_layout_bindings);
  vk::DescriptorSetLayout des_set_layout = device.createDescriptorSetLayout(des_set_layout_create_info);
  std::vector<vk::DescriptorSetLayout> des_set_layout_array = {des_set_layout};
  //! 创建pipeline layout
  vk::PipelineLayoutCreateInfo pipe_layout_create_info;
  pipe_layout_create_info.setSetLayouts(des_set_layout_array);
  vk::PipelineLayout pipe_layout = device.createPipelineLayout(pipe_layout_create_info);
  //! 创建pipeline
  vk::PipelineShaderStageCreateInfo shader_stage_create_info;
  shader_stage_create_info.setStage(vk::ShaderStageFlagBits::eCompute).setModule(shader_module).setPName("main");
  vk::ComputePipelineCreateInfo pipe_create_info;
  pipe_create_info.setLayout(pipe_layout).setStage(shader_stage_create_info);
  vk::PipelineCache pipe_cache = device.createPipelineCache({});  // NOTE: 可选
  auto result = device.createComputePipeline(pipe_cache, pipe_create_info);
  if(result.result != vk::Result::eSuccess) {
    cout << "[ERROR] Failed to create pipeline!" << endl;
    return 1;
  }
  vk::Pipeline pipeline = result.value;
  //! 创建descriptor pool
  vk::DescriptorPoolSize des_pool_size;
  des_pool_size.setType(vk::DescriptorType::eStorageBuffer).setDescriptorCount(3);
  vk::DescriptorPoolCreateInfo des_pool_create_info;
  des_pool_create_info.setMaxSets(3).setPoolSizes(des_pool_size);
  vk::DescriptorPool des_pool = device.createDescriptorPool(des_pool_create_info);
  //! 创建descriptor set
  vk::DescriptorSetAllocateInfo des_set_alloc_info;
  des_set_alloc_info.setDescriptorPool(des_pool).setSetLayouts(des_set_layout_array);
  const vector<vk::DescriptorSet> des_set_array = device.allocateDescriptorSets(des_set_alloc_info);
  vk::DescriptorSet des_set = des_set_array.front();
  //! 写descriptor set
  vk::DescriptorBufferInfo des_buff_in1, des_buff_in2, des_buff_out;
  des_buff_in1.setBuffer(buffer_in1).setOffset(0).setRange(byte_size);
  des_buff_in2.setBuffer(buffer_in2).setOffset(0).setRange(byte_size);
  des_buff_out.setBuffer(buffer_out).setOffset(0).setRange(byte_size);
  const std::vector<vk::WriteDescriptorSet> des_write_array = {
    {des_set, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &des_buff_in1},
    {des_set, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &des_buff_in2},
    {des_set, 2, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &des_buff_out},
  };
  device.updateDescriptorSets(des_write_array, {});
  //! 创建command pool
  vk::CommandPoolCreateInfo cmd_pool_create_info;
  cmd_pool_create_info.setQueueFamilyIndex(compute_queue_idx);
  vk::CommandPool cmd_pool = device.createCommandPool(cmd_pool_create_info);
  //! 创建command buffer
  vk::CommandBufferAllocateInfo cmd_buff_alloc_info;
  cmd_buff_alloc_info.setCommandPool(cmd_pool).setLevel(vk::CommandBufferLevel::ePrimary).setCommandBufferCount(1);
  const vector<vk::CommandBuffer> cmd_buff_array = device.allocateCommandBuffers(cmd_buff_alloc_info);
  auto cmd_buff = cmd_buff_array.front();
  //! 录制cmd
  vk::CommandBufferBeginInfo cmd_begin_info;
  cmd_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  cmd_buff.begin(cmd_begin_info);
  cmd_buff.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
  cmd_buff.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipe_layout, 0, {des_set}, {});
  cmd_buff.dispatch(element_num, 1, 1);
  cmd_buff.end();
  //! 提交cmd
  vk::SubmitInfo submit_info;
  submit_info.setCommandBuffers(cmd_buff_array);
  vk::Queue queue = device.getQueue(compute_queue_idx, 0);  //? 第二个参数是啥
  vk::Fence fence = device.createFence({});   // NOTE: 省略了FenceCreateInfo
  queue.submit({submit_info}, fence);
  auto run_result = device.waitForFences({fence}, true, UINT_MAX);
  if(run_result != vk::Result::eSuccess) {  // TODO: 弄成宏定义
    cout << "[ERROR] Failed to run pipeline!" << endl;
    return 1;
  }
  //! 打印结果
  void *mem_out_ptr = device.mapMemory(mem_out, 0, byte_size);
  memcpy(array1, mem_out_ptr, byte_size);
  device.unmapMemory(mem_out);
  cout << "[INFO] result = ";
  for(int i = 0; i < element_num; i++)
    cout << array1[i] << " ";
  cout << endl;
  //! 清理战场
  if(array1) delete array1;
  if(array2) delete array2;
 
  device.destroyFence(fence);   // NOTE: create的都没有析构函数，所以需要手动销毁吧？
  device.destroyCommandPool(cmd_pool);
  device.destroyDescriptorPool(des_pool);
  device.destroyPipeline(pipeline);
  device.destroyPipelineCache(pipe_cache);
  device.destroyPipelineLayout(pipe_layout);
  device.destroyDescriptorSetLayout(des_set_layout);
  device.destroyShaderModule(shader_module);
  device.freeMemory(mem_in1);
  device.freeMemory(mem_in2);
  device.freeMemory(mem_out);
  device.destroyBuffer(buffer_in1);
  device.destroyBuffer(buffer_in2);
  device.destroyBuffer(buffer_out);
  device.destroy();
  instance.destroy();

  return 0;
}