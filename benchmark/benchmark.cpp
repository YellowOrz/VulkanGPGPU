#define VMA_IMPLEMENTATION 1
#include "benchmark.h"
#include <iostream>
#include "shaders.hpp"
#include <random>

# define VK_CHECK(call) \
{ \
  auto res = vk::Result(call); \
  if (vk::Result::eSuccess != res) { \
    printf("[Error] vulkan call failed in %s:%d, result is %s\n",__FILE__,__LINE__, vk::to_string(res).c_str()); \
    return false; \
  } \
}

using namespace std;

void VkInfo::Destroy() {
#ifdef USE_VMA
  vmaDestroyAllocator(allocator);
#endif
  device.destroyCommandPool(cmd_pool);
  device.destroyDescriptorPool(desc_pool);
  device.destroy();
  instance.destroy();
}

bool Buffer::Init(const VkInfo &info, size_t elem_size, size_t num, vk::BufferUsageFlags buff_usage,
                  int alloc_usage) {
  one_elem_size = elem_size;
  num_elems = num;
  size = one_elem_size * num_elems; // TODO: 获取对齐后的size
  usage = buff_usage;

  vk::BufferCreateInfo buffer_info;
  buffer_info.setSize(size);
  buffer_info.setUsage(usage);
  buffer_info.setSharingMode(vk::SharingMode::eExclusive);  // TODO: 是这个吗
  buffer_info.setQueueFamilyIndices({info.queue_idx});
#ifdef USE_VMA
  allocator = info.allocator;
  VmaAllocationCreateInfo alloc_info;
  // alloc_info.usage = alloc_usage;
  alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
  alloc_info.requiredFlags = alloc_usage;

  VK_CHECK(vmaCreateBuffer(allocator, &static_cast<VkBufferCreateInfo>(buffer_info), &alloc_info,
    &static_cast<VkBuffer>(buffer), &allocation, nullptr));
#else
  device = info.device;
  VK_CHECK(device.createBuffer(&buffer_info, nullptr, &buffer));
  mem_req = info.device.getBufferMemoryRequirements(buffer);
  vk::MemoryAllocateInfo alloc_info;
  alloc_info.allocationSize = mem_req.size; // 不能用 size，因为有对齐的问题  // TODO: 看看mem_req.alignment是多少
  alloc_info.memoryTypeIndex = UINT32_MAX;
  auto &phy_mem_pros = info.mem_props;
  for (uint32_t i =0;i<phy_mem_pros.memoryTypeCount; i++) {
    auto flag = static_cast<VkMemoryPropertyFlags>(phy_mem_pros.memoryTypes[i].propertyFlags);  // TODO: 怎么写得更优雅
    if ((flag & alloc_usage) == alloc_usage) {  // 找到同时满足alloc_usage所有要求的mem
      alloc_info.memoryTypeIndex = i;
      break;
    }
  }
  if (alloc_info.memoryTypeIndex == UINT32_MAX) {
    printf("[FATAL] no property physical memory to crete Buffer\n");
    return false;
  }
  VK_CHECK(device.allocateMemory(&alloc_info, nullptr, &mem));
  device.bindBufferMemory(buffer, mem, 0);
#endif
  return true;
}

void Buffer::Destroy() {
#ifdef USE_VMA
  vmaDestroyBuffer(allocator, buffer, allocation);
#else
  device.freeMemory(mem);
  device.destroyBuffer(buffer);
#endif
}


template<typename T>
bool Buffer::SetData(T *data, size_t num) {
  if (num != num_elems) {
    printf("[FATAL] set data failed, because src.size() != num_elems\n");
    return false;
  }
  if (sizeof(T) != one_elem_size) {
    printf("[FATAL] set data failed, because sizeof(T) != one_elem_size\n");
    return false;
  }
  void *map_data;
#ifdef USE_VMA
  VK_CHECK(vmaMapMemory(allocator, allocation, &map_data));
  memcpy(map_data, data, size);
  VK_CHECK(vmaFlushAllocation(allocator, allocation, 0, size)); // TODO: 失败了需要unmap嘛？如果是coherent的就不要这个了吗？
  vmaUnmapMemory(allocator, allocation);
#else
  VK_CHECK(device.mapMemory(mem, 0, size, {}, &map_data));  // NOTE: 不能填写vk::MemoryMapFlagBits::ePlacedEXT
  memcpy(map_data, data, size);
  device.unmapMemory(mem);
#endif
  return true;
}

