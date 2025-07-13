#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#ifdef USE_VMA
#include "vk_mem_alloc.h" // Vulkan Memory Allocator library
#endif

// NOTE: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/group__group__alloc.html#gaa5846affa1e9da3800e3e78fae2305cc
#define MEMORY_GPU_ONLY VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
#define MEMORY_CPU_ONLY VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
#define MEMORY_CPU_TO_GPU VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
#define MEMORY_GPU_TO_CPU VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT
// #define MEMORY_GPU_ONLY vk::MemoryPropertyFlagBits::eDeviceLocal
// #define MEMORY_CPU_ONLY vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
// #define MEMORY_CPU_TO_GPU vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
// #define MEMORY_GPU_TO_CPU vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached

struct VkInfo {
  void Destroy();
  // ~VkInfo() { Destroy(); }
  vk::Instance instance;
  vk::PhysicalDevice phy_device;
  vk::Device device;
  uint32_t queue_idx;
  vk::Queue queue;
  vk::CommandPool cmd_pool;
  vk::DescriptorPool desc_pool;
#ifdef USE_VMA
  VmaAllocator allocator;
#else
  vk::PhysicalDeviceMemoryProperties mem_props;
#endif
};

struct Buffer {
  bool Init(const VkInfo &info, size_t elem_size, size_t num, vk::BufferUsageFlags buff_usage,
    int alloc_usage);
  void Destroy();
  /** 设置数据 */
  template<typename T>
  bool SetData(T *data, size_t num);  // TODO: 只有CPU VISIABLE的才能set和get
  /** 数据初始化为0 */
  bool SetZero();
  /** 获取数据 */
  template<typename T>
  bool GetData(T *data, size_t num);
  ~Buffer(){ Destroy(); }

  vk::Buffer buffer;
  size_t one_elem_size;
  size_t num_elems;
  size_t size;
  vk::BufferUsageFlags usage;

#ifdef USE_VMA
  VmaAllocation allocation; // Vulkan Memory Allocator allocation
  VmaAllocator allocator; // Vulkan Memory Allocator instance
#else
  vk::Device device;
  vk::DeviceMemory mem;
  vk::MemoryRequirements mem_req; // 记录对齐后的内存大小
#endif
};

struct DescriptorSet {
  bool Init(const VkInfo &info, const std::vector<Buffer*> &buffers);
  void Destroy();
  ~DescriptorSet() { Destroy(); }
  vk::DescriptorSet set;
  vk::DescriptorSetLayout layout;
  vk::Device device;
};

struct Pipeline {
  bool Init(const VkInfo &info, const DescriptorSet &desc, const std::vector<uint32_t> &shader_code);
  void Destroy();
  ~Pipeline() { Destroy(); }
  vk::Pipeline pipeline;
  vk::PipelineLayout layout;
  vk::PipelineCache cache;  // TODO: 有必要吗
  vk::ShaderModule shader_module;
  vk::Device device;
};

struct float3 {
  float3(float n): x(n), y(n), z(n) {}
  float x,y,z;
};

struct Input {
  Input(float n) :num1(n), vec(n), num2(n) {}
  alignas(4) float num1;
  alignas(16) float3 vec;
  alignas(4) int num2;
};

class Benchmark {
public:
  Benchmark(int elem_num);
  ~Benchmark();

  bool CreateSuccess() const {return create_succrss_;};
  bool Run();

private:
  bool InitVkInfo();
  bool CreateBuffers();
  bool CreateDescriptors();
  bool CreatePipelines();
  bool AllocateCommandBuffer();
  bool CreateFence();


  VkInfo vk_info_;
  std::unordered_map<std::string, Buffer> buffers_;
  std::unordered_map<std::string, DescriptorSet> desc_sets_;
  std::unordered_map<std::string, Pipeline> pipelines_;
  vk::CommandBuffer cmd_buffer_;
  vk::Fence fence_;

  int elem_num_;
  bool create_succrss_;
};




