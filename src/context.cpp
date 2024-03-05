#include "context.hpp"
#include <iostream>
#include <vector>

using namespace std;
Context::Context() {
    CreateInstance();
    PickupPhysicalDevice();
    CreateLogicalDevice();
    compute_queue_ = device_.getQueue(queue_indices_.compute_.value(), 0);
}

Context::~Context() {
    device_.destroy();
    instance_.destroy();
}

void Context::CreateInstance() {
    vk::ApplicationInfo app_info;
    app_info.setApiVersion(VK_API_VERSION_1_3);

    std::vector<const char *> layers = {"VK_LAYER_KHRONOS_validation"};

    vk::InstanceCreateInfo create_info;
    create_info.setPApplicationInfo(&app_info).setPEnabledLayerNames(layers);

    instance_ = vk::createInstance(create_info);
    if (!instance_)
        throw std::runtime_error("failed to create instance!");
}

void Context::PickupPhysicalDevice() {
    auto devices = instance_.enumeratePhysicalDevices();
    if (devices.size() == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    for (auto &d : devices) {
        cout << "[INFO] find physical deivce " << d.getProperties().deviceName << endl;
    }
    phy_device_ = devices[0];
    cout << "[INFO] pick up physical deivce " << phy_device_.getProperties().deviceName << endl;
}

void Context::QueryQueueInfo() {
    auto queue_props = phy_device_.getQueueFamilyProperties();
    for (int i = 0; i < queue_props.size(); i++) {
        if (queue_props[i].queueFlags & vk::QueueFlagBits::eCompute)
            queue_indices_.compute_ = i;
        if (queue_indices_)
            break;
    }
}
void Context::CreateLogicalDevice() {
    QueryQueueInfo();
    vk::DeviceQueueCreateInfo queue_create_info;
    float priority = 1;
    queue_create_info.setQueueFamilyIndex(queue_indices_.compute_.value())
        .setPQueuePriorities(&priority)
        .setQueueCount(1);

    vk::DeviceCreateInfo create_info;
    create_info.setQueueCreateInfos(queue_create_info);

    device_ = phy_device_.createDevice(create_info);
    if (!device_)
        throw std::runtime_error("failed to create logical device!");
}