bool Buffer::SetZero() {
  void *map_data;
#ifdef USE_VMA
  VK_CHECK(vmaMapMemory(allocator, allocation, &map_data));
  memset(map_data, 0, size);
  VK_CHECK(vmaFlushAllocation(allocator, allocation, 0, size));
  vmaUnmapMemory(allocator, allocation);
#else
  VK_CHECK(device.mapMemory(mem, 0, size, {}, &map_data));
  memset(map_data, 0, size);
  device.unmapMemory(mem);
#endif
  return true;
}

template<typename T>
bool Buffer::GetData(T *data, size_t num) {
  if (num != num_elems) {
    printf("[FATAL] get data failed, because src.size() != num_elems\n");
    return false;
  }
  if (sizeof(T) != one_elem_size) {
    printf("[FATAL] get data failed, because sizeof(T) != one_elem_size\n");
    return false;
  }
  void *map_data;
#ifdef USE_VMA
  VK_CHECK(vmaMapMemory(allocator, allocation, &map_data));
  VK_CHECK(vmaInvalidateAllocation(allocator, allocation, 0, size));  // TODO: 失败了需要unmap嘛
  memcpy(data, map_data, size);
  vmaUnmapMemory(allocator, allocation);
#else
  VK_CHECK(device.mapMemory(mem, 0, size, {}, &map_data));
  memcpy(data, map_data, size);
  device.unmapMemory(mem);
#endif
  return true;
}

// NOTE: 特例化
template bool Buffer::SetData(float *data, size_t num);
template bool Buffer::SetData(int *data, size_t num);
template bool Buffer::GetData(float *data, size_t num);

vk::DescriptorType ConvertVkBufferUsage2DescriptorType(vk::BufferUsageFlags usage) {
  if (usage & vk::BufferUsageFlagBits::eStorageBuffer)
    return vk::DescriptorType::eStorageBuffer;
  if (usage & vk::BufferUsageFlagBits::eUniformBuffer)
    return vk::DescriptorType::eUniformBuffer;
  return vk::DescriptorType::eSampler;  // 默认值
}

bool DescriptorSet::Init(const VkInfo &info, const std::vector<Buffer*> &buffers) {
  device = info.device;
  //! 检查输入buffer
  for (auto &buf: buffers) {
    if (!buf->buffer) {
      printf("[ERROR] invalid buffer to init descriptor set\n");
      return false;
    }
  }
  //! 创建descriptor set layout
  auto num = buffers.size();
  vector<vk::DescriptorSetLayoutBinding> bindings(num);
  for (uint32_t i = 0; i<num;i++)
    bindings[i] = {i, ConvertVkBufferUsage2DescriptorType(buffers[i]->usage), 1, vk::ShaderStageFlagBits::eCompute};

  vk::DescriptorSetLayoutCreateInfo layout_info;
  layout_info.setBindings(bindings);
  VK_CHECK(device.createDescriptorSetLayout(&layout_info, nullptr, &layout));

  //! 创建descriptor set
  vk::DescriptorSetAllocateInfo set_info;
  set_info.setDescriptorPool(info.desc_pool);
  set_info.setPSetLayouts(&layout);
  set_info.setDescriptorSetCount(1);
  VK_CHECK(device.allocateDescriptorSets(&set_info, &set));
  //! 写descriptor set
  vector<vk::DescriptorBufferInfo> buffer_info(num);
  vector<vk::WriteDescriptorSet> write_set(num);
  for (uint32_t i = 0; i<num; i++) {
    auto buffer = buffers[i];
    buffer_info[i].setBuffer(buffer->buffer);
    buffer_info[i].setOffset(0);
    buffer_info[i].setRange(buffer->size);

    write_set[i] = {set, i, 0, 1, ConvertVkBufferUsage2DescriptorType(buffer->usage), nullptr, &buffer_info[i]};
  }
  device.updateDescriptorSets(write_set, nullptr);  // TODO: 需要descriptor copy嘛？
  return true;
}

void DescriptorSet::Destroy() {
  device.destroyDescriptorSetLayout(layout);
}


