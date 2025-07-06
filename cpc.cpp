#include "cpc.h"
#include <random>
#include <cfloat>
#include <numeric>
#include "visual.h"

// Static map to store task-specific peak power values
std::unordered_map<NameType, double> CPCModel::taskPeakPowerMap = {
    {"v1", 14.0}, {"v2", 19.0}, {"v3", 20.0}, {"v4", 9.0}, {"v5", 24.0},
    {"v6", 10.0}, {"v7", 13.0}, {"v8", 17.0}, {"v9", 11.0}, {"v10", 15.0},
    {"v11", 18.0}, {"v12", 7.0}, {"v13", 16.0}, {"v14", 23.0}, {"v15", 8.0},
    {"v16", 20.0}, {"v17", 12.0}, {"v18", 25.0}, {"v19", 6.0}, {"v20", 22.0},

    {"u1", 15.0}, {"u2", 11.0}, {"u3", 8.0}, {"u4", 19.0}, {"u5", 17.0},
    {"u6", 10.0}, {"u7", 13.0}, {"u8", 22.0}, {"u9", 16.0}, {"u10", 9.0},
    {"u11", 14.0}, {"u12", 7.0}, {"u13", 12.0}, {"u14", 25.0}, {"u15", 6.0},
    {"u16", 21.0}, {"u17", 23.0}, {"u18", 10.0}, {"u19", 5.0}, {"u20", 24.0},

    {"x1", 11.0}, {"x2", 6.0}, {"x3", 18.0}, {"x4", 7.0}, {"x5", 15.0},
    {"x6", 21.0}, {"x7", 12.0}, {"x8", 13.0}, {"x9", 20.0}, {"x10", 10.0},
    {"x11", 16.0}, {"x12", 8.0}, {"x13", 17.0}, {"x14", 14.0}, {"x15", 22.0},
    {"x16", 9.0}, {"x17", 25.0}, {"x18", 19.0}, {"x19", 5.0}, {"x20", 23.0},

    {"w1", 17.0}, {"w2", 8.0}, {"w3", 20.0}, {"w4", 9.0}, {"w5", 24.0},
    {"w6", 14.0}, {"w7", 11.0}, {"w8", 16.0}, {"w9", 6.0}, {"w10", 13.0},
    {"w11", 25.0}, {"w12", 7.0}, {"w13", 12.0}, {"w14", 23.0}, {"w15", 10.0},
    {"w16", 15.0}, {"w17", 5.0}, {"w18", 19.0}, {"w19", 18.0}, {"w20", 21.0},

    {"t1", 9.0}, {"t2", 12.0}, {"t3", 14.0}, {"t4", 25.0}, {"t5", 7.0},
    {"t6", 15.0}, {"t7", 20.0}, {"t8", 8.0}, {"t9", 18.0}, {"t10", 11.0},
    {"t11", 23.0}, {"t12", 5.0}, {"t13", 17.0}, {"t14", 22.0}, {"t15", 6.0},
    {"t16", 21.0}, {"t17", 13.0}, {"t18", 10.0}, {"t19", 16.0}, {"t20", 19.0}
};

// Static method to handle the entire CPC model construction process with multiple scheduling strategies
void CPCModel::constructCPCModel() {
    std::cout << "[DEBUG] Constructing CPC Model..." << std::endl;
    int numCores = 9;        // Number of cores
    double maxChipTDP = 300.0; // Maximum chip TDP

    // Create vectors to store results for each strategy
    std::vector<std::unordered_map<NameType, int>> allCoreAssignments;
    std::vector<std::unordered_map<NameType, std::tuple<int, int, int>>> allSchedules;
    std::vector<std::vector<std::vector<NameType>>> allWdms;
    std::vector<std::vector<std::vector<double>>> allPpds;
    std::vector<int> allBlockingIntervals;
    std::vector<std::unordered_map<NameType, std::tuple<int, int, int>>> allDvfsSchedules;

    // Updated strategy names - changed index 1 from "Maximum Critical Path Length" to "First In First Out (FIFO)"
    std::vector<std::string> strategyNames = {
        "Minimum Deadline",
        "First In First Out (FIFO)",
        "Least Laxity (Deadline - Critical Path Length)",
        "Maximum Average Power",
        "Urgency Factor (Avg Peak Power / Laxity)"
    };

    // Run all five strategies
    for (int mode = 0; mode < 5; mode++) {
        std::cout << "\n\n====================================================" << std::endl;
        std::cout << "RUNNING STRATEGY " << mode << ": " << strategyNames[mode] << std::endl;
        std::cout << "====================================================" << std::endl;

        auto [coreAssignments, combinedSchedule, combinedWdm, combinedPpd, blockingInterval,successRate] =
            offlineCoreAssignment(numCores, maxChipTDP, mode);

        // Store results for this strategy
        allCoreAssignments.push_back(coreAssignments);
        allSchedules.push_back(combinedSchedule);
        allWdms.push_back(combinedWdm);
        allPpds.push_back(combinedPpd);
        allBlockingIntervals.push_back(blockingInterval);

        // Visualize initial schedule
        std::cout << "\n[DEBUG] Visualizing initial schedule for strategy " << strategyNames[mode] << "..." << std::endl;
        ScheduleVisualizer::visualizeSchedule(combinedSchedule, numCores, 0, combinedPpd, maxChipTDP);

        // Apply DVFS and get the modified schedule
        auto dvfsSchedule = applyDVFS(combinedSchedule, combinedWdm, combinedPpd, maxChipTDP, numCores);
        allDvfsSchedules.push_back(dvfsSchedule);

        // Visualize schedule after DVFS
        std::cout << "\n[DEBUG] Visualizing schedule after DVFS for strategy " << strategyNames[mode] << "..." << std::endl;
        ScheduleVisualizer::visualizeSchedule(dvfsSchedule, numCores, 0, combinedPpd, maxChipTDP);

        // Print comparison of schedules
        std::cout << "\nSchedule Comparison for Strategy " << strategyNames[mode] << ":" << std::endl;
        std::cout << "===================" << std::endl;
        std::cout << "Task\t|\t\t\tOriginal\t\t|\t\t\tAfter DVFS" << std::endl;
        std::cout << "\t|\tCore\t|\tStart\t|\tFinish\t|\tCore\t|\tStart\t|\tFinish" << std::endl;
        std::cout << "--------------------------------------------------------" << std::endl;

        for (const auto& [taskId, originalDetails] : combinedSchedule) {
            auto [origCore, origStart, origFinish] = originalDetails;
            auto [newCore, newStart, newFinish] = dvfsSchedule[taskId];
            std::cout << taskId << "\t|\t"
                      << origCore << "\t|\t" << origStart << "\t|\t" << origFinish << "\t|\t"
                      << newCore << "\t|\t" << newStart << "\t|\t" << newFinish << std::endl;
        }

        std::string userInput;
        do {
            std::cout << "Type 'next' to continue to the next strategy (or 'skip' to skip this pause): ";
            std::getline(std::cin, userInput);
        } while (userInput != "next" && userInput != "skip");
    }

    // Final comparison between all strategies
    std::cout << "\n\n====================================================" << std::endl;
    std::cout << "FINAL COMPARISON OF ALL STRATEGIES" << std::endl;
    std::cout << "====================================================" << std::endl;

    // Calculate and display metrics for each strategy
    for (int mode = 0; mode < 5; mode++) {
        // Calculate makespan (maximum finish time)
        int makespan = 0;
        int totalTasks = 0;
        double avgResponseTime = 0.0;

        for (const auto& [taskId, details] : allSchedules[mode]) {
            makespan = std::max(makespan, std::get<2>(details));
            totalTasks++;
            // Calculate response time (finish - start)
            int start = std::get<1>(details);
            int finish = std::get<2>(details);
            avgResponseTime += (finish - start);
        }

        if (totalTasks > 0) {
            avgResponseTime /= totalTasks;
        }

        // Calculate makespan after DVFS
        int dvfsMakespan = 0;
        double dvfsAvgResponseTime = 0.0;

        for (const auto& [taskId, details] : allDvfsSchedules[mode]) {
            dvfsMakespan = std::max(dvfsMakespan, std::get<2>(details));
            // Calculate response time (finish - start)
            int start = std::get<1>(details);
            int finish = std::get<2>(details);
            dvfsAvgResponseTime += (finish - start);
        }

        if (totalTasks > 0) {
            dvfsAvgResponseTime /= totalTasks;
        }

        std::cout << "\nStrategy " << mode << ": " << strategyNames[mode] << std::endl;
        std::cout << "------------------------------------" << std::endl;
        std::cout << "Number of tasks: " << totalTasks << std::endl;
        std::cout << "Makespan (before DVFS): " << makespan << std::endl;
        std::cout << "Makespan (after DVFS): " << dvfsMakespan << std::endl;
        std::cout << "Average response time (before DVFS): " << avgResponseTime << std::endl;
        std::cout << "Average response time (after DVFS): " << dvfsAvgResponseTime << std::endl;
        std::cout << "Blocking interval: " << allBlockingIntervals[mode] << std::endl;

        // Calculate average cores used
        double avgCores = 0.0;
        for (const auto& [appName, cores] : allCoreAssignments[mode]) {
            avgCores += cores;
        }
        if (!allCoreAssignments[mode].empty()) {
            avgCores /= allCoreAssignments[mode].size();
        }
        std::cout << "Average cores used: " << avgCores << std::endl;

        // Calculate average power usage before and after DVFS
        double avgPowerBeforeDVFS = calculateAveragePower(allPpds[mode]);
        double avgPowerAfterDVFS = calculateAveragePower(allPpds[mode]); // Note: DVFS would modify the power profile, but we're using the same array for simplicity

        std::cout << "Average power usage (before DVFS): " << avgPowerBeforeDVFS << "/" << maxChipTDP << " ("
                  << (avgPowerBeforeDVFS / maxChipTDP) * 100.0 << "%)" << std::endl;
        std::cout << "Average power usage (after DVFS): " << avgPowerAfterDVFS << "/" << maxChipTDP << " ("
                  << (avgPowerAfterDVFS / maxChipTDP) * 100.0 << "%)" << std::endl;

        // Add additional metrics specific to each strategy
        switch (mode) {
            case 1: // First In First Out (FIFO)
                std::cout << "Strategy-specific metrics:" << std::endl;
                std::cout << "  This strategy processes DAGs in the order they arrive (FIFO scheduling)" << std::endl;
                break;

            case 2: // Least Laxity
                std::cout << "Strategy-specific metrics:" << std::endl;
                std::cout << "  This strategy prioritizes DAGs with tighter deadlines relative to their critical paths" << std::endl;
                break;

            case 3: // Maximum Average Power
                std::cout << "Strategy-specific metrics:" << std::endl;
                std::cout << "  This strategy prioritizes DAGs with higher average power consumption" << std::endl;
                break;

            case 4: // Urgency Factor
                std::cout << "Strategy-specific metrics:" << std::endl;
                std::cout << "  This strategy balances power needs with timing constraints through the urgency factor" << std::endl;
                break;
        }
    }

    // Final analysis comparing all strategies
    std::cout << "\n\n====================================================" << std::endl;
    std::cout << "STRATEGY COMPARISON SUMMARY" << std::endl;
    std::cout << "====================================================" << std::endl;

    // Find best strategy for different metrics
    int bestMakespanStrategy = 0;
    int bestPowerStrategy = 0;
    int bestResponseTimeStrategy = 0;
    int bestCoreUtilizationStrategy = 0;

    int minMakespan = INT_MAX;
    double minPower = DBL_MAX;
    double minResponseTime = DBL_MAX;
    double bestCoreUtil = 0.0;

    for (int mode = 0; mode < 5; mode++) {
        // Find maximum finish time for makespan
        int makespan = 0;
        for (const auto& [taskId, details] : allSchedules[mode]) {
            makespan = std::max(makespan, std::get<2>(details));
        }

        // Calculate average response time
        double avgResponseTime = 0.0;
        int totalTasks = allSchedules[mode].size();
        for (const auto& [taskId, details] : allSchedules[mode]) {
            int start = std::get<1>(details);
            int finish = std::get<2>(details);
            avgResponseTime += (finish - start);
        }
        if (totalTasks > 0) {
            avgResponseTime /= totalTasks;
        }

        // Calculate average power
        double avgPower = calculateAveragePower(allPpds[mode]);

        // Calculate core utilization (average cores assigned)
        double avgCores = 0.0;
        for (const auto& [appName, cores] : allCoreAssignments[mode]) {
            avgCores += cores;
        }
        if (!allCoreAssignments[mode].empty()) {
            avgCores /= allCoreAssignments[mode].size();
        }
        double coreUtilization = avgCores / numCores;

        // Update best strategies
        if (makespan < minMakespan) {
            minMakespan = makespan;
            bestMakespanStrategy = mode;
        }

        if (avgPower < minPower) {
            minPower = avgPower;
            bestPowerStrategy = mode;
        }

        if (avgResponseTime < minResponseTime) {
            minResponseTime = avgResponseTime;
            bestResponseTimeStrategy = mode;
        }

        if (coreUtilization > bestCoreUtil) {
            bestCoreUtil = coreUtilization;
            bestCoreUtilizationStrategy = mode;
        }
    }

    // Print best strategies for each metric
    std::cout << "Best strategy for minimizing makespan: " << strategyNames[bestMakespanStrategy] << std::endl;
    std::cout << "Best strategy for minimizing power consumption: " << strategyNames[bestPowerStrategy] << std::endl;
    std::cout << "Best strategy for minimizing response time: " << strategyNames[bestResponseTimeStrategy] << std::endl;
    std::cout << "Best strategy for core utilization: " << strategyNames[bestCoreUtilizationStrategy] << std::endl;

    // Power-performance trade-off analysis
    std::cout << "\n====================================================" << std::endl;
    std::cout << "POWER-PERFORMANCE TRADE-OFF ANALYSIS" << std::endl;
    std::cout << "====================================================" << std::endl;

    // Find strategies that balance power and performance
    int bestBalancedStrategy = 0;
    double bestBalanceScore = DBL_MAX;

    for (int mode = 0; mode < 5; mode++) {
        // Calculate makespan and power metrics
        int makespan = 0;
        for (const auto& [taskId, details] : allSchedules[mode]) {
            makespan = std::max(makespan, std::get<2>(details));
        }

        double avgPower = calculateAveragePower(allPpds[mode]);

        // Normalize metrics (lower is better for both)
        double normalizedMakespan = static_cast<double>(makespan) / minMakespan;
        double normalizedPower = avgPower / minPower;

        // Combined score (equal weight to power and performance)
        double balanceScore = normalizedMakespan + normalizedPower;

        if (balanceScore < bestBalanceScore) {
            bestBalanceScore = balanceScore;
            bestBalancedStrategy = mode;
        }

        std::cout << "Strategy " << mode << " (" << strategyNames[mode] << ") - Balance Score: " << balanceScore
                  << " (Makespan: " << makespan << ", Power: " << avgPower << ")" << std::endl;
    }

    std::cout << "\nBest balanced strategy (power-performance trade-off): "
              << strategyNames[bestBalancedStrategy] << std::endl;

    std::string userInput;
    do {
        std::cout << "\nType 'exit' to finish: ";
        std::getline(std::cin, userInput);
    } while (userInput != "exit");

    return;
}

