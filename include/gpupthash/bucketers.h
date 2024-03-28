#include <numeric>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

namespace gpupthash {

    class Bucketer {
    public:

        virtual void init(double lambda, double expectedTableSize) {};
        virtual double getBucketRel(double relInput) const = 0;

        virtual ~Bucketer() {};
    };

    class UniformBucketer : public Bucketer {
    public:

        double getBucketRel(double relInput) const {
            return relInput;
        }
    };

    class OptBucketer : public Bucketer {
    public:
        static constexpr double local_collision_factor = 0.3;
        double slope;

        void init(double lambda, double expectedTableSize) {
            slope = std::max(
                    0.05, std::min(1.0, local_collision_factor * lambda / std::sqrt(expectedTableSize)));
        }

        double getBucketRel(double relInput) const {
            return std::max(relInput + (1 - relInput) * std::log(1 - relInput),
                            slope * relInput);;
        }
    };

    class SkewBucketer : public Bucketer {
    public:
        double getBucketRel(double relInput) const {
            if (relInput < 0.6) {
                return relInput / 0.6 * 0.3;
            } else {
                return 0.3 + (relInput - 0.6) / 0.4 * 0.7;
            }
        }
    };
}