bool Pipeline::Init(const VkInfo &info, const DescriptorSet &desc, const vector<uint32_t> &shader_code) {
  device = info.device;
  vk::PipelineLayoutCreateInfo layout_info;
  layout_info.setSetLayouts({desc.layout});
  VK_CHECK(device.createPipelineLayout(&layout_info, nullptr, &layout));

  vk::ShaderModuleCreateInfo shader_info;
  shader_info.setCode(shader_code); // 等价于下面两行
  // shader_info.setCodeSize(shader_code.size() * sizeof(uint32_t));
  // shader_info.setPCode(shader_code.data());
  VK_CHECK(device.createShaderModule(&shader_info, nullptr, &shader_module));

  vk::PipelineCacheCreateInfo cache_info; // TODO: 完善
  VK_CHECK(device.createPipelineCache(&cache_info, nullptr, &cache));

  vk::PipelineShaderStageCreateInfo stage_info;
  stage_info.setStage(vk::ShaderStageFlagBits::eCompute);
  stage_info.setModule(shader_module);
  stage_info.setPName("main");

  vk::ComputePipelineCreateInfo pipeline_info;
  pipeline_info.setLayout(layout);
  pipeline_info.setStage(stage_info);
  VK_CHECK(device.createComputePipelines(cache, 1, &pipeline_info, nullptr, &pipeline));

  return true;
}

void Pipeline::Destroy() {
  device.destroyPipeline(pipeline);
  device.destroyPipelineLayout(layout);
  device.destroyPipelineCache(cache);
  device.destroyShaderModule(shader_module);
}
// bool Barrier::Init() {
//   barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
//   barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
// }

Benchmark::Benchmark(int elem_num): elem_num_(elem_num) {
  create_succrss_ = false;
  //! 初始化 Vulkan 环境
  if (!InitVkInfo()) {
    printf("[FATAL] Failed to initialize Vulkan instance.\n");
    return;
  }
  //! 创建buffer、descriptor、pipeline
  if (!CreateBuffers()) {
    printf("[FATAL] Failed to create buffers.\n");
    return;
  }
  if (!CreateDescriptors()) {
    printf("[FATAL] Failed to create descriptors.\n");
    return;
  }
  if (!CreatePipelines()) {
    printf("[FATAL] Failed to create pipelines.\n");
    return;
  }
  if (!AllocateCommandBuffer()) {
    printf("[FATAL] Failed to allocate command buffer.\n");
    return;
  }
  if (!CreateFence()) {
    printf("[FATAL] Failed to create fence.\n");
    return;
  }
  create_succrss_ = true;
}
Benchmark::~Benchmark() {
  // 清理
  if (create_succrss_) {
    vk_info_.device.destroyFence(fence_);
    buffers_.clear();
    desc_sets_.clear();
    pipelines_.clear();
    vk_info_.Destroy();
  }
}

bool Benchmark::Run() {
  //! 初始化数据
  printf("[INFO] Num of element is %d\n", elem_num_);
  float base_num = 1.f;
  vector<float> data(elem_num_, base_num);
  bool set_success = true;
  set_success &= buffers_["array1"].SetData(data.data(), elem_num_);
  set_success &= buffers_["array2"].SetData(data.data(), elem_num_);
  set_success &= buffers_["num"].SetData<int>(&elem_num_, 1);
  set_success &= buffers_["sum"].SetZero();
  if (!set_success) {
    printf("[FATAL] Failed to set data.\n");
    return false;
  }
  //! 执行GPU计算
  vk::CommandBufferBeginInfo begin_info;
  begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  cmd_buffer_.begin(begin_info);
  cmd_buffer_.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines_["sum_two_array"].pipeline);
  cmd_buffer_.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelines_["sum_two_array"].layout, 0,
    {desc_sets_["sum_two_array"].set}, {});
  cmd_buffer_.dispatch(128, 1 ,1);  // 设置grid size  // NOTE: GLSL中设置的是block size
  vk::MemoryBarrier barrier;  // NOTE: 不能用execution barrier，因为上个shader的数据可能仅在GPU缓存中
  barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
  barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
  cmd_buffer_.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
    vk::DependencyFlagBits::eByRegion, 1, &barrier, 0, nullptr, 0, nullptr);
  cmd_buffer_.bindPipeline(vk::PipelineBindPoint::eCompute, pipelines_["array_reduction"].pipeline);
  cmd_buffer_.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelines_["array_reduction"].layout, 0,
    {desc_sets_["array_reduction"].set}, {});
  cmd_buffer_.dispatch(128, 1 ,1);  // 设置grid size  // NOTE: GLSL中设置的是block size
  cmd_buffer_.end();
  vk::SubmitInfo submit_info;
  submit_info.setCommandBufferCount(1).setPCommandBuffers(&cmd_buffer_);
  VK_CHECK(vk_info_.queue.submit(1, &submit_info, fence_));
  //! 输出GPU计算结果
  VK_CHECK(vk_info_.device.waitForFences(1, &fence_, VK_TRUE, UINT64_MAX));
  float sum;
  if (!buffers_["sum"].GetData(&sum, 1)) {
    printf("[FATAL] Failed to get data from GPU\n");
    return false;
  }
  printf("[INFO] Sum of array in GPU is %f\n", sum);
  //! CPU计算结果
  sum = elem_num_ * 2 * base_num;
  printf("[INFO] Sum of array in CPU is %f\n", sum);
  return true;
}