// Helper function to calculate average power consumption
double CPCModel::calculateAveragePower(const std::vector<std::vector<double>>& ppd) {
    double totalPower = 0.0;
    int timeSteps = 0;

    for (const auto& timeStep : ppd) {
        double stepPower = 0.0;
        for (const auto& corePower : timeStep) {
            stepPower += corePower;
        }
        totalPower += stepPower;
        timeSteps++;
    }

    return (timeSteps > 0) ? (totalPower / timeSteps) : 0.0;
}
// Static method to find the critical path
std::vector<NameType> CPCModel::findCriticalPath(Applications* app) {
    std::cout << "[DEBUG] Finding Critical Path..." << std::endl;
    std::unordered_map<NameType, int> longestPath;
    std::unordered_map<NameType, NameType> predecessor;
    NameType source = app->startState;
    std::vector<NameType> criticalPath;

    // Initialize all task distances to INT_MIN except source
    for (const auto& [taskId, _] : app->tasks) {
        longestPath[taskId] = (taskId == source) ? 0 : INT_MIN;
    }

    std::vector<NameType> order;
    TaskExecutionModels::getTopologicalOrder(app, &order);

    // Process tasks in topological order
    for (const auto& taskId : order) {
        // Skip if we haven't reached this task yet (not source and no predecessor)
        if (longestPath[taskId] == INT_MIN) continue;

        // For each outgoing transition
        for (const auto& transition : app->tasks[taskId]->transitions) {
            NameType nextTask = transition.targetState;
            // Add current task's execution time to path
            int newLength = longestPath[taskId] + app->tasks[taskId]->executionTime;
            if (newLength > longestPath[nextTask]) {
                longestPath[nextTask] = newLength;
                predecessor[nextTask] = taskId;
            }
        }
    }

    // Find sink with maximum path length
    NameType sink;
    int maxLength = INT_MIN;
    for (const auto& [taskId, length] : longestPath) {
        if (app->tasks[taskId]->transitions.empty() && length > maxLength) {
            maxLength = length;
            sink = taskId;
        }
    }

    // Check if we found a valid critical path
    if (maxLength == INT_MIN) {
        std::cout << "[ERROR] No valid sink found or critical path doesn't exist." << std::endl;
        app->criticalLength = 0; // Set critical length to 0 if no valid path found
        return criticalPath;
    }

    // Store the critical path length in the app object
    app->criticalLength = maxLength;
    std::cout << "[DEBUG] Critical Path Length: " << app->criticalLength << std::endl;

    // Reconstruct the critical path
    NameType current = sink;
    while (!current.empty()) {
        criticalPath.insert(criticalPath.begin(), current);
        if (predecessor.find(current) == predecessor.end()) break;
        current = predecessor[current];
    }

    // Calculate and verify the critical path length by summing execution times
    int verifiedLength = 0;
    std::cout << "[DEBUG] Critical Path Found: ";
    for (const auto& task : criticalPath) {
        std::cout << task << " ";
        verifiedLength += app->tasks[task]->executionTime;
    }
    std::cout << std::endl;

    // Verify the calculated length matches the sum of execution times
    if (verifiedLength != app->criticalLength) {
        std::cout << "[WARNING] Critical path length verification mismatch: "
                  << "Calculated=" << app->criticalLength
                  << ", Verified=" << verifiedLength << std::endl;

        // Update to verified length if there's a discrepancy
        app->criticalLength = verifiedLength;
        std::cout << "[DEBUG] Updated Critical Path Length: " << app->criticalLength << std::endl;
    }

    return criticalPath;
}

// Static method to identify capacity providers based on the algorithm
std::vector<std::vector<NameType>> CPCModel::identifyProviders(Applications* app,
                                                              const std::vector<NameType>& criticalPath) {
    std::cout << "[DEBUG] Identifying Providers using Algorithm..." << std::endl;

    std::vector<std::vector<NameType>> capacityProviders;

    if (criticalPath.empty()) {
        std::cout << "[ERROR] Cannot identify providers: Critical path is empty." << std::endl;
        return capacityProviders;
    }

    // Algorithm Step 1: Identifying capacity providers
    // Following the algorithm: Θ' = ∅; i = 1; k = 1;
    int i = 1;
    int k = 1;

    // V' = V\λ* (All vertices except those in the critical path)
    std::unordered_set<NameType> V_prime;
    for (const auto& [taskId, _] : app->tasks) {
        V_prime.insert(taskId);
    }

    for (const auto& task : criticalPath) {
        V_prime.erase(task);
    }

    // Create Θ' (list of capacity providers)
    // while k <= |λ*| do
    while (k <= static_cast<int>(criticalPath.size())) {
        // θi' = {λ*(k)}; k++;
        std::vector<NameType> theta_i;
        theta_i.push_back(criticalPath[k-1]);
        k++;

        // while pre(λ*(k)) = {λ*(k-1)} do
        while (k <= static_cast<int>(criticalPath.size()) &&
               hasSinglePredecessor(app, criticalPath[k-1], criticalPath[k-2])) {
            // θi' = θi' ∪ {λ*(k)}; k++;
            theta_i.push_back(criticalPath[k-1]);
            k++;
        }

        // Add provider to list
        capacityProviders.push_back(theta_i);
        i++;
    }

    std::cout << "[DEBUG] Providers Identified: " << capacityProviders.size() << " provider(s)" << std::endl;
    return capacityProviders;
}

// Helper function to check if a task has only one predecessor and it's the specified one
bool CPCModel::hasSinglePredecessor(Applications* app, NameType task, NameType expectedPredecessor) {
    int predecessorCount = 0;
    bool hasExpectedPredecessor = false;

    for (const auto& [taskId, taskInfo] : app->tasks) {
        for (const auto& transition : taskInfo->transitions) {
            if (transition.targetState == task) {
                predecessorCount++;
                if (taskId == expectedPredecessor) {
                    hasExpectedPredecessor = true;
                }
            }
        }
    }

    return predecessorCount == 1 && hasExpectedPredecessor;
}

// Static method to identify consumers based on the algorithm
std::tuple<std::unordered_map<NameType, std::vector<NameType>>,
          std::unordered_map<NameType, std::vector<NameType>>>
CPCModel::identifyConsumers(Applications* app,
                           const std::vector<NameType>& criticalPath,
                           const std::vector<std::vector<NameType>>& capacityProviders) {
    std::cout << "[DEBUG] Identifying Consumers using Algorithm..." << std::endl;

    std::unordered_map<NameType, std::vector<NameType>> directConsumers;
    std::unordered_map<NameType, std::vector<NameType>> indirectConsumers;

    // Create a set of tasks in the critical path for quick lookup
    std::unordered_set<NameType> criticalPathSet(criticalPath.begin(), criticalPath.end());

    // V' = V\λ* (All vertices except those in the critical path)
    std::unordered_set<NameType> V_prime;
    for (const auto& [taskId, _] : app->tasks) {
        V_prime.insert(taskId);
    }

    for (const auto& task : criticalPath) {
        V_prime.erase(task);
    }

    // Algorithm Step 2: Identifying capacity consumers
    // for i = 1; i < |Θ'|; i++ do
    for (size_t i = 0; i < capacityProviders.size(); i++) {
        // Get the last task of the current provider
        NameType providerLastTask = capacityProviders[i].back();

        // F(θi') = anc(θi+1') ∩ V'
        std::vector<NameType> F_theta_i;

        // If this is not the last provider
        if (i + 1 < capacityProviders.size()) {
            // Find ancestors of the next provider's first task that are in V'
            NameType nextProviderFirstTask = capacityProviders[i+1].front();
            std::unordered_set<NameType> ancestors = findAncestors(app, nextProviderFirstTask);

            for (const auto& ancestor : ancestors) {
            if (V_prime.find(ancestor) != V_prime.end()) {
                F_theta_i.insert(F_theta_i.begin(), ancestor); // Insert at front
            }//Push back would have been optimal
        }

            // These are direct consumers (can delay next provider)
            directConsumers[providerLastTask] = F_theta_i;
        }


        // Update V': V' = V' \ F(θi')
        for (const auto& task : F_theta_i) {
            V_prime.erase(task);
        }

        // G(θi') = ⋃v∈F(θi') {C(v)} ∩ V'
        std::vector<NameType> G_theta_i;
        std::unordered_set<NameType> G_theta_i_set;

        for (const auto& v : F_theta_i) {
            std::unordered_set<NameType> concurrentTasks = findConcurrentTasks(app, v);
            for (const auto& c : concurrentTasks) {
                if (V_prime.find(c) != V_prime.end() && G_theta_i_set.find(c) == G_theta_i_set.end()) {
                    G_theta_i.push_back(c);
                    G_theta_i_set.insert(c);
                }
            }
        }

        // These are indirect consumers
        indirectConsumers[providerLastTask] = G_theta_i;


    }

    std::cout << "[DEBUG] Consumers Identified and Categorized." << std::endl;
    return {directConsumers, indirectConsumers};
}

// Helper function to find all ancestors of a task
std::unordered_set<NameType> CPCModel::findAncestors(Applications* app, NameType task) {
    std::unordered_set<NameType> ancestors;
    std::queue<NameType> queue;
    std::unordered_set<NameType> visited;

    // Find all tasks that have a transition to this task
    for (const auto& [taskId, taskInfo] : app->tasks) {
        for (const auto& transition : taskInfo->transitions) {
            if (transition.targetState == task) {
                queue.push(taskId);
                break;
            }
        }
    }

    while (!queue.empty()) {
        NameType current = queue.front();
        queue.pop();

        if (visited.find(current) != visited.end()) continue;
        visited.insert(current);
        ancestors.insert(current);

        // Add all predecessors of current to queue
        for (const auto& [taskId, taskInfo] : app->tasks) {
            for (const auto& transition : taskInfo->transitions) {
                if (transition.targetState == current) {
                    queue.push(taskId);
                }
            }
        }
    }

    return ancestors;
}

//Helper functions to find concurrent tasks..
std::unordered_set<NameType> CPCModel::findConcurrentTasks(Applications* app, NameType task) {
    std::unordered_set<NameType> concurrentTasks;
    std::unordered_set<NameType> taskAncestors = findAncestors(app, task);

    for (const auto& [taskId, _] : app->tasks) {
        if (taskId == task) continue;
        std::unordered_set<NameType> otherAncestors = findAncestors(app, taskId);

        // Two tasks are concurrent if neither is an ancestor of the other
        if (taskAncestors.find(taskId) == taskAncestors.end() && otherAncestors.find(task) == otherAncestors.end()) {
            concurrentTasks.insert(taskId);
        }
    }
    return concurrentTasks;
}

// Static method to display the CPC model
void CPCModel::displayCPCModel(
    const std::vector<NameType>& criticalPath,
    const std::vector<std::vector<NameType>>& capacityProviders,
    const std::unordered_map<NameType, std::vector<NameType>>& directConsumers,
    const std::unordered_map<NameType, std::vector<NameType>>& indirectConsumers,
    const std::unordered_map<NameType, int>& priorities) {

    std::cout << "\n[DEBUG] Displaying CPC Model...\n";

    // Display Critical Path
    std::cout << "\n=== Critical Path ===\n";
    if (criticalPath.empty()) {
        std::cout << "No critical path found.\n";
    } else {
        for (size_t i = 0; i < criticalPath.size(); ++i) {
            std::cout << criticalPath[i];
            if (i < criticalPath.size() - 1) {
                std::cout << " -> ";
            }
        }
        std::cout << "\n";
    }

    // Display Capacity Providers and their Consumers
    std::cout << "\n=== Capacity Providers and Consumers ===\n";
    for (size_t i = 0; i < capacityProviders.size(); ++i) {
        std::cout << "\nProvider " << i + 1 << ": ";
        for (const auto& task : capacityProviders[i]) {
            std::cout << task << " ";
        }
        std::cout << "\n";

        // Display the provider's last task (the one with consumers)
        NameType lastTask = capacityProviders[i].back();

        // Display direct consumers (can delay next provider)
        std::cout << "  Direct Consumers (F(θi')): ";
        if (directConsumers.count(lastTask) > 0 && !directConsumers.at(lastTask).empty()) {
            for (const auto& consumer : directConsumers.at(lastTask)) {
                std::cout << consumer << " ";
            }
        } else {
            std::cout << "None";
        }
        std::cout << "\n";

        // Display indirect consumers (cannot delay next provider)
        std::cout << "  Indirect Consumers (G(θi')): ";
        if (indirectConsumers.count(lastTask) > 0 && !indirectConsumers.at(lastTask).empty()) {
            for (const auto& consumer : indirectConsumers.at(lastTask)) {
                std::cout << consumer << " ";
            }
        } else {
            std::cout << "None";
        }
        std::cout << "\n";
    }



    // Display the final priority assignments
    std::cout << "\n[DEBUG] Final Task Priorities:" << std::endl;
    std::cout << "==========================" << std::endl;
    std::cout << "Task\t|\tPriority" << std::endl;
    std::cout << "--------------------------" << std::endl;

    // Sort tasks by priority for display
    std::vector<std::pair<NameType, int>> sortedPriorities(priorities.begin(), priorities.end());
    std::sort(sortedPriorities.begin(), sortedPriorities.end(),
              [](const std::pair<NameType, int>& a, const std::pair<NameType, int>& b) {
                  return a.second > b.second; // Sort in descending order of priority
              });

    for (const auto& [task, priority] : sortedPriorities) {
        std::cout << task << "\t|\t" << priority << std::endl;
    }
    std::cout << "==========================" << std::endl;

    std::cout << "[DEBUG] Priority assignment complete." << std::endl;

    std::cout << "[DEBUG] CPC Model Display Complete." << std::endl;
}


