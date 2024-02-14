#pragma once

#include "ortho_encoder.hpp"

namespace pthash {

template <typename BaseEncoder1, typename BaseEncoder2, int num, int dom>
struct ortho_encoder_dual {

    template <typename Iterator>
    void encode(Iterator begin, uint64_t partitions, uint64_t buckets) {
        buckets1 = buckets * num/dom;
        uint64_t buckets2 = buckets - buckets1;

        #pragma omp task
        encoder1.encode(begin, partitions, buckets1);
        encoder2.encode(begin + buckets1 * partitions, partitions, buckets2);
    }

    inline uint64_t access(uint64_t partition, uint64_t bucket) const {
        if (bucket < buckets1) {
            return encoder1.access(partition, bucket);
        } else {
            return encoder2.access(partition, bucket - buckets1);
        }
    }


    static std::string name() {
        return "OrthoEncoderDual<"+BaseEncoder1::name()+", "+BaseEncoder2::name()+", "+num+", "+dom+"+>";
    }

    uint64_t num_bits() const {
        return encoder1.num_bits() + encoder2.num_bits();
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        encoder1.visit(visitor);
        encoder2.visit(visitor);
    }

private:
    uint64_t buckets1;
    ortho_encoder<BaseEncoder1> encoder1;
    ortho_encoder<BaseEncoder2> encoder2;
};
}