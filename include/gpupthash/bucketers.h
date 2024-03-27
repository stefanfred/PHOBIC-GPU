#include <spline.h>
#include <numeric>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

namespace gpupthash {

    class Bucketer {
    public:
        virtual double getBucketRel(double relInput) const = 0;
    };

    class ConstantBucketer : public Bucketer {
    public:
        double getBucketRel(double relInput) const {
            return relInput;
        }
    };

    class OptBucketer : public Bucketer {
    public:
        double getBucketRel(double relInput) const {
            const double lambda = 10; const uint64_t table_size=2048;
            constexpr double local_collision_factor = 0.5;
            double slope;
            slope = std::max(
                    0.05, std::min(1.0, local_collision_factor * lambda / std::sqrt((double)table_size)));
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

    class CSVBucketer : public Bucketer {
    private:
        tk::spline splineF;
    public:
        CSVBucketer(std::string path) {
            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "Error opening the file." << std::endl;
            }

            std::vector<double> x;
            std::vector<double> y;

            std::string line;
            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::string cell;


                // Read the first column
                if (std::getline(ss, cell, ',')) {
                    x.push_back(std::stod(cell));
                } else {
                    std::cerr << "Error reading the first column." << std::endl;
                }

                // Read the second column
                if (std::getline(ss, cell, ',')) {
                    y.push_back(std::stod(cell));
                } else {
                    std::cerr << "Error reading the second column." << std::endl;
                }
            }
            splineF = tk::spline(x, y);
        }

        double getBucketRel(double relInput) const {
            if (relInput > 0.9999) {
                return 1.0;
            }
            if (relInput < 0.0001) {
                return 0.0;
            }
            return splineF(relInput);
        }
    };
}