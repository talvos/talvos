#include "vulkan/vulkan_core.h"

/// Encapsulates a runtime test session.
class TestContext
{
public:
  TestContext(const char *AppName);
  ~TestContext();

  // Vulkan objects.
  VkInstance Instance;
  VkPhysicalDevice PhysicalDevice;
  VkDevice Device;
  VkQueue Queue;
  VkDescriptorPool DescriptorPool;
  VkCommandPool CommandPool;
};

/// Exit with an error if (Result != VK_SUCCESS).
void check(VkResult Result, const char *Operation);
