#pragma once

namespace gpupthash {

    template<typename BaseEncoder>
    struct ortho_encoder {

        template<typename Iterator>
        void encode(Iterator begin, uint64_t partitions, uint64_t buckets) {
            encoders.resize(buckets);
            //#pragma omp parallel for
            for (size_t j = 0; j < buckets; j++) {
                encoders[j].encode(begin + j * partitions, partitions);
            }
        }

        inline uint64_t access(uint64_t partition, uint64_t bucket) const {
            return encoders[bucket].access(partition);
        }


        static std::string name() {
            return "OrthoEncoder<" + BaseEncoder::name() + ">";
        }

        uint64_t num_bits() const {
            uint64_t sum = 0;
            for (auto &e: encoders) {
                sum += e.num_bits();
            }
            return sum;
        }

        template<typename Visitor>
        void visit(Visitor &visitor) {
            for (auto &e: encoders) {
                e.visit(visitor);
            }
        }

    private:
        std::vector<BaseEncoder> encoders;
    };
}