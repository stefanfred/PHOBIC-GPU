#pragma once

#include "encoders.hpp"

namespace pthash {

    template<typename BaseEncoder>
    struct linear_diff_encoder {
    private:
        int64_t increment;
        BaseEncoder enc;

    public:
        template<typename Iterator>
        void encode(Iterator begin, uint64_t size, uint64_t increment) {
            this->increment = increment;
            std::vector<uint64_t> diffValues;
            diffValues.reserve(size);
            int64_t expected = 0;
            for (uint64_t i = 0; i != size; ++i, ++begin) {
                int64_t toEncode = *begin - expected;
                uint64_t absToEncode = abs(toEncode);
                diffValues.push_back((absToEncode<<1) | uint64_t(toEncode>0));
                expected += increment;
            }

            enc.encode(diffValues.begin(), size);
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

        inline uint64_t access(uint64_t i) const {
            uint64_t value = enc.access(i);
            uint64_t expected = i * increment;
            int64_t sValue = (int64_t(value & 1) * 2 - 1) * int64_t(value >> 1);
            return sValue + expected;
        }

        template<typename Visitor>
        void visit(Visitor &visitor) {
            visitor.visit(enc);
        }
    };
}  // namespace pthash