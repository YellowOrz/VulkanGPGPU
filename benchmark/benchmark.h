#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include "vk_mem_alloc.h" // Vulkan Memory Allocator library

struct VkInfo {
  vk::Instance instance;
  vk::PhysicalDevice phy_device;
  vk::Device device;
  uint32_t queue_idx;
  // vk::Queue queue;
  vk::CommandPool cmd_pool;
  vk::DescriptorPool desc_pool;
  VmaAllocator allocator;
};

struct Buffer {
  bool init(const VkInfo &info, size_t elem_size, size_t num, vk::BufferUsageFlags buff_usage,
    VmaMemoryUsage alloc_usage);
  ~Buffer();
  vk::Buffer buffer;
  VmaAllocation allocation; // Vulkan Memory Allocator allocation
  VmaAllocator allocator; // Vulkan Memory Allocator instance
  size_t one_elem_size;
  size_t num_elems;
  size_t size;
  vk::BufferUsageFlags usage;

};

struct DescriptorSet {
  bool init(const VkInfo &info, const std::vector<Buffer*> &buffers);
  ~DescriptorSet();
  vk::DescriptorSet set;
  vk::DescriptorSetLayout layout;
  vk::Device device;
};

struct Pipeline {
  bool init(const VkInfo &info, const vk::DescriptorSetLayout &desc_layout, const std::vector<uint32_t> &shader_code);
  vk::Pipeline pipeline;
  vk::PipelineLayout layout;
  vk::PipelineCache cache;  // TODO: 有必要吗
  vk::ShaderModule shader_module;
  vk::Device device;
};

class Benchmark {
public:
  Benchmark();
  ~Benchmark();

  void run();

private:
  bool init_vk_info();
  bool init_buffers();
  bool init_descriptors();
  bool init_pipelines();

  VkInfo vk_info_;
  std::unordered_map<std::string, Buffer> buffers_;
  std::unordered_map<std::string, DescriptorSet> desc_sets_;
  std::unordered_map<std::string, Pipeline> pipelines_;
};




