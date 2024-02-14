#pragma once

#include <numeric>
#include <cmath>

#include "bit_vector.hpp"
#include "darray.hpp"
#include "compact_vector.hpp"


namespace pthash {
    struct golomb_sequence {

        template<typename Iterator>
        uint64_t optimalL(Iterator begin, uint64_t n) {
            uint64_t sum = std::accumulate(begin, begin + n, static_cast<uint64_t>(0));
            double p = (n / (double(sum) + n));
            const double gold = (sqrt(5.0) + 1.0) / 2.0;
            return uint64_t(
                    ceil(
                            log2(
                                    -log2(gold) / log2(1 - p)
                            )
                    )
            );
        }

        template<typename Iterator>
        void encode(Iterator begin, uint64_t n) {
            if (n == 0) return;

            uint64_t l = optimalL(begin, n);

            bit_vector_builder bvb_high_bits;
            compact_vector::builder cv_builder_low_bits(n, l);

            uint64_t low_mask = (uint64_t(1) << l) - 1;

            for (size_t i = 0; i < n; ++i, ++begin) {
                auto v = *begin;
                cv_builder_low_bits.push_back(v & low_mask);
                auto unary = v >> l;
                for (size_t j = 0; j < unary; ++j) {
                    bvb_high_bits.push_back(0);
                }
                bvb_high_bits.push_back(1);
            }

            bit_vector(&bvb_high_bits).swap(m_high_bits);
            cv_builder_low_bits.build(m_low_bits);
            m_high_bits_d1.build(m_high_bits);
        }

        inline uint64_t access(uint64_t i) const {
            assert(i < size());
            int64_t start = -1;
            if (i) {
                start = m_high_bits_d1.select(m_high_bits, i - 1);
            }
            int64_t end = m_high_bits_d1.select(m_high_bits, i);
            int64_t high = end - start - 1;
            return (high << m_low_bits.width()) | m_low_bits.access(i);
        }

        inline uint64_t size() const {
            return m_low_bits.size();
        }

        uint64_t num_bits() const {
            return 8 * (m_high_bits.bytes() + m_high_bits_d1.bytes() + m_low_bits.bytes());
        }

        template<typename Visitor>
        void visit(Visitor &visitor) {
            visitor.visit(m_high_bits);
            visitor.visit(m_high_bits_d1);
            visitor.visit(m_low_bits);
        }

    private:
        bit_vector m_high_bits;
        darray1 m_high_bits_d1;
        compact_vector m_low_bits;
    };

}  // namespace pthash