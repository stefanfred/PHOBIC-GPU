#pragma once

#include "encoders/base/encoders.hpp"

namespace gpupthash {

template <typename BaseEncoder>
struct direct_partition_encoder {
private:
    BaseEncoder enc;

public:
    template <typename Iterator>
    void encode(Iterator begin, uint64_t partitionsSize, uint64_t partitions) {
        enc.encode(begin, partitions);
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
}  // namespace gpupthash