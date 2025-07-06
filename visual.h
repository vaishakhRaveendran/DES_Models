#ifndef VISUAL_H
#define VISUAL_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include "applications.h"
#include <unordered_map>
#include <tuple>

using NameType = std::string;

class ScheduleVisualizer {
public:
    static void visualizeSchedule(
        const std::unordered_map<NameType, std::tuple<int, int, int>>& combinedSchedule,
        int maxCores,
        int hyperperiod,
        const std::vector<std::vector<double>>& powerProfile,
        double chipTDP
    );
};

#endif // VISUAL_H 