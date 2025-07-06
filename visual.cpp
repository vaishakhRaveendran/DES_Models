#include "visual.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <sstream>

void ScheduleVisualizer::visualizeSchedule(
    const std::unordered_map<NameType, std::tuple<int, int, int>>& combinedSchedule,
    int maxCores,
    int hyperperiod,
    const std::vector<std::vector<double>>& powerProfile,
    double chipTDP) {

    // Sort tasks by start time
    std::vector<std::pair<NameType, std::tuple<int, int, int>>> sortedTasks(
        combinedSchedule.begin(), combinedSchedule.end());
    std::sort(sortedTasks.begin(), sortedTasks.end(),
             [](const auto& a, const auto& b) {
                 return std::get<1>(a.second) < std::get<1>(b.second);
             });

    // Find the maximum finish time
    int maxFinishTime = 0;
    for (const auto& [task, details] : combinedSchedule) {
        maxFinishTime = std::max(maxFinishTime, std::get<2>(details));
    }

    // Scaling factor for wider boxes
    const int SCALE = 7;  // Width of each time unit
    const int scaledMaxFinish = maxFinishTime * SCALE;

    // Color palette for tasks
    const std::vector<int> bgColors = {
        41, 42, 43, 44, 45, 46, 47, 100, 101, 102, 103, 104, 105, 106
    };

    // Assign colors to tasks
    std::map<NameType, int> taskColors;
    std::set<NameType> uniqueTasks;
    for (const auto& [task, _] : combinedSchedule) {
        uniqueTasks.insert(task);
    }

    int colorIndex = 0;
    for (const auto& task : uniqueTasks) {
        taskColors[task] = bgColors[colorIndex % bgColors.size()];
        colorIndex++;
    }

    // Display Gantt chart header
    std::cout << "\n\033[1;36m[DEBUG] Task Schedule Gantt Chart:\033[0m" << std::endl;
    std::cout << "\033[1;33m=========================================\033[0m" << std::endl;

    // Display time scale
    std::cout << "\033[1;37mTime (ms):\033[0m\n";
    std::cout << "      ";
    for (int t = 0; t <= maxFinishTime; t += 2) {
        std::cout << std::setw(SCALE) << t << std::string(SCALE, ' ');
    }
    std::cout << std::endl;

    std::cout << "          ";
    for (int t = 0; t <= scaledMaxFinish + maxCores; ++t) {
        std::cout << "\033[90m─\033[0m";
    }
    std::cout << std::endl;

    // Helper function for text color
    auto getTextColor = [](int bgColor) -> int {
        return (bgColor == 43 || bgColor == 47 || bgColor == 102 || bgColor == 103 || bgColor == 106) ? 30 : 37;
    };

    // Create Gantt chart
    for (int core = 0; core < maxCores; ++core) {
        std::cout << "\033[1;37mCore " << core << ":\033[0m   ";

        std::vector<std::tuple<NameType, int, int>> coreTasks;
        for (const auto& [task, details] : combinedSchedule) {
            if (std::get<0>(details) == core) {
                coreTasks.push_back({task, std::get<1>(details), std::get<2>(details)});
            }
        }

        std::sort(coreTasks.begin(), coreTasks.end(),
                [](const auto& a, const auto& b) {
                    return std::get<1>(a) < std::get<1>(b);
                });

        int currentPos = 0;
        for (const auto& [task, start, finish] : coreTasks) {
            int startPos = start * SCALE;
            while (currentPos < startPos) {
                std::cout << " ";
                currentPos++;
            }

            int bgColor = taskColors[task];
            int fgColor = getTextColor(bgColor);
            int length = (finish - start) * SCALE;

            std::string taskLabel = task;
            if (taskLabel.length() > length - 2) {
                taskLabel = length > 3 ? taskLabel.substr(0, length - 3) + "." : taskLabel.substr(0, 1);
            }

            std::cout << "\033[" << fgColor << ";" << bgColor << "m" << "│";
            currentPos++;

            int padding = (length - 2 - taskLabel.length()) / 2;
            for (int i = 0; i < padding; i++) {
                std::cout << " ";
                currentPos++;
            }

            std::cout << taskLabel;
            currentPos += taskLabel.length();

            for (int i = 0; i < length - 2 - taskLabel.length() - padding; i++) {
                std::cout << " ";
                currentPos++;
            }

            std::cout << "│\033[0m";
            currentPos++;
        }

        std::cout << std::endl << "          ";
        int lastFinish = coreTasks.empty() ? 0 : std::get<2>(coreTasks.back());
        for (int t = 0; t < lastFinish * SCALE + 2; ++t) {
            std::cout << " ";
        }
        std::cout << std::endl;
    }

    // Display legend
    std::cout << "\033[1;33m=========================================\033[0m" << std::endl;
    std::cout << "\033[1;37mLegend:\033[0m" << std::endl;

    std::map<int, std::vector<NameType>> tasksByCore;
    for (const auto& [task, details] : combinedSchedule) {
        int core = std::get<0>(details);
        if (std::find(tasksByCore[core].begin(), tasksByCore[core].end(), task) == tasksByCore[core].end()) {
            tasksByCore[core].push_back(task);
        }
    }

    for (int core = 0; core < maxCores; ++core) {
        std::cout << "\033[1;37mCore " << core << " Tasks:\033[0m" << std::endl;

        for (const auto& task : tasksByCore[core]) {
            std::vector<std::pair<int, int>> instances;
            for (const auto& [t, details] : combinedSchedule) {
                if (t == task && std::get<0>(details) == core) {
                    instances.push_back({std::get<1>(details), std::get<2>(details)});
                }
            }

            std::sort(instances.begin(), instances.end());

            int bgColor = taskColors[task];
            int fgColor = getTextColor(bgColor);

            std::cout << "  \033[" << fgColor << ";" << bgColor << "m" << " " << task << " " << "\033[0m" << " - ";

            for (size_t i = 0; i < instances.size(); ++i) {
                std::cout << "Start: " << instances[i].first
                          << ", Finish: " << instances[i].second
                          << ", Duration: " << instances[i].second - instances[i].first;
                if (i < instances.size() - 1) std::cout << " | ";
            }
            std::cout << std::endl;
        }

        if (core < maxCores - 1 && !tasksByCore[core].empty()) {
            std::cout << "  --------------------------------" << std::endl;
        }
    }

    // Display power values at the end
    std::cout << "\n\033[1;37mPower Values:\033[0m" << std::endl;
    std::cout << "Time: ";
    for (int t = 0; t <= maxFinishTime; t++) {
        std::cout << std::setw(7) << t << " ";
    }
    std::cout << "\nPower: ";
    for (size_t t = 0; t < powerProfile.size(); t++) {
        double totalPower = 0.0;
        for (const auto& corePower : powerProfile[t]) {
            totalPower += corePower;
        }
        std::cout << std::fixed << std::setprecision(1) << std::setw(7) << totalPower << " ";
    }
    std::cout << "\nTDP:   ";
    for (int t = 0; t <= maxFinishTime; t++) {
        std::cout << std::setw(7) << chipTDP << " ";
    }
    std::cout << "\n";

    std::cout << "\033[1;33m=========================================\033[0m" << std::endl;

    // Display execution summary
    int makespan = 0;
    for (const auto& [_, details] : combinedSchedule) {
        makespan = std::max(makespan, std::get<2>(details));
    }

    std::cout << "\nExecution Summary:" << std::endl;
    std::cout << "==================" << std::endl;
    std::cout << "Total Execution Time (Makespan): " << makespan << " time units" << std::endl;
    std::cout << "Maximum Cores Available: " << maxCores << std::endl;
    std::cout << "Number of Tasks: " << combinedSchedule.size() << std::endl;
    std::cout << "Hyperperiod: " << hyperperiod << std::endl;
    std::cout << "Chip TDP: " << chipTDP << "W" << std::endl;
}