/**
 * @brief 检查物理设备是否支持所需的所有扩展
 * @param[in] physical_device 物理设备
 * @param[in] extension_names 需要支持的扩展名称
 * @return 第一个不支持的扩展id，<0表示都支持
 */
int check_device_extension(const vk::PhysicalDevice &physical_device, const vector<const char *> &extension_names) {
  auto available_extensions = physical_device.enumerateDeviceExtensionProperties();
  for (int i = 0; i < extension_names.size(); i++) {
    auto &name = extension_names[i];
    bool has_ext = false;
    for (auto &ext: available_extensions)
      if (strcmp(ext.extensionName, name) == 0) {
        has_ext = true;
        break;
      }
    if (!has_ext) return i;
  }
  return -1;
}

bool Benchmark::InitVkInfo() {
  auto &instance = vk_info_.instance;
  auto api_version = VK_API_VERSION_1_3;
  {//! 初始化instance
    vk::ApplicationInfo app_info;
    app_info.setPApplicationName("VulkanBenchmark");
    app_info.setApiVersion(api_version);
    app_info.setApplicationVersion(1);

    const vector<const char *> layer_names = {"VK_LAYER_KHRONOS_validation"};
    vk::InstanceCreateInfo create_info({}, &app_info, layer_names, {});
    VK_CHECK(vk::createInstance(&create_info, nullptr, &instance));
  }

  auto &phy_device = vk_info_.phy_device;
  const vector<const char *> extension_names = {"VK_EXT_shader_atomic_float", "VK_KHR_shader_non_semantic_info"};
  {//! 初始化physical device
    vector<vk::PhysicalDevice> phy_devices = instance.enumeratePhysicalDevices();
    if (phy_devices.empty()) {
      cout << "[FATAL] No physical device found!" << endl;
      return false;
    }
    for(int i = 0; i<phy_devices.size(); i++) {
      const auto &device = phy_devices[i];
      vk::PhysicalDeviceProperties prop = device.getProperties();
      cout << "[INFO] Found " << i << "th physical device name: " << prop.deviceName
            << ", type: " << vk::to_string(prop.deviceType) << ", ";
      int not_support_id = check_device_extension(device, extension_names);
      if (not_support_id >= 0) {
        cout << "not support extension: " << extension_names[not_support_id] << endl;
        return false;
      };
      cout << endl;
    }

    int device_id = 0;
    cout << "Please input the device id you want to use: " << endl;
    cin >> device_id;
    if(device_id > phy_devices.size() - 1 || device_id < 0) {
      cout << "[ERROR] invalid device id " << device_id << ", use the first device by default" << endl;
      device_id = 0;
    }
    phy_device = phy_devices[device_id];
  }

  auto &device = vk_info_.device;
  auto &queue_idx = vk_info_.queue_idx;
  { //! 初始化device
    vector<vk::QueueFamilyProperties> queue_props = phy_device.getQueueFamilyProperties();
    queue_idx = UINT32_MAX;
    for(uint32_t i = 0; i < queue_props.size(); i++) {
      if(queue_props[i].queueFlags & vk::QueueFlagBits::eCompute) {
        queue_idx = i;
        break;
      }
    }
    if (queue_idx == UINT32_MAX) {
      cout << "[FATAL] No compute queue family found!" << endl;
      return false;
    }
    cout << "[INFO] compute queue family index: " << queue_idx << endl;

    float priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_info({}, queue_idx, 1, &priority);

    vk::PhysicalDeviceShaderAtomicFloatFeaturesEXT atomic_float_feat; // TODO: 这是啥？
    // atomic_float_feat.shaderBufferFloat32Atomics = vk::True;
    // atomic_float_feat.shaderSharedFloat32Atomics = vk::True;
    atomic_float_feat.shaderBufferFloat32AtomicAdd = vk::True;
    atomic_float_feat.shaderSharedFloat32AtomicAdd = vk::True;
    // NOTE: PhysicalDeviceShaderAtomicFloat2FeaturesEXT里支持半精度和双进度的float以及max和min

    vk::DeviceCreateInfo create_info;
    create_info.setQueueCreateInfos({queue_info});
    create_info.setPEnabledLayerNames({});
    create_info.setPEnabledExtensionNames(extension_names);
    create_info.setPNext(&atomic_float_feat);  // 链接到特性结构
    VK_CHECK(phy_device.createDevice(&create_info, nullptr, &device));
  }
  { //! 初始化command pool
    vk::CommandPoolCreateInfo create_info({}, queue_idx);
    VK_CHECK(device.createCommandPool(&create_info, nullptr, &vk_info_.cmd_pool));
  }
  { //! 获取queue
    vk_info_.queue = device.getQueue(queue_idx, 0);
  }
  { //! 初始化descriptor pool
    vector<vk::DescriptorPoolSize> pool_sizes = {
      {vk::DescriptorType::eStorageBuffer, 10},
      {vk::DescriptorType::eUniformBuffer, 10}
    };
    vk::DescriptorPoolCreateInfo create_info;
    create_info.setMaxSets(10);             // 可以分配的descriptor set的最大数量
    create_info.setPoolSizes(pool_sizes);
    create_info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    VK_CHECK(device.createDescriptorPool(&create_info, nullptr, &vk_info_.desc_pool));
  }
#ifdef USE_VMA
  { //! 初始化VmaAllocator
    VmaAllocatorCreateInfo create_info = {};
    create_info.vulkanApiVersion = api_version;
    create_info.physicalDevice = phy_device;
    create_info.device = device;
    create_info.instance = instance;
    VK_CHECK(vmaCreateAllocator(&create_info, &vk_info_.allocator));
  }
#else
  vk_info_.mem_props = phy_device.getMemoryProperties();
#endif
  return true;
}

