#pragma once

#include "vulkan/vulkan.hpp"
#include <optional>

struct QueueIndices {
    std::optional<std::uint32_t> compute_;

    operator bool() const { return compute_.has_value(); }
};

class Context {
  public:
    Context(const std::string &spv_path);
    ~Context();

  private:
    /* --------------------------------------------------- init vulkan ---------------------------------------------- */
    void CreateInstance();
    void PickupPhysicalDevice();
    void QueryQueueInfo();
    void CreateLogicalDevice();

    vk::Instance instance_;
    vk::PhysicalDevice phy_device_;
    vk::Device device_;
    vk::Queue compute_queue_;
    QueueIndices queue_indices_;

    /* ----------------------------------------------------- shader ------------------------------------------------- */
    void CreateShader(const std::string &spv_path);
    vk::ShaderModule compute_module_;
    vk::DescriptorSetLayout compute_descrip_layouts_;

    /* ------------------------------------------------ compute pipeline -------------------------------------------- */
    void CreateComputePipeline();
    vk::PipelineCache pipeline_cache_ = nullptr;
    vk::PipelineLayout pipeline_layout_ = nullptr;
    vk::Pipeline compute_pipeline_ = nullptr;

    /* -------------------------------------------------- command pool ---------------------------------------------- */
    void CreateCommandPool();
    vk::CommandPool pool_;
};