// Static method to assign priorities to tasks based on Algorithm 2
std::unordered_map<NameType, int> CPCModel::assignPriorities(
    Applications* app,
    const std::vector<NameType>& criticalPath,
    const std::vector<std::vector<NameType>>& capacityProviders,
    const std::unordered_map<NameType, std::vector<NameType>>& directConsumers) {

    std::cout << "[DEBUG] Assigning task priorities using Algorithm 2..." << std::endl;

    std::unordered_map<NameType, int> priorities;
    // Convert critical path into a set for quick lookup
    std::unordered_set<NameType> criticalPathSet(criticalPath.begin(), criticalPath.end());

    // Flatten capacity providers into a set for quick lookup
    std::unordered_set<NameType> capacityProviderSet;
    for (const auto& provider : capacityProviders) {
        for (const auto& task : provider) {
            capacityProviderSet.insert(task);
        }
    }


    // Sort providers in topological order
    std::vector<NameType> topologicalOrder;
    TaskExecutionModels::getTopologicalOrder(app, &topologicalOrder);


    // Initialize according to the algorithm
    int p = topologicalOrder.size(); // Starting priority value
    int p_max =p;  // Maximum priority

    // Initialize priorities (Assignment Rule 1)
    // For all tasks in critical path and capacity providers, set priority to p
    for (size_t i = 0; i < criticalPath.size(); ++i) {
        priorities[criticalPath[i]] = p;
        // Decrement p
        p = p - 1;

    }


    // Get topological order for all providers
    std::vector<NameType> providerTasks;
    for (const auto& provider : capacityProviders) {
        providerTasks.insert(providerTasks.end(), provider.begin(), provider.end());
    }

    // Filter topological order to only include provider tasks
    std::vector<NameType> providerTopOrder;
    for (const auto& task : topologicalOrder) {
        if (capacityProviderSet.find(task) != capacityProviderSet.end()) {
            providerTopOrder.push_back(task);
        }
    }

    // Assignment Rule 2
    for (const auto& providerTask : providerTopOrder) {
        // Check if this task has direct consumers
        //Explicitly checks whether a mapping for the key (providerTask) exists in directConsumers. The direct consumers are mapped to the last task in the particular provider
        if (directConsumers.find(providerTask) == directConsumers.end() ||
            directConsumers.at(providerTask).empty()) {
            continue;
        }

        std::vector<NameType> F_theta_i = directConsumers.at(providerTask);

        while (!F_theta_i.empty()) {
            // Find the longest local path in F(θi')
            // For simplicity, we'll use a BFS approach to find the path
            NameType v_k = findLongestPathStart(app, F_theta_i);

            // Find λv_k: All tasks in the longest path from v_k
            std::vector<NameType> lambda_v_k = findLongestPathFrom(app, v_k, F_theta_i);

            // Check if there's any task in λv_k with more than one predecessor
            bool hasMultiplePredecessors = false;
            NameType taskWithMultiplePred;
            for (const auto& task : lambda_v_k) {
                int predCount = countPredecessors(app, task);
                if (predCount > 1) {
                    hasMultiplePredecessors = true;
                    taskWithMultiplePred = task;
                    break;
                }
            }

            if (hasMultiplePredecessors) {
                // Construct a CPC model for this subgraph
                // Find critical path within this subgraph
                std::vector<NameType> local_critical_path =
                    findLocalCriticalPath(app, lambda_v_k, taskWithMultiplePred);

                // Identify local providers and consumers
                std::vector<std::vector<NameType>> local_providers =
                    identifyLocalProviders(app, local_critical_path, lambda_v_k);

                auto [local_direct_consumers, _] =
                    identifyLocalConsumers(app, local_critical_path, local_providers, lambda_v_k);

                // Recursively assign priorities to this subgraph
                assignPriorities(app, local_critical_path, local_providers, local_direct_consumers);

                break;
            } else {
                // Assignment Rule 3
                // Assign priorities to all tasks in λv_k
                for (const auto& task : lambda_v_k) {
                    priorities[task] = p;
                }
                p = p - 1;

                // Update F(θi') = F(θi') \ λv_k
                std::vector<NameType> newF;
                for (const auto& task : F_theta_i) {
                    if (std::find(lambda_v_k.begin(), lambda_v_k.end(), task) == lambda_v_k.end()) {
                        newF.push_back(task);
                    }
                }
                F_theta_i = newF;
            }
        }
    }

    // Assign default priority (-1) to any remaining tasks
    for (const auto& [taskId, _] : app->tasks) {
        if (priorities.find(taskId) == priorities.end()) {
            priorities[taskId] = -1;
        }
    }
    std::cout << "Priority Assignment Complete" << std::endl;

    return priorities;
}

// Helper function to find the task that starts the longest path in F(θi')
NameType CPCModel::findLongestPathStart(Applications* app, const std::vector<NameType>& tasks) {
    // For a simple implementation, we'll use the task with the fewest predecessors
    // In a more sophisticated implementation, you might want to actually compute
    // the longest path within this subgraph

    NameType startTask;
    int minPredCount = INT_MAX;

    for (const auto& task : tasks) {
        int predCount = countPredecessors(app, task);
        if (predCount < minPredCount) {
            minPredCount = predCount;
            startTask = task;
        }
    }

    return startTask;
}

// Helper function to count predecessors of a task
int CPCModel::countPredecessors(Applications* app, NameType task) {
    int count = 0;
    for (const auto& [taskId, taskInfo] : app->tasks) {
        for (const auto& transition : taskInfo->transitions) {
            if (transition.targetState == task) {
                count++;
            }
        }
    }
    return count;
}

// Helper function to find the longest path starting from a task
std::vector<NameType> CPCModel::findLongestPathFrom(
    Applications* app, NameType startTask, const std::vector<NameType>& allowedTasks) {

    std::unordered_set<NameType> allowedTaskSet(allowedTasks.begin(), allowedTasks.end());
    std::vector<NameType> path;
    path.push_back(startTask);

    // Simple greedy approach to find a long path
    NameType currentTask = startTask;
    bool pathExtended = true;

    while (pathExtended) {
        pathExtended = false;
        int maxExecTime = -1;
        NameType nextTask;

        // Find successor with maximum execution time
        for (const auto& transition : app->tasks[currentTask]->transitions) {
            NameType successor = transition.targetState;
            if (allowedTaskSet.find(successor) != allowedTaskSet.end() &&
                std::find(path.begin(), path.end(), successor) == path.end()) {

                int execTime = app->tasks[successor]->executionTime;
                if (execTime > maxExecTime) {
                    maxExecTime = execTime;
                    nextTask = successor;
                    pathExtended = true;
                }
            }
        }

        if (pathExtended) {
            path.push_back(nextTask);
            currentTask = nextTask;
        }
    }

    return path;
}

// Helper function to find a critical path within a subgraph
std::vector<NameType> CPCModel::findLocalCriticalPath(
    Applications* app, const std::vector<NameType>& subgraph, NameType startTask) {

    // Create a set for quick lookups
    std::unordered_set<NameType> subgraphSet(subgraph.begin(), subgraph.end());

    // Initialize distances
    std::unordered_map<NameType, int> distances;
    std::unordered_map<NameType, NameType> predecessors;

    for (const auto& task : subgraph) {
        distances[task] = (task == startTask) ? 0 : INT_MIN;
    }

    // Topological sort of subgraph
    std::vector<NameType> topOrder;
    TaskExecutionModels::getTopologicalOrder(app, &topOrder);

    // Filter topological order to only include subgraph tasks
    std::vector<NameType> subgraphTopOrder;
    for (const auto& task : topOrder) {
        if (subgraphSet.find(task) != subgraphSet.end()) {
            subgraphTopOrder.push_back(task);
        }
    }

    // Find longest paths
    for (const auto& task : subgraphTopOrder) {
        if (distances[task] == INT_MIN) continue;

        for (const auto& transition : app->tasks[task]->transitions) {
            NameType nextTask = transition.targetState;
            if (subgraphSet.find(nextTask) != subgraphSet.end()) {
                int newDist = distances[task] + app->tasks[task]->executionTime;
                if (newDist > distances[nextTask]) {
                    distances[nextTask] = newDist;
                    predecessors[nextTask] = task;
                }
            }
        }
    }

    // Find end of critical path (max distance)
    NameType endTask;
    int maxDist = INT_MIN;

    for (const auto& [task, dist] : distances) {
        if (dist > maxDist) {
            maxDist = dist;
            endTask = task;
        }
    }

    // Reconstruct critical path
    std::vector<NameType> criticalPath;
    NameType current = endTask;

    while (!current.empty() && current != startTask) {
        criticalPath.insert(criticalPath.begin(), current);
        if (predecessors.find(current) == predecessors.end()) break;
        current = predecessors[current];
    }

    criticalPath.insert(criticalPath.begin(), startTask);
    return criticalPath;
}

// Helper function to identify local providers
std::vector<std::vector<NameType>> CPCModel::identifyLocalProviders(
    Applications* app, const std::vector<NameType>& localCriticalPath,
    const std::vector<NameType>& subgraph) {

    // Simplified version that treats the entire critical path as one provider
    std::vector<std::vector<NameType>> providers;
    providers.push_back(localCriticalPath);
    return providers;
}

// Helper function to identify local consumers
std::tuple<std::unordered_map<NameType, std::vector<NameType>>,
           std::unordered_map<NameType, std::vector<NameType>>>
