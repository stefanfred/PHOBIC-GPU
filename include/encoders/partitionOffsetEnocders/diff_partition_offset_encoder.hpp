#pragma once

#include "encoders/base/linear_diff_encoder.hpp"

namespace phobicgpu {

template <typename BaseEncoder>
struct diff_partition_encoder {
private:
    linear_diff_encoder<BaseEncoder> enc;

public:
    template <typename Iterator>
    void encode(Iterator begin, uint64_t partitionsSize, uint64_t partitions) {
        enc.encode(begin, partitions, partitionsSize);
    }

    static std::string name() {
        return "DiffToExpected<" + BaseEncoder::name() + ">";
    }

    size_t size() const {
        return enc.size();
    }

    size_t num_bits() const {
        return enc.num_bits();
    }

    inline uint64_t access(uint64_t partition) const {
        return enc.access(partition);
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(enc);
    }
};
}