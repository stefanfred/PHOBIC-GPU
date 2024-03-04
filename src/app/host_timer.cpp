#include "app/host_timer.h"
#include <iostream>

HostTimer::HostTimer() {
    reset();
}

void HostTimer::reset() {
    start = clock::now();
}

void HostTimer::addLabel(std::string name) {
    labels.push_back({elapsed(), name});
}

void HostTimer::addLabelManually(std::string name, double time) {
    labels.push_back({time, name});
}

void HostTimer::printLabels(double div) const {
    double last = 0;
    for (const Label &l: labels) {
        double now = l.time;
        double diff = now - last;
        std::cout << diff / div << " " << now / div << " " << l.name << std::endl;
        last = now;
    }
}

double HostTimer::elapsed() const {
    auto end = clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count() * 1000000000.0;
}

std::string HostTimer::getResultStyle(double div) const {
    double last = 0;
    std::string res;
    for (const Label &l: labels) {
        double now = l.time;
        double diff = now - last;
        res += l.name + "=" + std::to_string(diff / div) + " ";
        last = now;
    }
    return res;
}