CPCModel::identifyLocalConsumers(
    Applications* app, const std::vector<NameType>& localCriticalPath,
    const std::vector<std::vector<NameType>>& localProviders,
    const std::vector<NameType>& subgraph) {

    std::unordered_map<NameType, std::vector<NameType>> directConsumers;
    std::unordered_map<NameType, std::vector<NameType>> indirectConsumers;

    // Create a set of tasks in the local critical path for quick lookup
    std::unordered_set<NameType> criticalPathSet(localCriticalPath.begin(), localCriticalPath.end());

    // Create a set of all tasks in the subgraph for quick lookup
    std::unordered_set<NameType> subgraphSet(subgraph.begin(), subgraph.end());

    // Find consumers of the last provider
    if (!localProviders.empty() && !localProviders.back().empty()) {
        NameType lastProviderTask = localProviders.back().back();

        // Direct consumers are tasks in the subgraph that are not in the critical path
        std::vector<NameType> consumers;
        for (const auto& task : subgraph) {
            if (criticalPathSet.find(task) == criticalPathSet.end()) {
                consumers.push_back(task);
            }
        }

        directConsumers[lastProviderTask] = consumers;
    }

    return {directConsumers, indirectConsumers};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation to add to cpc.cpp
// Updated implementation to add to cpc.cpp
std::tuple<std::unordered_map<NameType, std::tuple<int, int, int>>,
           std::vector<std::vector<NameType>>,
           std::vector<std::vector<double>>>
CPCModel::constructPWDM(
    Applications* app,
    const std::vector<NameType>& criticalPath,
    const std::unordered_map<NameType, int>& priorities,
    const std::unordered_map<NameType, double>& peakPowerMap,
    int numCores,
    double maxChipTDP,
    int startTime) {

    std::cout << "=== PWDM Construction Started ===" << std::endl;
    std::cout << "numCores: " << numCores << ", maxChipTDP: " << maxChipTDP << ", startTime: " << startTime << std::endl;
    std::cout << "Critical path size: " << criticalPath.size() << std::endl;
    std::cout << "Priorities map size: " << priorities.size() << std::endl;
    std::cout << "Peak power map size: " << peakPowerMap.size() << std::endl;

    // Calculate time frame: startTime is max(arrivalTime, minStartTime)
    // End time is arrivalTime + relativeDeadline
    int arrivalTime = app->arrivalTime;
    int relativeDeadline = app->relativeDeadline;
    int frameStart = startTime;  // max(arrivalTime, minStartTime)
    int frameEnd = arrivalTime + relativeDeadline;

    std::cout << "DAG arrivalTime: " << arrivalTime << ", relativeDeadline: " << relativeDeadline << std::endl;
    std::cout << "Time frame: [" << frameStart << ", " << frameEnd << "]" << std::endl;

    if (frameStart >= frameEnd) {
        std::cout << "ERROR: Invalid time frame - start time >= end time!" << std::endl;
        return {{}, {}, {}};  // Return empty results
    }

    // Output: Task schedule mapping (task -> {core, start_time, finish_time})
    std::unordered_map<NameType, std::tuple<int, int, int>> schedule;

    // Set of critical path tasks for quick lookup
    std::unordered_set<NameType> criticalPathSet(criticalPath.begin(), criticalPath.end());
    std::cout << "Critical path tasks: ";
    for (const auto& task : criticalPath) {
        std::cout << task << " ";
    }
    std::cout << std::endl;

    // Get tasks in topological order
    std::vector<NameType> topoOrder;
    TaskExecutionModels::getTopologicalOrder(app, &topoOrder);
    std::cout << "Topological order size: " << topoOrder.size() << std::endl;

    // Sort tasks with same topological position by priority
    std::vector<NameType> prioritizedOrder;
    std::unordered_map<NameType, bool> processed;

    // Initialize prioritized task queue with tasks having no predecessors
    std::vector<NameType> readyTasks;
    for (const auto& task : topoOrder) {
        int predCount = countPredecessors(app, task);
        std::cout << "Task " << task << " has " << predCount << " predecessors" << std::endl;
        if (predCount == 0) {
            readyTasks.push_back(task);
            processed[task] = true;
            std::cout << "  -> Added to ready tasks" << std::endl;
        }
    }

    std::cout << "Initial ready tasks count: " << readyTasks.size() << std::endl;

    // Sort ready tasks by priority (higher priority first)
    std::sort(readyTasks.begin(), readyTasks.end(),
             [&priorities](const NameType& a, const NameType& b) {
                 int prioA = priorities.find(a) != priorities.end() ? priorities.at(a) : -1;
                 int prioB = priorities.find(b) != priorities.end() ? priorities.at(b) : -1;
                 return prioA > prioB;
             });

    std::cout << "Ready tasks after priority sort: ";
    for (size_t i = 0; i < readyTasks.size(); i++) {
        int prio = priorities.find(readyTasks[i]) != priorities.end() ? priorities.at(readyTasks[i]) : -1;
        std::cout << readyTasks[i] << "(" << prio << ") ";
    }
    std::cout << std::endl;

    // Core finish times - when each core becomes available
    std::vector<int> coreFinishTimes(numCores, frameStart);  // Initialize with frame start
    std::cout << "Initialized " << numCores << " cores with finish time " << frameStart << std::endl;

    // Task finish times - when each task completes
    std::unordered_map<NameType, int> taskFinishTimes;

    // Track the power consumption of each core at each timestamp
    std::vector<std::vector<double>> corePowerValues;

    // Track which tasks are running at each time step
    std::vector<std::vector<NameType>> pwdm;

    // Process ready tasks in priority order
    int taskCounter = 0;
    while (!readyTasks.empty()) {
        taskCounter++;
        std::cout << "\n--- Processing Task #" << taskCounter << " ---" << std::endl;

        // Get highest priority task
        NameType currentTask = readyTasks.front();
        readyTasks.erase(readyTasks.begin());
        std::cout << "Current task: " << currentTask << std::endl;

        // Calculate earliest possible start time based on predecessors
        int earliestStart = frameStart;  // No task can start before frame start
        std::cout << "Initial earliest start: " << earliestStart << std::endl;

        int predCount = 0;
        for (const auto& [taskId, taskInfo] : app->tasks) {
            for (const auto& transition : taskInfo->transitions) {
                if (transition.targetState == currentTask) {
                    predCount++;
                    auto it = taskFinishTimes.find(taskId);
                    if (it != taskFinishTimes.end()) {
                        int predFinishTime = it->second;
                        std::cout << "  Predecessor " << taskId << " finishes at time " << predFinishTime << std::endl;
                        earliestStart = std::max(earliestStart, predFinishTime);
                    } else {
                        std::cout << "  WARNING: Predecessor " << taskId << " not found in taskFinishTimes!" << std::endl;
                    }
                }
            }
        }
        std::cout << "Found " << predCount << " predecessors, earliest start: " << earliestStart << std::endl;

        // Get task execution time and peak power
        int execTime = app->tasks[currentTask]->executionTime;
        std::cout << "Execution time: " << execTime << std::endl;

        // Check if task can complete within the deadline
        if (earliestStart + execTime > frameEnd) {
            std::cout << "ERROR: Task " << currentTask << " cannot complete within deadline!" << std::endl;
            std::cout << "  Earliest start: " << earliestStart << ", Execution time: " << execTime
                      << ", Would finish at: " << (earliestStart + execTime) << ", Deadline: " << frameEnd << std::endl;
            std::cout << "PWDM construction failed - returning empty results" << std::endl;
            return {{}, {}, {}};  // Return empty results if any task cannot be scheduled
        }

        // Get the base task name (without instance number)
        NameType baseTaskId = currentTask;
        size_t underscorePos = currentTask.find_last_of('_');
        if (underscorePos != std::string::npos) {
            baseTaskId = currentTask.substr(0, underscorePos);
            std::cout << "Base task ID: " << baseTaskId << " (from " << currentTask << ")" << std::endl;
        }

        double taskPower = peakPowerMap.find(baseTaskId) != peakPowerMap.end() ?
                           peakPowerMap.at(baseTaskId) : 1.0;
        std::cout << "Task power: " << taskPower << std::endl;

        // Determine which core to assign to the task
        int assignedCore = -1;
        int taskStartTime = -1;

        // If it's a critical path task, always assign to core 0
        if (criticalPathSet.find(currentTask) != criticalPathSet.end()) {
            std::cout << "Task is on critical path - assigning to core 0" << std::endl;
            assignedCore = 0;
            taskStartTime = std::max(earliestStart, coreFinishTimes[0]);

            // Check if critical path task can finish within deadline
            if (taskStartTime + execTime > frameEnd) {
                std::cout << "ERROR: Critical path task " << currentTask << " cannot complete within deadline!" << std::endl;
                std::cout << "  Start time: " << taskStartTime << ", Would finish at: " << (taskStartTime + execTime)
                          << ", Deadline: " << frameEnd << std::endl;
                std::cout << "PWDM construction failed - returning empty results" << std::endl;
                return {{}, {}, {}};
            }

            std::cout << "Core 0 available at: " << coreFinishTimes[0] << ", task start time: " << taskStartTime << std::endl;
        } else {
            std::cout << "Task is NOT on critical path - finding best core" << std::endl;
            // Try to find the earliest available core that won't violate TDP
            int minStartTime = INT_MAX;

            for (int core = 0; core < numCores; core++) {
                // Earliest this core could start the task
                int potentialStartTime = std::max(earliestStart, coreFinishTimes[core]);

                // Check if task can complete within deadline on this core
                if (potentialStartTime + execTime > frameEnd) {
                    std::cout << "  Core " << core << " rejected - would exceed deadline (start: "
                              << potentialStartTime << ", finish: " << (potentialStartTime + execTime)
                              << ", deadline: " << frameEnd << ")" << std::endl;
                    continue;
                }

                std::cout << "  Core " << core << " available at " << coreFinishTimes[core]
                          << ", potential start: " << potentialStartTime << std::endl;

                // Check if TDP constraint will be violated
                bool tdpViolation = false;
                for (int t = potentialStartTime; t < potentialStartTime + execTime; t++) {
                    // Expand vectors if needed (only within frame bounds)
                    while (corePowerValues.size() <= t && t < frameEnd) {
                        corePowerValues.push_back(std::vector<double>(numCores, 0.0));
                    }

                    // Calculate total power at time t if we add this task
                    double totalPower = taskPower;  // Start with current task's power
                    for (int c = 0; c < numCores; c++) {
                        if (c != core && t < corePowerValues.size()) {
                            totalPower += corePowerValues[t][c];
                        }
                    }

                    // Check if adding this task would exceed TDP
                    if (totalPower > maxChipTDP) {
                        std::cout << "    TDP violation at time " << t << ": " << totalPower
                                  << " > " << maxChipTDP << std::endl;
                        tdpViolation = true;
                        break;
                    }
                }

                // If no TDP violation and this is the earliest start time, select this core
                if (!tdpViolation && potentialStartTime < minStartTime) {
                    std::cout << "    Core " << core << " is viable with start time " << potentialStartTime << std::endl;
                    minStartTime = potentialStartTime;
                    assignedCore = core;
                } else if (tdpViolation) {
                    std::cout << "    Core " << core << " rejected due to TDP violation" << std::endl;
                } else {
                    std::cout << "    Core " << core << " rejected (later start time: " << potentialStartTime
                              << " vs current best: " << minStartTime << ")" << std::endl;
                }
            }

            if (assignedCore != -1) {
                taskStartTime = minStartTime;
                std::cout << "Selected core " << assignedCore << " with start time " << taskStartTime << std::endl;
            } else {
                std::cout << "ERROR: No suitable core found for task " << currentTask << " within deadline constraints!" << std::endl;
                std::cout << "PWDM construction failed - returning empty results" << std::endl;
                return {{}, {}, {}};  // Return empty results if any task cannot be scheduled
            }
        }

        // Schedule the task
        int finishTime = taskStartTime + execTime;
        schedule[currentTask] = {assignedCore, taskStartTime, finishTime};
        taskFinishTimes[currentTask] = finishTime;
        coreFinishTimes[assignedCore] = finishTime;

        std::cout << "SCHEDULED: " << currentTask << " -> Core " << assignedCore
                  << ", Start: " << taskStartTime << ", Finish: " << finishTime << std::endl;

        // Update power values and PWDM (only within frame bounds)
        for (int t = taskStartTime; t < finishTime && t < frameEnd; t++) {
            while (corePowerValues.size() <= t) {
                corePowerValues.push_back(std::vector<double>(numCores, 0.0));
            }
            while (pwdm.size() <= t) {
                pwdm.push_back(std::vector<NameType>());
            }

            corePowerValues[t][assignedCore] = taskPower;
            pwdm[t].push_back(currentTask);
        }

        std::cout << "Updated power values and PWDM for time range [" << taskStartTime
                  << ", " << finishTime << ")" << std::endl;

        // Update ready tasks - check if any new tasks are ready
        std::vector<NameType> newlyReady;
        for (const auto& transition : app->tasks[currentTask]->transitions) {
            NameType nextTask = transition.targetState;
            std::cout << "Checking if successor " << nextTask << " is ready..." << std::endl;

            if (processed.find(nextTask) == processed.end()) {
                bool allPredecessorsFinished = true;
                std::vector<NameType> predecessors;

                for (const auto& [taskId, taskInfo] : app->tasks) {
                    for (const auto& trans : taskInfo->transitions) {
                        if (trans.targetState == nextTask) {
                            predecessors.push_back(taskId);
                            if (taskFinishTimes.find(taskId) == taskFinishTimes.end()) {
                                allPredecessorsFinished = false;
                                std::cout << "  Predecessor " << taskId << " not finished yet" << std::endl;
                            }
                        }
                    }
                    if (!allPredecessorsFinished) break;
                }

                if (allPredecessorsFinished) {
                    std::cout << "  All predecessors finished for " << nextTask << " - adding to ready tasks" << std::endl;
                    readyTasks.push_back(nextTask);
                    newlyReady.push_back(nextTask);
                    processed[nextTask] = true;
                } else {
                    std::cout << "  " << nextTask << " still waiting for predecessors" << std::endl;
                }
            } else {
                std::cout << "  " << nextTask << " already processed" << std::endl;
            }
        }

        if (!newlyReady.empty()) {
            std::cout << "Added " << newlyReady.size() << " new ready tasks, re-sorting by priority" << std::endl;
            std::sort(readyTasks.begin(), readyTasks.end(),
                     [&priorities](const NameType& a, const NameType& b) {
                         int prioA = priorities.find(a) != priorities.end() ? priorities.at(a) : -1;
                         int prioB = priorities.find(b) != priorities.end() ? priorities.at(b) : -1;
                         return prioA > prioB;
                     });

            std::cout << "Updated ready tasks queue: ";
            for (const auto& task : readyTasks) {
                int prio = priorities.find(task) != priorities.end() ? priorities.at(task) : -1;
                std::cout << task << "(" << prio << ") ";
            }
            std::cout << std::endl;
        }

        std::cout << "Remaining ready tasks: " << readyTasks.size() << std::endl;
    }

    std::cout << "\n=== PWDM Construction Completed Successfully ===" << std::endl;
    std::cout << "Total tasks scheduled: " << schedule.size() << std::endl;
    std::cout << "PWDM time steps: " << pwdm.size() << std::endl;
    std::cout << "Power values time steps: " << corePowerValues.size() << std::endl;
    std::cout << "Time frame used: [" << frameStart << ", " << frameEnd << "]" << std::endl;

    // Print final schedule summary
    std::cout << "\nFinal Schedule Summary:" << std::endl;
    for (const auto& [task, schedInfo] : schedule) {
        auto [core, start, finish] = schedInfo;
        std::cout << "  " << task << ": Core " << core << ", [" << start << "-" << finish << "]" << std::endl;
    }

    return {schedule, pwdm, corePowerValues};
}

///////////////////////////////////////////////////////////////////////////////////////
/**
 * Implements Algorithm 2: Peak Power-Aware Accumulation of PWDMs with task-specific scheduling
 *
 * @param schedule1 First application's task schedule
 * @param wdm1 First workload distribution model
 * @param ppd1 First power profile distribution
 * @param schedule2 Second application's task schedule
 * @param wdm2 Second workload distribution model
 * @param ppd2 Second power profile distribution
 * @param maxCores Maximum number of cores (m in the algorithm)
 * @param chipTDP Maximum chip TDP
 * @return Tuple containing: combined WDM, combined schedule, combined power profile, and blocking interval
 */

 std::tuple<std::vector<std::vector<NameType>>,
           std::unordered_map<NameType, std::tuple<int, int, int>>,
           std::vector<std::vector<double>>,
           int>
CPCModel::accumulatePWDMs(
    const std::unordered_map<NameType, std::tuple<int, int, int>>& schedule1,
    const std::vector<std::vector<NameType>>& wdm1,
    const std::vector<std::vector<double>>& ppd1,
    const std::unordered_map<NameType, std::tuple<int, int, int>>& schedule2,
    const std::vector<std::vector<NameType>>& wdm2,
    const std::vector<std::vector<double>>& ppd2,
    int maxCores,
    double chipTDP,
    int relativeDeadline) {

    // Check if wdm2 and ppd2 are empty
    bool wdm2Empty = wdm2.empty() || std::all_of(wdm2.begin(), wdm2.end(),
                                                  [](const std::vector<NameType>& v) { return v.empty(); });
    bool ppd2Empty = ppd2.empty() || std::all_of(ppd2.begin(), ppd2.end(),
                                                  [](const std::vector<double>& v) { return v.empty() ||
                                                   std::all_of(v.begin(), v.end(), [](double d) { return d == 0.0; }); });

    if (wdm2Empty && ppd2Empty) {
        std::cout << "[WARNING] wdm2 and ppd2 are empty. Returning original schedule with large blocking interval." << std::endl;
        const int LARGE_BLOCKING_INTERVAL = 10000;
        return {wdm1, schedule1, ppd1, LARGE_BLOCKING_INTERVAL};
    }

    // Extract task start times from wdm2
    std::unordered_map<NameType, int> taskStartTimes;
    for (size_t t = 0; t < wdm2.size(); t++) {
        for (const auto& task : wdm2[t]) {
            if (taskStartTimes.find(task) == taskStartTimes.end()) {
                taskStartTimes[task] = t;
            }
        }
    }

    // Initialize result with first schedule
    std::vector<std::vector<NameType>> combinedWdm = wdm1;
    std::unordered_map<NameType, std::tuple<int, int, int>> combinedSchedule = schedule1;
    std::vector<std::vector<double>> combinedPpd = ppd1;
    int totalBlocking = 0;

    // Process tasks from wdm2 in order of their original start times
    std::vector<std::pair<NameType, int>> tasksByStartTime(taskStartTimes.begin(), taskStartTimes.end());
    std::sort(tasksByStartTime.begin(), tasksByStartTime.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    for (const auto& [task, originalStartTime] : tasksByStartTime) {
        // Skip if task already in schedule1
        if (schedule1.find(task) != schedule1.end()) {
            std::cout << "[INFO] Task " << task << " already in schedule1, skipping." << std::endl;
            continue;
        }

        // Get task info from schedule2
        if (schedule2.find(task) == schedule2.end()) {
            std::cout << "[ERROR] Task " << task << " not found in schedule2!" << std::endl;
            continue;
        }

        auto [origCore, start, finish] = schedule2.at(task);
        int execTime = finish - start;
        int latestStartTime = relativeDeadline - execTime;

        // Try to place the task starting from its original time up to the deadline
        bool placed = false;
        for (int attemptStart = originalStartTime;
             attemptStart <= latestStartTime && !placed;
             attemptStart++) {

            // Try each core for this start time
            for (int core = 0; core < maxCores && !placed; core++) {
                bool coreAvailable = true;
                int attemptFinish = attemptStart + execTime;

                // Check core availability and power for each time slot
                for (int t = attemptStart; t < attemptFinish; t++) {
                    // Resize if needed
                    if (t >= combinedWdm.size()) {
                        combinedWdm.resize(t + 1);
                    }
                    if (t >= combinedPpd.size()) {
                        combinedPpd.resize(t + 1, std::vector<double>(ppd1[0].size(), 0.0));
                    }

                    // Check if core is already in use
                    for (const auto& existingTask : combinedWdm[t]) {
                        if (combinedSchedule.find(existingTask) != combinedSchedule.end()) {
                            if (std::get<0>(combinedSchedule[existingTask]) == core) {
                                coreAvailable = false;
                                break;
                            }
                        }
                    }
                    if (!coreAvailable) break;

                    // Check power constraints
                    double totalPower = 0.0;
                    for (const auto& corePower : combinedPpd[t]) {
                        totalPower += corePower;
                    }

                    // Add the power of this task
                    double taskPower = 0.0;
                    if (originalStartTime < ppd2.size() && origCore < ppd2[originalStartTime].size()) {
                        taskPower = ppd2[originalStartTime][origCore];
                    }

                    if (totalPower + taskPower > chipTDP) {
                        coreAvailable = false;
                        break;
                    }
                }

                // If core is available, place the task
                if (coreAvailable) {
                    combinedSchedule[task] = {core, attemptStart, attemptFinish};
                    totalBlocking = std::max(totalBlocking, attemptStart - originalStartTime);

                    // Update WDM
                    for (int t = attemptStart; t < attemptFinish; t++) {
                        if (t >= combinedWdm.size()) {
                            combinedWdm.resize(t + 1);
                        }
                        combinedWdm[t].push_back(task);
                    }

                    // Update PPD
                    for (int t = attemptStart; t < attemptFinish; t++) {
                        if (t >= combinedPpd.size()) {
                            combinedPpd.resize(t + 1, std::vector<double>(ppd1[0].size(), 0.0));
                        }
                        if (originalStartTime < ppd2.size() && origCore < ppd2[originalStartTime].size()) {
                            combinedPpd[t][core] += ppd2[originalStartTime][origCore];
                        }
                    }

                    placed = true;
                    std::cout << "[DEBUG] Placed task " << task << " on core " << core
                              << " from " << attemptStart << " to " << attemptFinish
                              << " (shifted by " << (attemptStart - originalStartTime) << ")" << std::endl;
                }
            }
        }

        if (!placed) {
            std::cout << "[ERROR] Could not place task " << task << " before deadline!" << std::endl;
            // Return with large blocking interval to indicate failure
            return {combinedWdm, combinedSchedule, combinedPpd, 10000};
        }
    }

    std::cout << "[DEBUG] PWDM Accumulation Complete. Total blocking: " << totalBlocking << std::endl;
    return {combinedWdm, combinedSchedule, combinedPpd, totalBlocking};
}




/**
 * Combines two application schedules using the power-aware accumulation algorithm
 *
 * @param schedule1 First application's task schedule
 * @param wdm1 First application's workload distribution model
 * @param ppd1 First application's power profile distribution
 * @param schedule2 Second application's task schedule
 * @param wdm2 Second application's workload distribution model
 * @param ppd2 Second application's power profile distribution
 * @param maxCores Maximum number of cores
 * @param chipTDP Maximum chip TDP
 * @return Tuple containing: combined schedule, combined WDM, combined power profile, and blocking interval
 */
std::tuple<std::unordered_map<NameType, std::tuple<int, int, int>>,
           std::vector<std::vector<NameType>>,
           std::vector<std::vector<double>>,
           int>
CPCModel::combineApplicationSchedules(
    const std::unordered_map<NameType, std::tuple<int, int, int>>& schedule1,
    const std::vector<std::vector<NameType>>& wdm1,
    const std::vector<std::vector<double>>& ppd1,
    const std::unordered_map<NameType, std::tuple<int, int, int>>& schedule2,
    const std::vector<std::vector<NameType>>& wdm2,
    const std::vector<std::vector<double>>& ppd2,
    int maxCores,
    double chipTDP,int relativeDeadline) {

    std::cout << "[DEBUG] Combining two application schedules..." << std::endl;

    // Accumulate PWDMs - now includes schedule creation
    auto [combinedWdm, combinedSchedule, combinedPpd, blockingInterval] = accumulatePWDMs(
        schedule1, wdm1, ppd1, schedule2, wdm2, ppd2, maxCores, chipTDP,relativeDeadline);

    // Display the combined schedule
    std::cout << "\n[DEBUG] Combined Task Schedule:" << std::endl;
    std::cout << "==========================" << std::endl;
    std::cout << "Task\t|\tCore\t|\tStart\t|\tFinish" << std::endl;
    std::cout << "--------------------------" << std::endl;

    // Sort tasks by start time for display
    std::vector<std::pair<NameType, std::tuple<int, int, int>>> sortedSchedule(
        combinedSchedule.begin(), combinedSchedule.end());
    std::sort(sortedSchedule.begin(), sortedSchedule.end(),
             [](const auto& a, const auto& b) {
                 return std::get<1>(a.second) < std::get<1>(b.second);
             });

    for (const auto& [task, details] : sortedSchedule) {
        int core = std::get<0>(details);
        int start = std::get<1>(details);
        int finish = std::get<2>(details);

        std::cout << task << "\t|\t" << core << "\t|\t" << start << "\t|\t" << finish << std::endl;
    }
    std::cout << "==========================" << std::endl;

    // Display the power workload distribution
    std::cout << "\n[DEBUG] Combined Power Workload Distribution:" << std::endl;
    std::cout << "==========================" << std::endl;
    std::cout << "Time\t|\tRunning Tasks\t|\tChip TDP" << std::endl;
    std::cout << "--------------------------" << std::endl;

    for (size_t t = 0; t < combinedWdm.size(); t++) {
        double totalPower = 0.0;
        if (t < combinedPpd.size()) {
            for (const auto& corePower : combinedPpd[t]) {
                totalPower += corePower;
            }
        }

        std::cout << t << "\t|\t";
        for (const auto& task : combinedWdm[t]) {
            std::cout << task << " ";
        }
        std::cout << "\t|\t" << totalPower << "/" << chipTDP << std::endl;
    }
    std::cout << "==========================" << std::endl;



    std::cout << "[DEBUG] Application Schedule Combination Complete." << std::endl;

    return {combinedSchedule, combinedWdm, combinedPpd, blockingInterval};
}

// Define the enhanced task tracking structure
struct TaskMetadata {
    // Task identification
    NameType originalTaskId;       // Original task ID
    NameType dagName;              // DAG name
    int instanceNumber;            // Instance number

    // Performance metrics
    double peakPower;              // Peak power value

    // Constructor for default values
    TaskMetadata() :
        originalTaskId(""),
        dagName("unknown"),
        instanceNumber(-1),
        peakPower(20.0) {}
};

// Global map to track all task metadata
std::unordered_map<NameType, TaskMetadata> globalTaskMetadata;

// Function to get task peak power (follows the existing pattern)
double getTaskPeakPower(const NameType& originalTaskId) {
    static std::unordered_map<NameType, double> taskPeakPowerMap = {
    {"v1", 14.0}, {"v2", 19.0}, {"v3", 20.0}, {"v4", 9.0}, {"v5", 24.0},
    {"v6", 10.0}, {"v7", 13.0}, {"v8", 17.0}, {"v9", 11.0}, {"v10", 15.0},
    {"v11", 18.0}, {"v12", 7.0}, {"v13", 16.0}, {"v14", 23.0}, {"v15", 8.0},
    {"v16", 20.0}, {"v17", 12.0}, {"v18", 25.0}, {"v19", 6.0}, {"v20", 22.0},

    {"u1", 15.0}, {"u2", 11.0}, {"u3", 8.0}, {"u4", 19.0}, {"u5", 17.0},
    {"u6", 10.0}, {"u7", 13.0}, {"u8", 22.0}, {"u9", 16.0}, {"u10", 9.0},
    {"u11", 14.0}, {"u12", 7.0}, {"u13", 12.0}, {"u14", 25.0}, {"u15", 6.0},
    {"u16", 21.0}, {"u17", 23.0}, {"u18", 10.0}, {"u19", 5.0}, {"u20", 24.0},

    {"x1", 11.0}, {"x2", 6.0}, {"x3", 18.0}, {"x4", 7.0}, {"x5", 15.0},
    {"x6", 21.0}, {"x7", 12.0}, {"x8", 13.0}, {"x9", 20.0}, {"x10", 10.0},
    {"x11", 16.0}, {"x12", 8.0}, {"x13", 17.0}, {"x14", 14.0}, {"x15", 22.0},
    {"x16", 9.0}, {"x17", 25.0}, {"x18", 19.0}, {"x19", 5.0}, {"x20", 23.0},

    {"w1", 17.0}, {"w2", 8.0}, {"w3", 20.0}, {"w4", 9.0}, {"w5", 24.0},
    {"w6", 14.0}, {"w7", 11.0}, {"w8", 16.0}, {"w9", 6.0}, {"w10", 13.0},
    {"w11", 25.0}, {"w12", 7.0}, {"w13", 12.0}, {"w14", 23.0}, {"w15", 10.0},
    {"w16", 15.0}, {"w17", 5.0}, {"w18", 19.0}, {"w19", 18.0}, {"w20", 21.0},

    {"t1", 9.0}, {"t2", 12.0}, {"t3", 14.0}, {"t4", 25.0}, {"t5", 7.0},
    {"t6", 15.0}, {"t7", 20.0}, {"t8", 8.0}, {"t9", 18.0}, {"t10", 11.0},
    {"t11", 23.0}, {"t12", 5.0}, {"t13", 17.0}, {"t14", 22.0}, {"t15", 6.0},
    {"t16", 21.0}, {"t17", 13.0}, {"t18", 10.0}, {"t19", 16.0}, {"t20", 19.0}
};


    if (taskPeakPowerMap.find(originalTaskId) != taskPeakPowerMap.end()) {
        return taskPeakPowerMap[originalTaskId];
    }

    // Default value if not found
    return 20.0;
}

// Helper functions for the task tracking system
void initializeTaskMetadata() {
    globalTaskMetadata.clear();
}

void registerTask(const NameType& instanceTaskId, const NameType& originalTaskId,
                 const NameType& dagName, int instanceNumber) {
    TaskMetadata metadata;
    metadata.originalTaskId = originalTaskId;
    metadata.dagName = dagName;
    metadata.instanceNumber = instanceNumber;
    metadata.peakPower = getTaskPeakPower(originalTaskId);

    globalTaskMetadata[instanceTaskId] = metadata;
}

TaskMetadata getTaskMetadata(const NameType& instanceTaskId) {
    if (globalTaskMetadata.find(instanceTaskId) != globalTaskMetadata.end()) {
        return globalTaskMetadata[instanceTaskId];
    }

    return TaskMetadata(); // Return default metadata if not found
}

// Function to print task metadata for debugging
void printTaskMetadata(const std::unordered_map<NameType, std::tuple<int, int, int>>& combinedSchedule) {
    std::cout << "\nTask Metadata:" << std::endl;
    std::cout << "==============" << std::endl;
    std::cout << std::setw(20) << "Task ID"
              << std::setw(15) << "DAG"
              << std::setw(10) << "Instance"
              << std::setw(10) << "Core"
              << std::setw(10) << "Start"
              << std::setw(10) << "Finish"
              << std::setw(10) << "Power"
              << std::endl;

    for (const auto& [taskId, metadata] : globalTaskMetadata) {
        int core = -1, start = -1, finish = -1;
        if (combinedSchedule.find(taskId) != combinedSchedule.end()) {
            auto [c, s, f] = combinedSchedule.at(taskId);
            core = c;
            start = s;
            finish = f;
        }

        std::cout << std::setw(20) << taskId
                  << std::setw(15) << metadata.dagName
                  << std::setw(10) << metadata.instanceNumber
                  << std::setw(10) << core
                  << std::setw(10) << start
                  << std::setw(10) << finish
                  << std::setw(10) << metadata.peakPower
                  << std::endl;
    }
}




// Enhanced version of offlineCoreAssignment that adds DAG selection strategies
std::tuple<std::unordered_map<NameType, int>,
           std::unordered_map<NameType, std::tuple<int, int, int>>,
           std::vector<std::vector<NameType>>,
           std::vector<std::vector<double>>,
           int,double>
CPCModel::offlineCoreAssignment(int maxCores, double chipTDP, int mode = 0) {
     std::random_device rd;
     std::mt19937 gen(rd());
    // Initialize task metadata tracking
    initializeTaskMetadata();

    std::cout << "[DEBUG] Performing Offline Core Assignment for Multi-DAG..." << std::endl;

    // Display selection policy based on mode
    switch (mode) {
        case 0:
            std::cout << "[DEBUG] DAG Selection Policy: Select DAG with earliest arrival time. If multiple DAGs have the same arrival time, select the one with the lowest deadline." << std::endl;
            break;
        case 1:
            std::cout << "[DEBUG] DAG Selection Policy: First In First Out (FIFO) - Select DAG with earliest arrival time. If multiple DAGs have the same arrival time, select the one with maximum peak power. If peak power is also the same, choose randomly." << std::endl;
            break;
        case 2:
            std::cout << "[DEBUG] DAG Selection Policy: Select DAG with earliest arrival time. If multiple DAGs have the same arrival time, select the one with the least laxity (deadline - critical path length)." << std::endl;
            break;
        case 3:
            std::cout << "[DEBUG] DAG Selection Policy: Select DAG with earliest arrival time. If multiple DAGs have the same arrival time, select the one with the maximum average power consumption." << std::endl;
            break;
        case 4:
            std::cout << "[DEBUG] DAG Selection Policy: Select DAG with earliest arrival time. If multiple DAGs have the same arrival time, select the one with the highest urgency factor (average peak power / laxity)." << std::endl;
            break;
        default:
            std::cout << "[DEBUG] DAG Selection Policy: Using default - Select DAG with earliest arrival time. If multiple DAGs have the same arrival time, select the one with the lowest deadline." << std::endl;
            mode = 0;
            break;
    }

    std::vector<std::vector<NameType>> combinedWdm;
    std::unordered_map<NameType, int> coreAssignments;
    std::unordered_map<NameType, std::tuple<int, int, int>> combinedSchedule;
    std::vector<std::vector<double>> combinedPpd;
    int blockingInterval = 0;
    double successRate=0.0;

    // Get and sort DAGs from the models directory
    std::vector<Applications*> sortedDags;
    std::vector<int> periods;

    // Read all DAG files from the models directory.................
    for (const auto& entry : std::filesystem::directory_iterator("Models")) {
        if (entry.path().extension() == ".dag") {
            try {
                Applications* app = DAG::readDAGFromFile(entry.path().stem().string());

                // Pre-compute the critical path for this application
                std::vector<NameType> criticalPath = findCriticalPath(app);
                // app->criticalLength is now set by the findCriticalPath function

                sortedDags.push_back(app);
                periods.push_back(app->period);
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Error loading DAG " << entry.path().filename().string()
                          << ": " << e.what() << std::endl;
            }
        }
    }

    if (sortedDags.empty()) {
        std::cout << "[ERROR] No DAG files found in the models directory" << std::endl;
        return {coreAssignments, combinedSchedule, combinedWdm, combinedPpd, blockingInterval,successRate};
    }

    // Calculate hyperperiod which is the lcm of all the dags.............
     int hyperperiod = 1;
    for (int period : periods) {
        hyperperiod = std::lcm(hyperperiod, period);
    }
    std::cout << "[DEBUG] Hyperperiod: " << hyperperiod << std::endl;

    std::uniform_int_distribution<> arrivalDist(0, hyperperiod - 1);

    // Track last completion time for each application
    std::unordered_map<std::string, int> lastCompletionTime;

    // Create queues for each application with randomized first arrival time
    struct AppQueue {
        Applications* app;
        int nextInstance;
        int nextArrivalTime;
    };

    std::vector<AppQueue> appQueues;
    for (auto* app : sortedDags) {
        AppQueue queue;
        queue.app = app;
        queue.nextInstance = 0;
        queue.nextArrivalTime = arrivalDist(gen);
        appQueues.push_back(queue);
        lastCompletionTime[app->appName] = 0;
    }

    // Precompute metrics for each application
    struct AppMetrics {
        double maxPeakPower;   // Maximum peak power of any task
        double avgPeakPower;   // Average peak power across all tasks
        int laxity;            // Deadline - critical path length
        double urgencyFactor;  // avgPeakPower / laxity
    };

    std::unordered_map<std::string, AppMetrics> appMetricsMap;

    for (auto* app : sortedDags) {
        AppMetrics metrics;
        double totalPower = 0.0;
        double maxPower = 0.0;
        int taskCount = 0;

        for (const auto& [taskId, taskInfo] : app->tasks) {
            // Get peak power for this task
            double taskPower = 0.0;
            if (taskPeakPowerMap.find(taskId) != taskPeakPowerMap.end()) {
                taskPower = taskPeakPowerMap[taskId];
            } else {
                // If no predefined value, generate a random power value based on task name
                taskPower = 5.0 + ((std::hash<std::string>{}(taskId) % 60) / 10.0);
            }

            totalPower += taskPower;
            maxPower = std::max(maxPower, taskPower);
            taskCount++;
        }

        metrics.maxPeakPower = maxPower;
        metrics.avgPeakPower = (taskCount > 0) ? (totalPower / taskCount) : 0.0;
        metrics.laxity = app->relativeDeadline - app->criticalLength;
        metrics.laxity = (metrics.laxity > 0) ? metrics.laxity : 1; // Avoid division by zero
        metrics.urgencyFactor = metrics.avgPeakPower / metrics.laxity;

        appMetricsMap[app->appName] = metrics;

        std::cout << "[DEBUG] Pre-computed metrics for " << app->appName
                  << ": Critical Path Length=" << app->criticalLength
                  << ", Laxity=" << metrics.laxity
                  << ", Avg Power=" << metrics.avgPeakPower
                  << ", Max Power=" << metrics.maxPeakPower
                  << ", Urgency Factor=" << metrics.urgencyFactor << std::endl;
    }

    // Statistics tracking
    int totalDAGsProcessed = 0;
    int successfullyScheduledDAGs = 0;
    int failedDAGs = 0;

    int currentTime = 0;
    while (currentTime < 2*hyperperiod) {// we will takes the dags if its arrival time is less than hyperperiod..........
        std::cout << "\n[DEBUG] Time step: " << currentTime << std::endl;
        std::cout << "[DEBUG] Available DAGs:" << std::endl;

        for (size_t i = 0; i < appQueues.size(); i++) {// going through all the queues and finding which app instances are ready.....
            if (appQueues[i].nextArrivalTime <= currentTime) {
                const auto& app = appQueues[i].app;
                const auto& metrics = appMetricsMap[app->appName];

                std::cout << "  - " << app->appName
                          << " (Arrival: " << appQueues[i].nextArrivalTime
                          << ", Deadline: " << app->relativeDeadline
                          << ", Critical Path: " << app->criticalLength
                          << ", Laxity: " << metrics.laxity
                          << ", Instance: " << appQueues[i].nextInstance << ")";

                switch (mode) {
                    case 1:
                        std::cout << " [Max Power: " << metrics.maxPeakPower << "]";
                        break;
                    case 3:
                    case 4:
                        std::cout << " [Avg Power: " << metrics.avgPeakPower
                                  << ", Urgency Factor: " << metrics.urgencyFactor << "]";
                        break;
                }

                std::cout << std::endl;
            }
        }

        // Find the application with the earliest arrival time
        int earliestArrival = INT_MAX;
        int selectedQueueIndex = -1;

        // Values for comparison based on mode
        int earliestDeadline = INT_MAX;
        int maxCriticalPath = -1;
        int minLaxity = INT_MAX;
        double maxAvgPower = -1.0;
        double maxUrgencyFactor = -1.0;
        double maxPeakPower = -1.0; // For FIFO mode

        // First pass: find earliest arrival time
        for (size_t i = 0; i < appQueues.size(); i++) {
            if (appQueues[i].nextArrivalTime <= currentTime) {
                if (appQueues[i].nextArrivalTime < earliestArrival) {
                    earliestArrival = appQueues[i].nextArrivalTime;
                }
            }
        }

        // Collect all DAGs with earliest arrival time for FIFO tie-breaking
        std::vector<int> candidateIndices;

        // Second pass: find DAGs with earliest arrival time and compare based on mode
        for (size_t i = 0; i < appQueues.size(); i++) {
            if (appQueues[i].nextArrivalTime <= currentTime &&
                appQueues[i].nextArrivalTime == earliestArrival) {

                bool select = false;
                const auto& app = appQueues[i].app;
                const auto& metrics = appMetricsMap[app->appName];

                switch (mode) {
                    case 0: // Minimum deadline
                        if (selectedQueueIndex == -1 || app->relativeDeadline < earliestDeadline) {
                            earliestDeadline = app->relativeDeadline;
                            select = true;
                        }
                        break;

                    case 1: // FIFO with max peak power tie-breaking, then random
                        candidateIndices.push_back(i);
                        if (selectedQueueIndex == -1 || metrics.maxPeakPower > maxPeakPower) {
                            maxPeakPower = metrics.maxPeakPower;
                            select = true;
                        } else if (metrics.maxPeakPower == maxPeakPower && selectedQueueIndex != -1) {
                            // Same max peak power - will be handled by random selection later
                            select = false;
                        }
                        break;

                    case 2: // Minimum laxity (deadline - critical path)
                        if (selectedQueueIndex == -1 || metrics.laxity < minLaxity) {
                            minLaxity = metrics.laxity;
                            select = true;
                        }
                        break;

                    case 3: // Maximum average power
                        if (selectedQueueIndex == -1 || metrics.avgPeakPower > maxAvgPower) {
                            maxAvgPower = metrics.avgPeakPower;
                            select = true;
                        }
                        break;

                    case 4: // Maximum urgency factor (avg peak power / laxity)
                        if (selectedQueueIndex == -1 || metrics.urgencyFactor > maxUrgencyFactor) {
                            maxUrgencyFactor = metrics.urgencyFactor;
                            select = true;
                        }
                        break;
                }

                if (select) {
                    selectedQueueIndex = i;
                }
            }
        }

        // Special handling for FIFO mode: random selection among ties
        if (mode == 1 && !candidateIndices.empty()) {
            // Find all candidates with the same max peak power
            std::vector<int> maxPowerCandidates;
            for (int idx : candidateIndices) {
                const auto& metrics = appMetricsMap[appQueues[idx].app->appName];
                if (std::abs(metrics.maxPeakPower - maxPeakPower) < 1e-6) { // Account for floating point precision
                    maxPowerCandidates.push_back(idx);
                }
            }

            if (maxPowerCandidates.size() > 1) {
                // Multiple DAGs with same max peak power - choose randomly
                std::uniform_int_distribution<> dis(0, maxPowerCandidates.size() - 1);
                selectedQueueIndex = maxPowerCandidates[dis(gen)];
                std::cout << "[DEBUG] FIFO: Multiple DAGs with same max peak power (" << maxPeakPower
                          << "), randomly selected index " << selectedQueueIndex << std::endl;
            } else if (maxPowerCandidates.size() == 1) {
                selectedQueueIndex = maxPowerCandidates[0];
            }
        }

        if (selectedQueueIndex == -1) {
            currentTime++;
            continue;
        }

        AppQueue& selectedQueue = appQueues[selectedQueueIndex];
        Applications* app = selectedQueue.app;
        const auto& metrics = appMetricsMap[app->appName];

        std::cout << "[DEBUG] Selected DAG: " << app->appName
                  << " (Arrival: " << selectedQueue.nextArrivalTime
                  << ", Deadline: " << app->relativeDeadline;

        // Print additional selection criteria based on mode
        switch (mode) {
            case 1:
                std::cout << ", Max Peak Power: " << metrics.maxPeakPower;
                break;
            case 2:
                std::cout << ", Laxity: " << metrics.laxity;
                break;
            case 3:
                std::cout << ", Avg Power: " << metrics.avgPeakPower;
                break;
            case 4:
                std::cout << ", Urgency Factor: " << metrics.urgencyFactor;
                break;
        }

        std::cout << ")" << std::endl;

        // Create a new instance
        Applications* instanceApp = new Applications(*app);
        instanceApp->appName = app->appName + "_" + std::to_string(selectedQueue.nextInstance);
        instanceApp->arrivalTime = selectedQueue.nextArrivalTime;
        //calculate the relative deadline of each app instance.....
        instanceApp->relativeDeadline = instanceApp->arrivalTime + app->relativeDeadline - app->arrivalTime;

        // Get the actual start time (max of arrival time and last completion time)
        //Actual instancestart time is important eventhough a application can arrive at arrivaltime but it wont be taken until its previous instance completed execution.....
        int instanceStartTime = std::max(selectedQueue.nextArrivalTime,
                                       lastCompletionTime[app->appName]);

        std::cout << "[DEBUG] Processing " << instanceApp->appName
                  << " (Start: " << instanceStartTime << ")" << std::endl;

        totalDAGsProcessed++;

        // Register tasks in our metadata tracking system
        for (const auto& [taskId, taskInfo] : instanceApp->tasks) {
            NameType instanceTaskId = taskId + "_" + std::to_string(selectedQueue.nextInstance);
            registerTask(instanceTaskId, taskId, app->appName, selectedQueue.nextInstance);
        }

        // Initialize peak power map with instance-specific task names
        std::unordered_map<NameType, double> peakPowerMap;
        for (const auto& [taskId, taskInfo] : instanceApp->tasks) {
            // Get the base task name (without instance number)
            NameType baseTaskId = taskId;

            // If we have a predefined peak power for this task, use it
            if (taskPeakPowerMap.find(baseTaskId) != taskPeakPowerMap.end()) {
                peakPowerMap[baseTaskId] = taskPeakPowerMap[baseTaskId];
            } else {
                // If no predefined value, generate a random power value based on task name
                // This ensures same task gets same power across instances
                double randomPower = 5.0 + ((std::hash<std::string>{}(baseTaskId) % 60) / 10.0);
                peakPowerMap[baseTaskId] = randomPower;
            }
        }

        // Find best core assignment
        double bestResponseTime = std::numeric_limits<double>::infinity();
        int bestCoreCount = 0;
        std::unordered_map<NameType, std::tuple<int, int, int>> bestSchedule;
        std::vector<std::vector<NameType>> bestWdm;
        std::vector<std::vector<double>> bestPpd;

        for (int i = maxCores; i >= 1; i--) {
            std::vector<NameType> criticalPath = findCriticalPath(instanceApp);

            // Register tasks in our metadata tracking system
            for (const auto& [taskId, taskInfo] : instanceApp->tasks) {
                NameType instanceTaskId = taskId + "_" + std::to_string(selectedQueue.nextInstance);
                registerTask(instanceTaskId, taskId, app->appName, selectedQueue.nextInstance);
            }

            std::vector<std::vector<NameType>> capacityProviders = identifyProviders(instanceApp, criticalPath);
            auto [directConsumers, indirectConsumers] = identifyConsumers(instanceApp, criticalPath, capacityProviders);
            std::unordered_map<NameType, int> priorities = assignPriorities(instanceApp, criticalPath, capacityProviders, directConsumers);
            auto [schedule, wdm, ppd] = constructPWDM(
                instanceApp, criticalPath, priorities, peakPowerMap, i, chipTDP, instanceStartTime);
            // Update task names in schedule to include instance number
            std::unordered_map<NameType, std::tuple<int, int, int>> instanceSchedule;
            for (const auto& [taskId, details] : schedule) {
                NameType instanceTaskId = taskId + "_" + std::to_string(selectedQueue.nextInstance);
                instanceSchedule[instanceTaskId] = details;
            }

            // Update task names in WDM
            std::vector<std::vector<NameType>> instanceWdm;
            for (const auto& tasks : wdm) {
                std::vector<NameType> instanceTasks;
                for (const auto& task : tasks) {
                    NameType instanceTaskId = task + "_" + std::to_string(selectedQueue.nextInstance);
                    instanceTasks.push_back(instanceTaskId);
                }
                instanceWdm.push_back(instanceTasks);
            }

            std::vector<std::vector<NameType>> tempWdm;
            std::unordered_map<NameType, std::tuple<int, int, int>> tempSchedule;
            std::vector<std::vector<double>> tempPpd;
            int tempBlockingInterval = 0;

            if (combinedWdm.empty()) {
                tempWdm = instanceWdm;
                tempSchedule = instanceSchedule;
                tempPpd = ppd;
            } else {
                auto result = accumulatePWDMs(
                    combinedSchedule, combinedWdm, combinedPpd,
                    instanceSchedule, instanceWdm, ppd,
                    maxCores, chipTDP,instanceApp->relativeDeadline);
                tempWdm = std::get<0>(result);
                tempSchedule = std::get<1>(result);
                tempPpd = std::get<2>(result);
                tempBlockingInterval = std::get<3>(result);
            }

            double responseTime = calculateResponseTime(instanceApp, i, tempBlockingInterval);

            if (responseTime < bestResponseTime) {
                bestResponseTime = responseTime;
                bestCoreCount = i;
                bestSchedule = instanceSchedule;
                bestWdm = instanceWdm;
                bestPpd = ppd;
            }
        }

        // CRITICAL FIX: Check deadline BEFORE adding to combined schedule
        if (bestResponseTime > app->relativeDeadline - app->arrivalTime) {
            std::cout << "[WARNING] Cannot meet deadline for " << instanceApp->appName
                      << " (Response Time: " << bestResponseTime
                      << ", Deadline: " << app->relativeDeadline
                      << "). Skipping this DAG instance." << std::endl;

            failedDAGs++;
            delete instanceApp;

            // Update queue for next instance
            selectedQueue.nextInstance++;
            selectedQueue.nextArrivalTime = selectedQueue.nextInstance * app->period;

            // Move to next time step or continue with other DAGs
            currentTime++;
            continue; // CONTINUE PROCESSING OTHER DAGS instead of terminating
        }

        // Only reach here if deadline CAN be met
        std::cout << "[SUCCESS] DAG " << instanceApp->appName
                  << " successfully scheduled with response time " << bestResponseTime
                  << " (Deadline: " << app->relativeDeadline << ")" << std::endl;

        successfullyScheduledDAGs++;
        coreAssignments[instanceApp->appName] = bestCoreCount;

        // Add to combined schedule only after validation
        if (combinedWdm.empty()) {
            combinedWdm = bestWdm;
            combinedSchedule = bestSchedule;
            combinedPpd = bestPpd;


        } else {
            auto result = accumulatePWDMs(
                combinedSchedule, combinedWdm, combinedPpd,
                bestSchedule, bestWdm, bestPpd,
                maxCores, chipTDP,instanceApp->relativeDeadline);

           //Rewriting the combined wdm and everything for the next iteration
            combinedWdm = std::get<0>(result);
            combinedSchedule = std::get<1>(result);
            combinedPpd = std::get<2>(result);
            blockingInterval = std::max(blockingInterval, std::get<3>(result));
        }

        // Update last completion time for this application
        int completionTime = 0;
        for (const auto& [task, details] : bestSchedule) {
            completionTime = std::max(completionTime, std::get<2>(details));
        }
        lastCompletionTime[app->appName] = completionTime;

        // Update queue for next instance
        selectedQueue.nextInstance++;
        selectedQueue.nextArrivalTime = selectedQueue.nextInstance * app->period;

        currentTime = completionTime;
        delete instanceApp;
    }

    // Print scheduling statistics
    std::cout << "\n[SCHEDULING STATISTICS]" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Total DAGs processed: " << totalDAGsProcessed << std::endl;
    std::cout << "Successfully scheduled: " << successfullyScheduledDAGs << std::endl;
    std::cout << "Failed to schedule: " << failedDAGs << std::endl;
    std::cout << "Success rate: " << (totalDAGsProcessed > 0 ?
                                    (double)successfullyScheduledDAGs / totalDAGsProcessed * 100.0 : 0.0)
              << "%" << std::endl;
    successRate=(double)successfullyScheduledDAGs / totalDAGsProcessed;

    // Display core assignments
    std::cout << "\nCore Assignments:" << std::endl;
    std::cout << "================" << std::endl;
    for (const auto& [dagId, cores] : coreAssignments) {
        std::cout << "DAG " << dagId << ": " << cores << " core(s)" << std::endl;
    }

    return {coreAssignments, combinedSchedule, combinedWdm, combinedPpd, blockingInterval,successRate};
}


























// Dynamic Voltage and Frequency Scaling function
// This function optimizes energy consumption by identifying slack time and
// reducing frequency where possible without violating deadlines or power constraints
std::unordered_map<NameType, std::tuple<int, int, int>> CPCModel::applyDVFS(
    const std::unordered_map<NameType, std::tuple<int, int, int>>& combinedSchedule,
    std::vector<std::vector<NameType>>& combinedWdm,
    std::vector<std::vector<double>>& combinedPpd,
    double chipTDP,
    int maxCores) {

    std::cout << "\n[DEBUG] Applying DVFS optimization..." << std::endl;

    // Create a copy of the original schedule to modify
    std::unordered_map<NameType, std::tuple<int, int, int>> dvfsSchedule = combinedSchedule;

    // Define available frequencies and their scaling factors
    const std::vector<double> availableFrequencies = {1.0, 1.5, 2.0};  // 1GHz, 1.5GHz, 2GHz
    const double baseFrequency = 2.0;  // Default frequency (2GHz)

    // Structure to hold task scheduling information for each core
    std::vector<std::vector<std::tuple<NameType, int, int>>> coreSchedules(maxCores);

    // Map to store the new frequency assignments for tasks
    std::unordered_map<NameType, double> taskFrequencies;

    // Initialize all tasks with base frequency
    for (const auto& [taskId, scheduleInfo] : dvfsSchedule) {
        taskFrequencies[taskId] = baseFrequency;
    }

    // Populate core schedules from combined schedule
    for (const auto& [taskId, scheduleInfo] : dvfsSchedule) {
        int core = std::get<0>(scheduleInfo);
        int start = std::get<1>(scheduleInfo);
        int finish = std::get<2>(scheduleInfo);

        if (core >= 0 && core < maxCores) {
            coreSchedules[core].push_back(std::make_tuple(taskId, start, finish));
        }
    }

    // Sort tasks in each core by start time
    for (auto& coreTasks : coreSchedules) {
        std::sort(coreTasks.begin(), coreTasks.end(),
            [](const auto& a, const auto& b) { return std::get<1>(a) < std::get<1>(b); });
    }

    // Find the hyperperiod (max finish time in schedule)
    int hyperperiod = 30;
    for (const auto& [taskId, scheduleInfo] : dvfsSchedule) {
        hyperperiod = std::max(hyperperiod, std::get<2>(scheduleInfo));
    }

    // Process each core to find and utilize slack time
    for (int core = 0; core < maxCores; core++) {
        const auto& coreTasks = coreSchedules[core];

        // Skip empty cores
        if (coreTasks.empty()) continue;

        std::cout << "[DEBUG] Analyzing core " << core << " for slack time..." << std::endl;

        // Find slack intervals on this core
        std::vector<std::tuple<int, int, NameType>> slackIntervals; // (start, end, preceding task)

        // Add slack from start if first task doesn't start at time 0
        if (!coreTasks.empty() && std::get<1>(coreTasks[0]) > 0) {
            slackIntervals.push_back(std::make_tuple(0, std::get<1>(coreTasks[0]), ""));
        }

        // Find slack between tasks
        for (size_t i = 0; i < coreTasks.size() - 1; i++) {
            int currentFinish = std::get<2>(coreTasks[i]);
            int nextStart = std::get<1>(coreTasks[i+1]);

            if (nextStart > currentFinish) {
                slackIntervals.push_back(std::make_tuple(
                    currentFinish, nextStart, std::get<0>(coreTasks[i])));
            }
        }

        // Add slack at the end if last task doesn't end at hyperperiod
        if (!coreTasks.empty()) {
            int lastTaskFinish = std::get<2>(coreTasks.back());
            if (lastTaskFinish < hyperperiod) {
                slackIntervals.push_back(std::make_tuple(
                    lastTaskFinish, hyperperiod, std::get<0>(coreTasks.back())));
                std::cout << "[DEBUG] Found end-of-hyperperiod slack for core " << core
                          << " from " << lastTaskFinish << " to " << hyperperiod << std::endl;
            }
        }

        // Process slack intervals in reverse order (from end to beginning)
        for (auto slackIt = slackIntervals.rbegin(); slackIt != slackIntervals.rend(); ++slackIt) {
            int slackStart = std::get<0>(*slackIt);
            int slackEnd = std::get<1>(*slackIt);
            const NameType& precedingTaskId = std::get<2>(*slackIt);

            int slackSize = slackEnd - slackStart;

            if (slackSize <= 0 || precedingTaskId.empty()) continue;

            std::cout << "[DEBUG] Found slack interval [" << slackStart << ", " << slackEnd
                      << "] after task " << precedingTaskId << std::endl;

            // Get task metadata
            TaskMetadata taskMetadata = getTaskMetadata(precedingTaskId);
            if (taskMetadata.originalTaskId.empty()) continue;

            // Get current task execution details
            int taskStart = std::get<1>(dvfsSchedule.at(precedingTaskId));
            int taskFinish = std::get<2>(dvfsSchedule.at(precedingTaskId));
            int taskDuration = taskFinish - taskStart;
            double currentPower = taskMetadata.peakPower;

            // Try each frequency lower than the base frequency
            bool frequencyReduced = false;
            double bestFrequency = taskFrequencies[precedingTaskId];
            int bestExtraTimeNeeded = 0;
            double bestNewPower = currentPower;

            for (auto freqIt = availableFrequencies.rbegin(); freqIt != availableFrequencies.rend(); ++freqIt) {
                double frequency = *freqIt;

                // Skip frequencies higher than or equal to current frequency
                if (frequency >= taskFrequencies[precedingTaskId]) continue;

                // Calculate new execution time at this frequency
                // Execution time is inversely proportional to frequency
                double frequencyRatio = taskFrequencies[precedingTaskId] / frequency;
                int newDuration = static_cast<int>(std::ceil(taskDuration * frequencyRatio));
                int extraTimeNeeded = newDuration - taskDuration;

                // Skip if slack is not big enough
                if (extraTimeNeeded > slackSize) continue;

                // Calculate new power at this frequency
                // Power scales approximately as frequency^2
                double frequencySquaredRatio = std::pow(frequency / taskFrequencies[precedingTaskId], 2);
                double newPower = currentPower * frequencySquaredRatio;

                std::cout << "[DEBUG] Considering reducing " << precedingTaskId
                          << " from " << taskFrequencies[precedingTaskId] << "GHz to "
                          << frequency << "GHz (power: " << currentPower << " -> "
                          << newPower << ")" << std::endl;

                // Check if this change would violate power constraints
                bool powerConstraintViolated = false;

                // Create a temporary copy of the power profile to test changes
                auto tempPpd = combinedPpd;

                // Adjust the power profile for the extended execution
                for (int t = taskStart; t < taskFinish + extraTimeNeeded; t++) {
                    if (t >= static_cast<int>(tempPpd.size())) continue;

                    // Remove the original task's power contribution for this time step
                    if (t < taskFinish) {
                        tempPpd[t][core] -= currentPower;
                    }

                    // Add the new power level
                    tempPpd[t][core] += newPower;

                    // Check total power across all cores at this time step
                    double totalPower = 0.0;
                    for (const auto& corePower : tempPpd[t]) {
                        totalPower += corePower;
                    }

                    if (totalPower > chipTDP) {
                        powerConstraintViolated = true;
                        break;
                    }
                }

                // Skip this frequency if it violates power constraints
                if (powerConstraintViolated) {
                    std::cout << "[DEBUG] Power constraint would be violated, skipping." << std::endl;
                    continue;
                }

                // Check if the extended execution would affect deadline
                bool affectsOtherTasks = false;
                for (const auto& [otherTaskId, otherScheduleInfo] : dvfsSchedule) {
                    if (otherTaskId == precedingTaskId) continue;

                    int otherCore = std::get<0>(otherScheduleInfo);
                    int otherStart = std::get<1>(otherScheduleInfo);

                    // If another task on the same core starts during our extended execution
                    if (otherCore == core && otherStart >= taskFinish && otherStart < taskFinish + extraTimeNeeded) {
                        affectsOtherTasks = true;
                        break;
                    }
                }

                if (affectsOtherTasks) {
                    std::cout << "[DEBUG] Would affect other tasks, skipping." << std::endl;
                    continue;
                }

                // This frequency is valid, update the best frequency if it's lower
                if (frequency < bestFrequency) {
                    bestFrequency = frequency;
                    bestExtraTimeNeeded = extraTimeNeeded;
                    bestNewPower = newPower;
                    frequencyReduced = true;
                }
            }

            // Apply the best frequency found
            if (frequencyReduced) {
                // Apply the frequency reduction
                taskFrequencies[precedingTaskId] = bestFrequency;

                // Update dvfsSchedule with new finish time
                dvfsSchedule[precedingTaskId] = std::make_tuple(
                    std::get<0>(dvfsSchedule[precedingTaskId]),
                    taskStart,
                    taskFinish + bestExtraTimeNeeded
                );

                // Update combinedPpd with new power values
                for (int t = taskStart; t < taskFinish; t++) {
                    if (t < static_cast<int>(combinedPpd.size())) {
                        combinedPpd[t][core] = bestNewPower;
                    }
                }

                // Update the workload distribution matrix for the extended execution
                for (int t = taskFinish; t < taskFinish + bestExtraTimeNeeded; t++) {
                    if (t >= static_cast<int>(combinedWdm.size())) {
                        // Expand WDM and PPD if needed
                        while (t >= static_cast<int>(combinedWdm.size())) {
                            combinedWdm.push_back(std::vector<NameType>());
                            std::vector<double> corePowers(maxCores, 0.0);
                            combinedPpd.push_back(corePowers);
                        }
                    }

                    combinedWdm[t].push_back(precedingTaskId);
                    combinedPpd[t][core] = bestNewPower;
                }

                std::cout << "[DEBUG] Applied frequency reduction for " << precedingTaskId
                          << " to " << bestFrequency << "GHz, extending execution by "
                          << bestExtraTimeNeeded << " time units" << std::endl;
            } else {
                std::cout << "[DEBUG] Could not reduce frequency for task " << precedingTaskId
                          << " without violating constraints" << std::endl;
            }
        }
    }

    // Print frequency assignments and power changes
    std::cout << "\nDetailed Schedule Comparison:" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << std::setw(20) << "Task ID"
              << std::setw(10) << "Core"
              << std::setw(10) << "Start"
              << std::setw(10) << "Finish"
              << std::setw(10) << "Duration"
              << std::setw(15) << "Frequency"
              << std::setw(15) << "Power"
              << std::setw(15) << "Status" << std::endl;
    std::cout << std::string(120, '-') << std::endl;

    // Create a sorted list of tasks for consistent display
    std::vector<NameType> sortedTasks;
    for (const auto& [taskId, _] : dvfsSchedule) {
        sortedTasks.push_back(taskId);
    }
    std::sort(sortedTasks.begin(), sortedTasks.end());

    // Track statistics for summary
    int totalTasks = sortedTasks.size();
    int modifiedTasks = 0;
    int extendedTasks = 0;
    double totalPowerReduction = 0.0;
    double totalTimeExtension = 0.0;

    for (const auto& taskId : sortedTasks) {
        TaskMetadata metadata = getTaskMetadata(taskId);
        auto [origCore, origStart, origFinish] = combinedSchedule.at(taskId);
        auto [newCore, newStart, newFinish] = dvfsSchedule.at(taskId);
        double frequency = taskFrequencies[taskId];

        // Calculate power change
        double originalPower = metadata.peakPower;
        double newPower = originalPower * std::pow(frequency / baseFrequency, 2);
        double powerReduction = originalPower - newPower;

        // Calculate time extension
        int timeExtension = newFinish - origFinish;

        // Update statistics
        if (frequency != baseFrequency) {
            modifiedTasks++;
            totalPowerReduction += powerReduction;
        }
        if (newFinish > origFinish) {
            extendedTasks++;
            totalTimeExtension += timeExtension;
        }

        // Determine if task was modified
        std::string status = "Unchanged";
        if (frequency != baseFrequency) {
            status = "Modified";
        }
        if (newFinish > origFinish) {
            status = "Extended";
        }

        std::cout << std::setw(20) << taskId
                  << std::setw(10) << newCore
                  << std::setw(10) << newStart
                  << std::setw(10) << newFinish
                  << std::setw(10) << (newFinish - newStart)
                  << std::setw(15) << std::fixed << std::setprecision(1) << frequency
                  << std::setw(15) << std::fixed << std::setprecision(1) << newPower
                  << std::setw(15) << status << std::endl;
    }

    // Print summary statistics
    std::cout << "\nSchedule Modification Summary:" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Total Tasks: " << totalTasks << std::endl;
    std::cout << "Modified Tasks: " << modifiedTasks << " ("
              << std::fixed << std::setprecision(1)
              << (static_cast<double>(modifiedTasks) / totalTasks * 100.0) << "%)" << std::endl;
    std::cout << "Extended Tasks: " << extendedTasks << " ("
              << std::fixed << std::setprecision(1)
              << (static_cast<double>(extendedTasks) / totalTasks * 100.0) << "%)" << std::endl;
    std::cout << "Total Power Reduction: " << std::fixed << std::setprecision(2)
              << totalPowerReduction << "W" << std::endl;
    std::cout << "Total Time Extension: " << totalTimeExtension << " time units" << std::endl;
    std::cout << "Average Power Reduction per Modified Task: "
              << std::fixed << std::setprecision(2)
              << (modifiedTasks > 0 ? totalPowerReduction / modifiedTasks : 0.0) << "W" << std::endl;
    std::cout << "Average Time Extension per Extended Task: "
              << std::fixed << std::setprecision(1)
              << (extendedTasks > 0 ? static_cast<double>(totalTimeExtension) / extendedTasks : 0.0)
              << " time units" << std::endl;

    // Print core utilization comparison
    std::cout << "\nCore Utilization Comparison:" << std::endl;
    std::cout << "===========================" << std::endl;
    std::cout << std::setw(10) << "Core"
              << std::setw(15) << "Original Load"
              << std::setw(15) << "New Load"
              << std::setw(15) << "Change" << std::endl;
    std::cout << std::string(55, '-') << std::endl;

    for (int core = 0; core < maxCores; core++) {
        double originalLoad = 0.0;
        double newLoad = 0.0;

        // Calculate loads for this core
        for (const auto& [taskId, scheduleInfo] : combinedSchedule) {
            if (std::get<0>(scheduleInfo) == core) {
                originalLoad += std::get<2>(scheduleInfo) - std::get<1>(scheduleInfo);
            }
        }
        for (const auto& [taskId, scheduleInfo] : dvfsSchedule) {
            if (std::get<0>(scheduleInfo) == core) {
                newLoad += std::get<2>(scheduleInfo) - std::get<1>(scheduleInfo);
            }
        }

        double loadChange = newLoad - originalLoad;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << loadChange << " ("
           << std::fixed << std::setprecision(1) << (loadChange / originalLoad) * 100.0 << "%)";
        std::string changeStr = ss.str();

        std::cout << std::setw(10) << core
                  << std::setw(15) << std::fixed << std::setprecision(1) << originalLoad
                  << std::setw(15) << std::fixed << std::setprecision(1) << newLoad
                  << std::setw(15) << changeStr << std::endl;
    }

    // Update the taskPeakPowerMap with new power values
    for (const auto& [taskId, frequency] : taskFrequencies) {
        TaskMetadata metadata = getTaskMetadata(taskId);
        if (!metadata.originalTaskId.empty()) {
            double originalPower = metadata.peakPower;
            double newPower = originalPower * std::pow(frequency / baseFrequency, 2);
            taskPeakPowerMap[metadata.originalTaskId] = newPower;
        }
    }

    // Print power summary
    std::cout << "\nPower Summary:" << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << std::setw(20) << "Task Type"
              << std::setw(15) << "Original Power"
              << std::setw(15) << "New Power"
              << std::setw(15) << "Reduction" << std::endl;
    std::cout << std::string(75, '-') << std::endl;

    // Group tasks by their base type (without instance numbers)
    std::unordered_map<NameType, std::pair<double, double>> powerChanges;
    for (const auto& [taskId, frequency] : taskFrequencies) {
        TaskMetadata metadata = getTaskMetadata(taskId);
        if (!metadata.originalTaskId.empty()) {
            double originalPower = metadata.peakPower;
            double newPower = originalPower * std::pow(frequency / baseFrequency, 2);
            powerChanges[metadata.originalTaskId] = {originalPower, newPower};
        }
    }

    // Display power changes for each task type
    for (const auto& [taskType, powers] : powerChanges) {
        double originalPower = powers.first;
        double newPower = powers.second;
        double reduction = ((originalPower - newPower) / originalPower) * 100.0;

        std::cout << std::setw(20) << taskType
                  << std::setw(15) << std::fixed << std::setprecision(1) << originalPower
                  << std::setw(15) << std::fixed << std::setprecision(1) << newPower
                  << std::setw(15) << std::fixed << std::setprecision(1) << reduction << "%" << std::endl;
    }

    // Calculate and display overall energy savings
    double originalEnergy = 0.0;
    double newEnergy = 0.0;

    for (size_t t = 0; t < combinedPpd.size(); t++) {
        for (size_t c = 0; c < combinedPpd[t].size(); c++) {
            if (combinedPpd[t][c] > 0) {
                originalEnergy += baseFrequency * baseFrequency;
                newEnergy += combinedPpd[t][c];
            }
        }
    }

    double energySavingsPercent = (1.0 - (newEnergy / originalEnergy)) * 100.0;

    std::cout << "\nOverall Energy Analysis:" << std::endl;
    std::cout << "=======================" << std::endl;
    std::cout << "Original Energy: " << std::fixed << std::setprecision(2) << originalEnergy << " units" << std::endl;
    std::cout << "New Energy: " << std::fixed << std::setprecision(2) << newEnergy << " units" << std::endl;
    std::cout << "Energy Savings: " << std::fixed << std::setprecision(2) << energySavingsPercent << "%" << std::endl;

    return dvfsSchedule;
}

double CPCModel::calculateResponseTime(Applications* app, int numCores, int blockingInterval) {
    if (numCores <= 0) {
        throw std::invalid_argument("Number of cores must be greater than zero.");
    }

    // Calculate total execution time (sum of all task execution times)
    double totalExecutionTime = 0;
    for (const auto& [taskId, taskPtr] : app->tasks) {
        totalExecutionTime += taskPtr->executionTime;
    }

    // Calculate parallelizable portion
    double parallelizablePortion = totalExecutionTime - app->criticalLength;

    // Response time formula: parallelizable_portion/num_cores + critical_length + blocking_interval
    double responseTime = (parallelizablePortion / numCores) + app->criticalLength + blockingInterval;

    return responseTime;
}
