#include <numeric>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

namespace phobicgpu {

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
        double c;

        void init(double lambda, double expectedTableSize) {
            c = 0.2 * lambda / std::sqrt(expectedTableSize);
        }

        double getBucketRel(double relInput) const {
            return (relInput + (1.0 - relInput) * std::log(1.0 - relInput)) * (1.0 - c) + c * relInput;
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