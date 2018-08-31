// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include "talvos/Device.h"
#include "talvos/Image.h"
#include "talvos/Memory.h"

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo,
                const VkAllocationCallbacks *pAllocator, VkSampler *pSampler)
{
  *pSampler = new VkSampler_T;

  (*pSampler)->Sampler = new talvos::Sampler(*pCreateInfo);

  // Create sampler object in memory.
  talvos::Memory &Mem = device->Device->getGlobalMemory();
  (*pSampler)->ObjectAddress = Mem.allocate(sizeof(talvos::Sampler *));
  Mem.store((*pSampler)->ObjectAddress, sizeof(talvos::Sampler *),
            (uint8_t *)&(*pSampler)->Sampler);

  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSamplerYcbcrConversion(
    VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkSamplerYcbcrConversion *pYcbcrConversion)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSamplerYcbcrConversionKHR(
    VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkSamplerYcbcrConversion *pYcbcrConversion)
{
  return vkCreateSamplerYcbcrConversion(device, pCreateInfo, pAllocator,
                                        pYcbcrConversion);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySampler(
    VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator)
{
  if (sampler)
  {
    device->Device->getGlobalMemory().release(sampler->ObjectAddress);
    delete sampler->Sampler;
    delete sampler;
  }
}

VKAPI_ATTR void VKAPI_CALL vkDestroySamplerYcbcrConversion(
    VkDevice device, VkSamplerYcbcrConversion ycbcrConversion,
    const VkAllocationCallbacks *pAllocator)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkDestroySamplerYcbcrConversionKHR(
    VkDevice device, VkSamplerYcbcrConversion ycbcrConversion,
    const VkAllocationCallbacks *pAllocator)
{
  vkDestroySamplerYcbcrConversion(device, ycbcrConversion, pAllocator);
}
