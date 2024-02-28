#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const vector<const char *> validation_layers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const bool enable_validation_layers = false;
const bool enable_debug_messenger = false;
#else
const bool enable_validation_layers = true;
const bool enable_debug_messenger = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *create_info,
                                      const VkAllocationCallbacks *allocator,
                                      VkDebugUtilsMessengerEXT *debug_messenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, create_info, allocator, debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
                                   const VkAllocationCallbacks *allocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debug_messenger, allocator);
}

class HelloTriangleApplication {
  public:
    void Run() {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

  private:
    GLFWwindow *window_;
    VkInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info_;
    /** 初始化GLFW窗口 */
    void InitWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window_ = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }
    /* --------------------------------------------------- InitVulkan
     * --------------------------------------------------- */
    /** 初始化vulkan所需资源 */
    void InitVulkan() {
        if (enable_validation_layers && !CheckValidationLayerSupport())
            throw runtime_error("validation layers requested, but not available!");
        CreateInstance();
        if (enable_debug_messenger &&
            CreateDebugUtilsMessengerEXT(instance_, &debug_create_info_, nullptr, &debug_messenger_) != VK_SUCCESS)
            throw std::runtime_error("failed to set up debug messenger!");
    }
    /** 检查所需validation layer是否支持 */
    bool CheckValidationLayerSupport() {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        if (layer_count <= 0)
            throw runtime_error("found no validation layer!");
        vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
        // bool layer_found = false;
        for (const char *layer_name : validation_layers) {
            for (const auto &layer_properties : available_layers) {
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    // layer_found = true;
                    return true;
                }
                printf("[INFO] find layer name %s\n", layer_properties.layerName);
            }
        }
        return false;
    }
    /** 创建vulkan instance */
    void CreateInstance() {
        //! 创建 application info
        VkApplicationInfo app_info{}; // NOTE: {}不能省略
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Hello Triangle";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;
        //! 获取必须的extension 信息
        auto extensions = GetRequiredExtensions();
        //! 创建debug message info
        if (enable_debug_messenger) {
            debug_create_info_ = {};
            debug_create_info_.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debug_create_info_.messageSeverity = /* VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | */
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debug_create_info_.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debug_create_info_.pfnUserCallback = DebugCallback; // 自定义的回调函数
        }

        //! 创建create info
        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = extensions.size(); // extension
        create_info.ppEnabledExtensionNames = extensions.data();
        if (enable_validation_layers) { // validation layer
            create_info.enabledLayerCount = validation_layers.size();
            create_info.ppEnabledLayerNames = validation_layers.data();
        } else
            create_info.enabledLayerCount = 0;
        create_info.pNext = enable_debug_messenger ? // debug message //? 转指针
                                (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info_
                                                   : nullptr;

        //! 创建instance
        if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS)
            throw std::runtime_error("failed to create instance!");
    }
    /** 寻找必须的extensions */
    std::vector<const char *> GetRequiredExtensions() {
        uint32_t glfw_extension_count = 0;
        const char **glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
        if (enable_debug_messenger)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        return extensions;
    }
    /** 自定义的debug message回调函数 */
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                        VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                        void *user_data) {
        cerr << "validation layer: " << callback_data->pMessage << endl;
        return VK_FALSE;
    }
    void MainLoop() {
        while (!glfwWindowShouldClose(window_))
            glfwPollEvents();
    }
    void Cleanup() {
        if (enable_debug_messenger)
            DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
            
        vkDestroyInstance(instance_, nullptr);

        glfwDestroyWindow(window_);

        glfwTerminate();
    }
};

int main() {
    HelloTriangleApplication app;
    try {
        app.Run();
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}