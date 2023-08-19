#pragma once

#define DEFAULT_VK_SAMPLER\
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,\
    .minFilter = VK_FILTER_LINEAR,\
    .magFilter = VK_FILTER_LINEAR,\
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,\
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,\
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,\
    .anisotropyEnable = VK_TRUE,\
    .maxAnisotropy = physical_device_properties->limits.maxSamplerAnisotropy,\
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,\
    .unnormalizedCoordinates = VK_FALSE,\
    .compareEnable = VK_FALSE,\
    .compareOp = VK_COMPARE_OP_ALWAYS,\
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,\
    .minLod = 0.0f,\
    .maxLod = 0.0f,\
    .mipLodBias = 0.0f



