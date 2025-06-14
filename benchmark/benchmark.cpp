#include "benchmark.h"
#include <iostream>

using namespace std;

Benchmark::Benchmark() {
    // 初始化
}
Benchmark::~Benchmark() {
    // 清理
}
void Benchmark::run() {
}
bool Benchmark::init_vk_info() {
  auto &instance = vk_info_.instance;
  {//! 初始化instance
    vk::ApplicationInfo app_info;
    app_info.setPApplicationName("VulkanBenchmark");
    app_info.setApiVersion(VK_API_VERSION_1_3);
    app_info.setApplicationVersion(1);

    const vector<const char *> layer_names = {"VK_LAYER_KHRONOS_validation"};
    vk::InstanceCreateInfo create_info({}, &app_info, layer_names, {});
    instance = vk::createInstance(create_info);
  }

  auto &phy_device = vk_info_.phy_device;
  {//! 初始化physical device
    vector<vk::PhysicalDevice> phy_devices = instance.enumeratePhysicalDevices();
    if (phy_devices.empty()) {
      cout << "[FATAL] No physical device found!" << endl;
      return false;
    }
    for(const auto &device : phy_devices) {
      vk::PhysicalDeviceProperties prop = device.getProperties();
      cout << "[INFO] found physical device name: " << prop.deviceName
            << ", type: " << vk::to_string(prop.deviceType) << endl;
    }

    int device_id = 0;
    cout << "Please input the device id you want to use: ";
    cin >> device_id;
    if(device_id > phy_devices.size() - 1 || device_id < 0) {
      cout << "[ERROR] invalid device id, use the first device by default" << endl;
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
    const vector<const char *> extension_names = {"VK_EXT_shader_atomic_float", "VK_KHR_shader_non_semantic_info"};
    vk::DeviceCreateInfo create_info({}, 1, &queue_info, 0, nullptr, 0, nullptr);
    device = phy_device.createDevice(create_info);
  }

  { //! 初始化command pool
    vk::CommandPoolCreateInfo create_info({}, queue_idx);
    vk_info_.cmd_pool = device.createCommandPool(create_info);
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
    vk_info_.desc_pool = device.createDescriptorPool(create_info);
  }
  return true;
}

bool Benchmark::init_descriptors() {

  return true;
}