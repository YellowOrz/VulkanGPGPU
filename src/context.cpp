#include "context.hpp"
#include "tool.hpp"
#include <iostream>
#include <vector>

using namespace std;
Context::Context(const std::string &spv_path) {
    //! init vulkan
    CreateInstance();
    PickupPhysicalDevice();
    CreateLogicalDevice();
    compute_queue_ = device_.getQueue(queue_indices_.compute_.value(), 0);

    CreateShader(spv_path);
    CreateComputePipeline();
    CreateCommandPool();
}

Context::~Context() {
    device_.destroyPipeline();
    device_.destroyPipelineLayout();
    device_.destroy();
    instance_.destroy();
}
/* ------------------------------------------------------------------------------------------------------------------ */
/*                                                     init vulkan                                                    */
/* ------------------------------------------------------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------------------------------------------------------ */
/*                                                    create shader                                                   */
/* ------------------------------------------------------------------------------------------------------------------ */
void Context::CreateShader(const std::string &spv_path) {
    //! create shader module
    auto source_code = ReadWholeFile(spv_path);
    vk::ShaderModuleCreateInfo module_info;
    module_info.setCodeSize(source_code.size()).setPCode((uint32_t *)source_code.data());
    compute_module_ = device_.createShaderModule(module_info);
    //! descriptor set layout
    vk::DescriptorSetLayoutBinding binding;
    binding.setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eCompute);
    vk::DescriptorSetLayoutCreateInfo layout_info;
    layout_info.setBindings(binding);
    compute_descrip_layouts_ = device_.createDescriptorSetLayout(layout_info);
}

/* ------------------------------------------------------------------------------------------------------------------ */
/*                                                  compute pipeline                                                  */
/* ------------------------------------------------------------------------------------------------------------------ */
void Context::CreateComputePipeline() {
    // vk::PushConstantRange range; // TODO: 需要吗
    // range.setOffset()
    //! pipeline layout info & create
    vk::PipelineLayoutCreateInfo layout_info;
    layout_info.setSetLayouts(compute_descrip_layouts_); // ?需要setLayoutCount吗
    pipeline_layout_ = device_.createPipelineLayout(layout_info);
    //! pipeline shader stage info
    vk::PipelineShaderStageCreateInfo stage_info;
    stage_info.setModule(compute_module_).setStage(vk::ShaderStageFlagBits::eCompute).setPName("main");
    //! pipeline info & create
    vk::ComputePipelineCreateInfo pipeline_info;
    pipeline_info.setLayout(pipeline_layout_)
        .setStage(stage_info)
        .setBasePipelineHandle(nullptr)
        .setBasePipelineIndex(-1);
    auto result = device_.createComputePipelines(pipeline_cache_, pipeline_info);
    if (result.result != vk::Result::eSuccess)
        throw std::runtime_error("failed to create compute pipeline!");

    compute_pipeline_ = result.value[0];    // NOTE: result.value是个vector
}

/* ------------------------------------------------------------------------------------------------------------------ */
/*                                                    command pool                                                    */
/* ------------------------------------------------------------------------------------------------------------------ */
void Context::CreateCommandPool() {
    vk::CommandPoolCreateInfo pool_info;
    pool_info.setQueueFamilyIndex(queue_indices_.compute_.value())
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    pool_ = device_.createCommandPool(pool_info);
}