bool Benchmark::CreateBuffers() {
  bool create_success = true;
  create_success &= buffers_["array1"].Init(vk_info_, sizeof(float), elem_num_, vk::BufferUsageFlagBits::eStorageBuffer,
      MEMORY_CPU_TO_GPU);
  create_success &= buffers_["array2"].Init(vk_info_, sizeof(float), elem_num_, vk::BufferUsageFlagBits::eStorageBuffer,
      MEMORY_CPU_TO_GPU);
  create_success &= buffers_["array3"].Init(vk_info_, sizeof(float), elem_num_, vk::BufferUsageFlagBits::eStorageBuffer,
      MEMORY_GPU_ONLY);
  create_success &= buffers_["num"].Init(vk_info_, sizeof(int), 1, vk::BufferUsageFlagBits::eUniformBuffer,
      MEMORY_CPU_TO_GPU);
  create_success &= buffers_["sum"].Init(vk_info_, sizeof(float), 1, vk::BufferUsageFlagBits::eStorageBuffer,
      MEMORY_GPU_TO_CPU);
  return create_success;
}

bool Benchmark::CreateDescriptors() {
  bool create_success = true;
  create_success &= desc_sets_["sum_two_array"].Init(vk_info_, {&buffers_["array1"], &buffers_["array2"],
      &buffers_["array3"], &buffers_["num"]});
  create_success &= desc_sets_["array_reduction"].Init(vk_info_, {&buffers_["array3"], &buffers_["sum"],
       &buffers_["num"]});
  return create_success;
}

bool Benchmark::CreatePipelines() {
  bool create_success = true;
  for (string name : {"sum_two_array", "array_reduction"})
    create_success &= pipelines_[name].Init(vk_info_, desc_sets_[name], shader::comp_spv[name]);
  return create_success;
}

bool Benchmark::AllocateCommandBuffer() {
  vk::CommandBufferAllocateInfo info;
  info.setCommandPool(vk_info_.cmd_pool);
  info.setLevel(vk::CommandBufferLevel::ePrimary);
  info.setCommandBufferCount(1);
  VK_CHECK(vk_info_.device.allocateCommandBuffers(&info, &cmd_buffer_));
  return true;
}

bool Benchmark::CreateFence() {
  vk::FenceCreateInfo info;
  VK_CHECK(vk_info_.device.createFence(&info, nullptr, &fence_));
  return true;
}
