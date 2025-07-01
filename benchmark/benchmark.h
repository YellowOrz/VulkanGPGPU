#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include "vk_mem_alloc.h" // Vulkan Memory Allocator library

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
  VmaAllocator allocator;
};

struct Buffer {
  enum Usage {  // NOTE: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/group__group__alloc.html#gaa5846affa1e9da3800e3e78fae2305cc
    kGPU_ONLY = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    kCPU_ONLY = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    kCPU_TO_GPU = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    kGPU_TO_CPU = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    kCPU_COPY = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  };
  bool Init(const VkInfo &info, size_t elem_size, size_t num, vk::BufferUsageFlags buff_usage,
    Usage alloc_usage);
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
  VmaAllocation allocation; // Vulkan Memory Allocator allocation
  VmaAllocator allocator; // Vulkan Memory Allocator instance
  size_t one_elem_size;
  size_t num_elems;
  size_t size;
  vk::BufferUsageFlags usage;

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

// struct Barrier {
//   bool Init(const VkInfo &info);
//   vk::MemoryBarrier barrier;
// };

class Benchmark {
public:
  Benchmark(int elem_num);
  ~Benchmark();

  bool CreateSuccess() const {return create_succrss_;};
  bool run();

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




