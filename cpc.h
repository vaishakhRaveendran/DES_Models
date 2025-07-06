#ifndef CPC_H
#define CPC_H
#include "applications.h"
#include "taskExecutionModels.h"
#include "dags.h"
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <climits>
#include <queue>
#include <iostream>
#include <set>
#include <map>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iomanip>

class CPCModel {
public:
    // Static map to store task-specific peak power values
    static std::unordered_map<NameType, double> taskPeakPowerMap;

    // Main function to construct the CPC model
    static void constructCPCModel();

    // Function to find the critical path
    static std::vector<NameType> findCriticalPath(Applications* app);

    // Function to identify capacity providers
    static std::vector<std::vector<NameType>> identifyProviders(Applications* app,
                                                               const std::vector<NameType>& criticalPath);

    // Function to identify capacity consumers
    static std::tuple<std::unordered_map<NameType, std::vector<NameType>>,
                     std::unordered_map<NameType, std::vector<NameType>>>
    identifyConsumers(Applications* app,
                     const std::vector<NameType>& criticalPath,
                     const std::vector<std::vector<NameType>>& capacityProviders);

    // Function to assign priorities to tasks
    static std::unordered_map<NameType, int> assignPriorities(
        Applications* app,
        const std::vector<NameType>& criticalPath,
        const std::vector<std::vector<NameType>>& capacityProviders,
        const std::unordered_map<NameType, std::vector<NameType>>& directConsumers);

    // Helper function to display the CPC model
    static void displayCPCModel(
        const std::vector<NameType>& criticalPath,
        const std::vector<std::vector<NameType>>& capacityProviders,
        const std::unordered_map<NameType, std::vector<NameType>>& directConsumers,
        const std::unordered_map<NameType, std::vector<NameType>>& indirectConsumers,
        const std::unordered_map<NameType, int> &priorities);


    // New method to construct the Power-Aware Workload Distribution Model
    static std::tuple<std::unordered_map<NameType, std::tuple<int, int, int>>,
           std::vector<std::vector<NameType>>,
           std::vector<std::vector<double>>>
    constructPWDM(Applications* app,
                 const std::vector<NameType>& criticalPath,
                 const std::unordered_map<NameType, int>& priorities,
                 const std::unordered_map<NameType, double>& peakPowerMap,
                 int numCores,
                 double maxChipTDP,
                 int startTime);

    //Helper function for combining two applications..
    static std::tuple<std::vector<std::vector<NameType>>,
           std::unordered_map<NameType, std::tuple<int, int, int>>,
           std::vector<std::vector<double>>,
           int>
    accumulatePWDMs(
    const std::unordered_map<NameType, std::tuple<int, int, int>>& schedule1,
    const std::vector<std::vector<NameType>>& wdm1,
    const std::vector<std::vector<double>>& ppd1,
    const std::unordered_map<NameType, std::tuple<int, int, int>>& schedule2,
    const std::vector<std::vector<NameType>>& wdm2,
    const std::vector<std::vector<double>>& ppd2,
    int maxCores,
    double chipTDP,int relativeDeadline);

    static  std::tuple<std::unordered_map<NameType, std::tuple<int, int, int>>,
           std::vector<std::vector<NameType>>,
           std::vector<std::vector<double>>,
           int>
    combineApplicationSchedules(
    const std::unordered_map<NameType, std::tuple<int, int, int>>& schedule1,
    const std::vector<std::vector<NameType>>& wdm1,
    const std::vector<std::vector<double>>& ppd1,
    const std::unordered_map<NameType, std::tuple<int, int, int>>& schedule2,
    const std::vector<std::vector<NameType>>& wdm2,
    const std::vector<std::vector<double>>& ppd2,
    int maxCores,
    double chipTDP,int relativeDeadline);


/////////////////////////////////////////////////////////////////////////////////////////


// Offline Core Assignment for Multi-DAG applications
    static std::tuple<std::unordered_map<NameType, int>,
               std::unordered_map<NameType, std::tuple<int, int, int>>,
               std::vector<std::vector<NameType>>,
               std::vector<std::vector<double>>,
               int,double>
    offlineCoreAssignment(int maxCores, double chipTDP,int mode);

    // Dynamic Voltage and Frequency Scaling function
    static std::unordered_map<NameType, std::tuple<int, int, int>> applyDVFS(
        const std::unordered_map<NameType, std::tuple<int, int, int>>& combinedSchedule,
        std::vector<std::vector<NameType>>& combinedWdm,
        std::vector<std::vector<double>>& combinedPpd,
        double chipTDP,
        int maxCores);

    // Updated response time calculation function
    static double calculateResponseTime(Applications* app,int numCores, int blockingInterval);

    static double calculateAveragePower(const std::vector<std::vector<double>>& ppd);



//////////////////////////////////////////////////////////////////////////////////////////////

private:
    // Helper function to check if a task has a single predecessor
    static bool hasSinglePredecessor(Applications* app, NameType task, NameType expectedPredecessor);

    // Helper function to find ancestors of a task
    static std::unordered_set<NameType> findAncestors(Applications* app, NameType task);

    // Helper function to find concurrent tasks
    static std::unordered_set<NameType> findConcurrentTasks(Applications* app, NameType task);

    // Helper functions for priority assignment
    static NameType findLongestPathStart(Applications* app, const std::vector<NameType>& tasks);
    static int countPredecessors(Applications* app, NameType task);
    static std::vector<NameType> findLongestPathFrom(
        Applications* app, NameType startTask, const std::vector<NameType>& allowedTasks);
    static std::vector<NameType> findLocalCriticalPath(
        Applications* app, const std::vector<NameType>& subgraph, NameType startTask);
    static std::vector<std::vector<NameType>> identifyLocalProviders(
        Applications* app, const std::vector<NameType>& localCriticalPath,
        const std::vector<NameType>& subgraph);
    static std::tuple<std::unordered_map<NameType, std::vector<NameType>>,
                     std::unordered_map<NameType, std::vector<NameType>>>
    identifyLocalConsumers(
        Applications* app, const std::vector<NameType>& localCriticalPath,
        const std::vector<std::vector<NameType>>& localProviders,
        const std::vector<NameType>& subgraph);
};

#endif // CPC_H
