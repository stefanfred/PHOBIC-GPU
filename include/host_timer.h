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
    void addLabelManual(std::string name, double time);
    void printLabels(double div);
    double elapsed() const;
};