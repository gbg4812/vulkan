#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vector>

#include "vk_utils.h"

class vk_SwapChain {
   public:
    vk_SwapChain(int width, int height, VkDevice device,
                 VkPhysicalDevice physicalDevice);

    ~vk_SwapChain();

    VkFramebuffer getFrameBuffer(int index);
    VkImage getImage(int index);
    VkFormat getImageFormat();
    VkExtent2D getImageExtent();
    VkSwapchainKHR getSwapChain();
    VkSurfaceKHR surface;
    virtual const std::vector<const char*> getRequiredExtensions();

    void recreate(int width, int height, VkDevice device,
                  VkPhysicalDevice physicalDevice);

   private:
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    virtual void createSurface();

    VkSwapchainKHR swapChain;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainImageExtent;
    std::vector<VkImageView> swapChainImageViews;
    bool frameBufferResized = false;

    VkDevice device;

    SwapChainSupportDetails querySupport(VkPhysicalDevice physicalDevice);

    VkSurfaceFormatKHR chooseFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR choosePresentMode(
        const std::vector<VkPresentModeKHR>& presentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                                int width, int height);

    void cleanup();
    void create(int width, int height, VkDevice device,
                VkPhysicalDevice physicalDevice);
    void createImageViews();
    void createColorResources();
    void createDepthResources();
    void createFrameBuffers();
};
