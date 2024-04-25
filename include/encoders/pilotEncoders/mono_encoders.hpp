#pragma once

#include "encoders/base/encoders.hpp"

namespace phobicgpu {

template <typename BaseEncoder>
struct mono_encoder {
private:
    uint64_t partitions;
    BaseEncoder enc;

public:
    template <typename Iterator>
    void encode(Iterator begin, uint64_t partitions, uint64_t buckets) {
        this->partitions = partitions;
        enc.encode(begin, partitions * buckets);
    }

    std::string name() {
        return BaseEncoder::name();
    }

    size_t size() const {
        return enc.size();
    }

    size_t num_bits() const {
        return enc.num_bits();
    }

    inline uint64_t access(uint64_t partition, uint64_t bucket) const {
        return enc.access(partitions * bucket + partition);
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(enc);
    }
};
}