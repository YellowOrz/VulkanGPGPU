#include "context.hpp"
#include "tool.hpp"
#include <iostream>
#include <vector>

using namespace std;
Context::Context(const std::string &spv_path, uint32_t size) {
    //! init vulkan
    CreateInstance();
    PickupPhysicalDevice();
    CreateLogicalDevice();
    compute_queue_ = device_.getQueue(queue_indices_.compute_.value(), 0);

    CreateDescriptorSet(size);

    CreateCommandPoolAndBuffer();

    CreateComputePipeline(spv_path);
}

Context::~Context() {
    printf("[INFO] compute finished, clear\n");
    device_.destroyCommandPool(cmd_pool_);
    device_.destroyDescriptorPool(descriptor_pool_);
    device_.destroyDescriptorSetLayout(descriptor_layout_);
    device_.destroyPipeline(compute_pipeline_);
    device_.destroyPipelineLayout(pipeline_layout_);
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
        if (queue_props[i].queueFlags | vk::QueueFlagBits::eCompute)
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
/*                                                       command                                                      */
/* ------------------------------------------------------------------------------------------------------------------ */
void Context::CreateCommandPoolAndBuffer() {
    //! command pool
    vk::CommandPoolCreateInfo pool_info;
    pool_info.setQueueFamilyIndex(queue_indices_.compute_.value())
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    cmd_pool_ = device_.createCommandPool(pool_info);
    if(!cmd_pool_)
        throw std::runtime_error("failed to create command pool!");
    //! command buffer
    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.setCommandPool(cmd_pool_).setCommandBufferCount(1).setLevel(vk::CommandBufferLevel::ePrimary);
    auto bufs = device_.allocateCommandBuffers(alloc_info);
    if(bufs.empty())
        throw std::runtime_error("failed to allocate command buffer!");
    cmd_buffer_ = bufs[0];
}

/* ------------------------------------------------------------------------------------------------------------------ */
/*                                                   descriptor set                                                   */
/* ------------------------------------------------------------------------------------------------------------------ */
void Context::CreateDescriptorSet(uint32_t size) {
    //! create desrcriptor pool
    vk::DescriptorPoolSize pool_size;
    pool_size.setType(vk::DescriptorType::eStorageBuffer).setDescriptorCount(size);

    vk::DescriptorPoolCreateInfo pool_info;
    pool_info.setPoolSizes(pool_size).setMaxSets(size);

    descriptor_pool_ = device_.createDescriptorPool(pool_info);
    
    //! create descriptor layout
    vk::DescriptorSetLayoutBinding binding;
    binding.setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)  // TODO: 换成eUniformBuffer？
        .setStageFlags(vk::ShaderStageFlagBits::eCompute);

    vk::DescriptorSetLayoutCreateInfo layout_info;
    layout_info.setBindings(binding);

    descriptor_layout_ = device_.createDescriptorSetLayout(layout_info);

    //! create descriptor set
    std::array layouts = {descriptor_layout_};
    vk::DescriptorSetAllocateInfo alloc_info;
    alloc_info.setDescriptorPool(descriptor_pool_).setSetLayouts(layouts)
              .setDescriptorSetCount(1);    // TODO: 是1吗

    auto sets = device_.allocateDescriptorSets(alloc_info);
    if(sets.empty())
        throw std::runtime_error("failed to allocate descriptor set!");
    descriptor_set_ = sets[0];
}


/* ------------------------------------------------------------------------------------------------------------------ */
/*                                                  compute pipeline                                                  */
/* ------------------------------------------------------------------------------------------------------------------ */
void Context::CreateComputePipeline(const std::string &spv_path) {
    // vk::PushConstantRange range; // TODO: 需要吗
    // range.setOffset()

    //! pipeline layout info & create
    vk::PipelineLayoutCreateInfo layout_info;
    layout_info.setSetLayouts(descriptor_layout_); // ?需要setLayoutCount吗
    pipeline_layout_ = device_.createPipelineLayout(layout_info);
    if(!pipeline_layout_)
        throw std::runtime_error("failed to create pipeline layout!");

    //! create shader module
    auto source_code = ReadWholeFile(spv_path);
    vk::ShaderModuleCreateInfo module_info;
    module_info.setCodeSize(source_code.size()).setPCode((uint32_t *)source_code.data());
    vk::ShaderModule compute_module = device_.createShaderModule(module_info);
    //! pipeline shader stage info
    vk::PipelineShaderStageCreateInfo stage_info;
    stage_info.setModule(compute_module).setStage(vk::ShaderStageFlagBits::eCompute).setPName("main");

    //! pipeline info & create
    vk::ComputePipelineCreateInfo pipeline_info;
    pipeline_info.setLayout(pipeline_layout_)
        .setStage(stage_info);
    auto result = device_.createComputePipeline({}, pipeline_info);
    if (result.result != vk::Result::eSuccess)
        throw std::runtime_error("failed to create compute pipeline!");

    compute_pipeline_ = result.value;

    device_.destroyShaderModule(compute_module);
}