#pragma once

#include "encoders.hpp"

namespace pthash {

template <typename BaseEncoder>
struct mono_encoder {
    template <typename Iterator>
    void encode(Iterator begin, uint64_t partitions, uint64_t buckets) {
        bucketsPerPart = buckets;
        enc.encode(begin, partitions * buckets);
    }

    static std::string name() {
        return BaseEncoder::name();
    }

    size_t size() const {
        return enc.size();
    }

    size_t num_bits() const {
        return enc.num_bits();
    }

    inline uint64_t access(uint64_t partition, uint64_t bucket) const {
        return enc.access(bucketsPerPart * partition + bucket);
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(enc);
    }

    private:
    uint64_t bucketsPerPart;
    BaseEncoder enc;
};
}  // namespace pthash