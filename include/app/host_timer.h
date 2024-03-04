#pragma once

#include <chrono>
#include <string>
#include <vector>

class HostTimer {
    struct Label {
        double time;
        std::string name;
    };
private:
    using clock = std::chrono::high_resolution_clock;
    std::vector<Label> labels;
    clock::time_point start;

public:
    HostTimer();

    void reset();

    void addLabel(std::string name);

    void addLabelManually(std::string name, double time);

    std::string getResultStyle(double div) const;

    void printLabels(double div) const;

    double elapsed() const;
};