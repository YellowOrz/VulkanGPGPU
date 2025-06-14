#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>

struct VkInfo {
  vk::Instance instance;
  vk::PhysicalDevice phy_device;
  vk::Device device;
  uint32_t queue_idx;
  // vk::Queue queue;
  vk::CommandPool cmd_pool;
  vk::DescriptorPool desc_pool;
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
};




