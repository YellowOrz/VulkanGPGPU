#pragma once

#include "vulkan/vulkan.hpp" 
#include <optional>

struct QueueIndices {
    std::optional<std::uint32_t> compute_;

    operator bool() const {
        return compute_.has_value();
    }
};

class Context{
    public:
    Context();
    ~Context();

    private:
    void CreateInstance();
    void PickupPhysicalDevice();
    void QueryQueueInfo();
    void CreateLogicalDevice();

    vk::Instance instance_;
    vk::PhysicalDevice phy_device_;
    vk::Device device_;
    vk::Queue compute_queue_;
    QueueIndices queue_indices_;
    
};