#pragma once

#include "vulkan/vulkan.hpp"
#include <optional>

struct QueueIndices {
    std::optional<std::uint32_t> compute_;

    operator bool() const { return compute_.has_value(); }
};

class Context {
  public:
    Context(const std::string &spv_path, uint32_t size);
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

    /* ----------------------------------------------------- command ------------------------------------------------ */
    void CreateCommandPoolAndBuffer();
    vk::CommandPool cmd_pool_;
    vk::CommandBuffer cmd_buffer_;

    /* ------------------------------------------------- descriptor set --------------------------------------------- */
    void CreateDescriptorSet(uint32_t size);
    vk::DescriptorPool descriptor_pool_ = nullptr;
    vk::DescriptorSetLayout descriptor_layout_;
    vk::DescriptorSet descriptor_set_;

    /* ------------------------------------------------ compute pipeline -------------------------------------------- */
    void CreateComputePipeline(const std::string &spv_path);
    // vk::PipelineCache pipeline_cache_ = nullptr;
    vk::PipelineLayout pipeline_layout_ = nullptr;
    vk::Pipeline compute_pipeline_ = nullptr;
    // vk::ShaderModule compute_module_;
};