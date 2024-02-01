#pragma once

#include <optional>

struct QueueFamilyIndices {
    std::optional<uint32_t> transferFamily;
    std::optional<uint32_t> computeFamily;
    

    bool isComplete() const {
        return transferFamily.has_value()
            && computeFamily.has_value();
    }
};
