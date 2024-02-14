#pragma once

#include "app/vulkan_api.h"
#include <vulkan/vk_enum_string_helper.h>

#include <iostream>

static void CHECK(bool assertion, const char *message) {
    if (!assertion) {
        std::cerr << "[fatal] assertion failed: " << message << std::endl;
        throw std::runtime_error(message);
    }
}

static void CHECK(vk::Result result, const char *message) {
    CHECK(result == vk::Result::eSuccess, message);
}

template<typename T>
static T CHECK(vk::ResultValue<T> x, const char *message) {
    if (x.result != vk::Result::eSuccess) {
        std::cerr << "[fatal] assertion failed: " << static_cast<int>(x.result) << std::endl;
    }
    CHECK(x.result == vk::Result::eSuccess, message);
    return x.value;
}
