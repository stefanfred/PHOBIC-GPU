#pragma once

#include "ortho_encoder.hpp"

namespace gpupthash {

    template<typename BaseEncoder1, typename BaseEncoder2>
    struct ortho_encoder_dual {
        template<typename Iterator>
        void encode(Iterator begin, uint64_t partitions, uint64_t buckets) {
            initialized = true;
            buckets1 = buckets * tradeoff;
            uint64_t buckets2 = buckets - buckets1;

#pragma omp task
            encoder1.encode(begin, partitions, buckets1);
            encoder2.encode(begin + buckets1 * partitions, partitions, buckets2);
        }

        void setEncoderTradeoff(float tradeoff) {
            assert(tradeoff <= 1.0);
            assert(tradeoff >= 0.0);
            assert(!initialized);
            this->tradeoff = tradeoff;
        }

        inline uint64_t access(uint64_t partition, uint64_t bucket) const {
            if (bucket < buckets1) {
                return encoder1.access(partition, bucket);
            } else {
                return encoder2.access(partition, bucket - buckets1);
            }
        }

        std::string name() {
            return "OrthoEncoderDual<" + BaseEncoder1::name() + ", " + BaseEncoder2::name() + ", " + tradeoff + ">";
        }

        uint64_t num_bits() const {
            return encoder1.num_bits() + encoder2.num_bits();
        }

        template<typename Visitor>
        void visit(Visitor &visitor) {
            encoder1.visit(visitor);
            encoder2.visit(visitor);
        }

    private:
        bool initialized = false;
        uint64_t buckets1;
        float tradeoff = 0.5;
        ortho_encoder <BaseEncoder1> encoder1;
        ortho_encoder <BaseEncoder2> encoder2;
    };
}  // namespace gpupthash