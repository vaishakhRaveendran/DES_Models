#include "cpc.h"
#include <random>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <mutex>

// Updated ExperimentResult structure - ADD this field
struct ExperimentResult {
    double successRate;
    double totalExecutionTime;
    double averageExecutionTime;
    double criticalPathLength;
    double hyperperiod;
    int totalTasks;
    int scheduledTasks;
    int makespan;
    std::chrono::milliseconds algorithmRuntime;
    double tdp;
    int strategy;
    double appSetSchedulability; // NEW FIELD - 1.0 if successRate == 1.0, else 0.0
};

// Updated AggregatedResult structure - ADD these fields
struct AggregatedResult {
    double meanSuccessRate;
    double stdSuccessRate;
    double meanMakespan;
    double stdMakespan;
    double meanRuntime;
    double stdRuntime;
    double tdp;
    int strategy;
    int totalIterations;
    double ci95_lower_success;
    double ci95_upper_success;
    double ci95_lower_makespan;
    double ci95_upper_makespan;
    // NEW FIELDS for Application Set Schedulability
    double meanAppSetSchedulability;
    double stdAppSetSchedulability;
    double ci95_lower_appset;
    double ci95_upper_appset;
};

// Temp DAG save function (standalone)
void saveTempDAG(Applications* app, const std::string& filename) {
    if (!app) {
        std::cerr << "Invalid Applications pointer provided.\n";
        return;
    }

    std::string tempFilename = "TempModels/" + filename + ".dag";
    std::ofstream file(tempFilename);
    if (!file) {
        std::cerr << "Error opening file for writing temp DAG data.\n";
        return;
    }
    file << app->appName << "\n";
    file << app->tasks.size() << "\n";
    file << app->startState << "\n";
    file << app->relativeDeadline << "\n";
    file << app->period << "\n";
    for (const auto& [key, task] : app->tasks) {
        file << task->id << " " << task->marked << " " << task->executionTime << "\n";
        file << task->transitions.size() << "\n";
        for (const auto& transition : task->transitions) {
            file << transition.targetState << " " << transition.label << " " << transition.isControlled << "\n";
        }
    }
    file.close();
}

// Temp DAG read function (standalone)
Applications* readTempDAG(const std::string& filename) {
    std::string tempFilename = "TempModels/" + filename + ".dag";
    std::ifstream file(tempFilename);

    if (!file) {
        std::cerr << "Error opening file: " << tempFilename << " for reading temp DAG data.\n";
        return nullptr;
    }

    Applications* app = new Applications();
    int numTasks;
    if (!(file >> app->appName)) {
        std::cerr << "Error reading name of application from temp file.\n";
        delete app;
        return nullptr;
    }

    if (!(file >> numTasks)) {
        std::cerr << "Error reading number of tasks from temp file.\n";
        delete app;
        return nullptr;
    }

    if (!(file >> app->startState)) {
        std::cerr << "Error start state of applications from temp file.\n";
        delete app;
        return nullptr;
    }

    if (!(file>>app->relativeDeadline)) {
        std::cerr << "Error relativeDeadline of application from temp file.\n";
        delete app;
        return nullptr;
    }

    if (!(file>>app->period)) {
        std::cerr << "Error reading period of application from temp file.\n";
        delete app;
        return nullptr;
    }

    for (int i = 0; i < numTasks; i++) {
        NameType taskId;
        bool isMarked;
        int executionTime;
        if (!(file >> taskId >> isMarked >> executionTime)) {
            std::cerr << "Error reading task data from temp file.\n";
            delete app;
            return nullptr;
        }
        app->addTask(taskId,executionTime,isMarked);

        int numTransitions;
        if (!(file >> numTransitions)) {
            std::cerr << "Error reading number of transitions for task " << taskId << "\n";
            delete app;
            return nullptr;
        }

        for (int j = 0; j < numTransitions; j++) {
            NameType target, label;
            bool isControlled;
            if (!(file >> target >> label >> isControlled)) {
                std::cerr << "Error reading transition data for task " << taskId << "\n";
                delete app;
                return nullptr;
            }
            app->tasks[taskId]->addTransition(target, label, isControlled);
        }
    }

    file.close();
    return app;
}

// Helper: Read DAGs from folder (returns vector of Applications*)
std::vector<Applications*> read_dags_from_folder(const std::string& folder) {
    std::vector<Applications*> dags;
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.path().extension() == ".dag") {
            try {
                Applications* app = DAG::readDAGFromFile(entry.path().stem().string());
                if (app) {
                    dags.push_back(app);
                }
            } catch (...) {
                std::cerr << "Error loading DAG: " << entry.path().filename() << std::endl;
            }
        }
    }
    return dags;
}

// Calculate execution time metrics for a set of DAGs
ExperimentResult calculateDAGMetrics(const std::vector<Applications*>& dags) {
    ExperimentResult metrics = {};

    int totalTasks = 0;
    double totalExecTime = 0.0;
    double totalCriticalPath = 0.0;
    double totalHyperperiod = 0.0;

    for (const Applications* dag : dags) {
        totalTasks += dag->tasks.size();
        totalHyperperiod += dag->period;

        // Calculate total execution time for this DAG
        double dagExecTime = 0.0;
        for (const auto& [tid, task] : dag->tasks) {
            dagExecTime += task->executionTime;
        }
        totalExecTime += dagExecTime;

        // Use the critical path length (assuming it's calculated and stored)
        totalCriticalPath += dag->criticalLength;
    }

    metrics.totalTasks = totalTasks;
    metrics.totalExecutionTime = totalExecTime;
    metrics.averageExecutionTime = totalTasks > 0 ? totalExecTime / totalTasks : 0.0;
    metrics.criticalPathLength = totalCriticalPath / dags.size(); // Average critical path
    metrics.hyperperiod = totalHyperperiod / dags.size(); // Average hyperperiod

    return metrics;
}

// Enhanced DAG variant generator with more variation strategies
std::vector<Applications*> generate_dag_variants(
    const std::vector<Applications*>& base_dags, int iteration) {

    std::vector<Applications*> variants;

    for (size_t dag_idx = 0; dag_idx < base_dags.size(); ++dag_idx) {
        Applications* orig = base_dags[dag_idx];
        Applications* variant = new Applications(*orig); // Deep copy

        if (iteration == 1) {
            // Use original execution times for first iteration
            std::vector<NameType> critical_path = CPCModel::findCriticalPath(variant);
            int critical_path_length = variant->criticalLength;

            int hyperperiod = variant->period;
            if (critical_path_length >= hyperperiod) {
                variant->relativeDeadline = hyperperiod;
            } else {
                variant->relativeDeadline = (critical_path_length + hyperperiod) / 2;
            }

            variants.push_back(variant);
            continue;
        }

        // Enhanced variation with multiple strategies
        std::mt19937 rng(iteration * 1000 + dag_idx);

        // Choose variation strategy based on iteration
        int strategy = (iteration - 1) % 4; // 4 different variation strategies

        std::vector<std::string> tids;
        std::vector<int> orig_times;

        for (const auto& [tid, tptr] : variant->tasks) {
            tids.push_back(tid);
            orig_times.push_back(tptr->executionTime);
        }

        if (tids.empty()) {
            variants.push_back(variant);
            continue;
        }

        int hyperperiod = variant->period;
        std::vector<int> new_times;

        switch (strategy) {
            case 0: { // Uniform variation (50%-150%)
                std::uniform_real_distribution<double> dist(0.5, 1.5);
                for (int orig_time : orig_times) {
                    new_times.push_back(std::max(1, static_cast<int>(orig_time * dist(rng))));
                }
                break;
            }
            case 1: { // Bimodal variation (either 70% or 130%)
                std::uniform_int_distribution<int> choice(0, 1);
                for (int orig_time : orig_times) {
                    double factor = choice(rng) ? 0.7 : 1.3;
                    new_times.push_back(std::max(1, static_cast<int>(orig_time * factor)));
                }
                break;
            }
            case 2: { // Heavy-tail variation (some tasks get much longer)
                std::uniform_real_distribution<double> uniform(0.0, 1.0);
                for (int orig_time : orig_times) {
                    double factor;
                    if (uniform(rng) < 0.8) {
                        factor = 0.8 + 0.4 * uniform(rng); // 80%-120% for 80% of tasks
                    } else {
                        factor = 1.5 + 1.0 * uniform(rng); // 150%-250% for 20% of tasks
                    }
                    new_times.push_back(std::max(1, static_cast<int>(orig_time * factor)));
                }
                break;
            }
            case 3: { // Critical path focused variation
                // First identify critical path tasks
                std::vector<NameType> critical_path = CPCModel::findCriticalPath(variant);
                std::set<std::string> critical_tasks(critical_path.begin(), critical_path.end());

                std::uniform_real_distribution<double> crit_dist(0.8, 1.2); // Less variation for critical tasks
                std::uniform_real_distribution<double> non_crit_dist(0.5, 1.8); // More variation for non-critical

                for (size_t i = 0; i < tids.size(); ++i) {
                    double factor;
                    if (critical_tasks.count(tids[i])) {
                        factor = crit_dist(rng);
                    } else {
                        factor = non_crit_dist(rng);
                    }
                    new_times.push_back(std::max(1, static_cast<int>(orig_times[i] * factor)));
                }
                break;
            }
        }

        // Scale down if total exceeds hyperperiod
        int total_new_time = std::accumulate(new_times.begin(), new_times.end(), 0);
        if (total_new_time > hyperperiod) {
            double scale_factor = static_cast<double>(hyperperiod - tids.size()) / (total_new_time - tids.size());
            total_new_time = 0;
            for (size_t i = 0; i < new_times.size(); ++i) {
                int excess = new_times[i] - 1;
                new_times[i] = 1 + static_cast<int>(excess * scale_factor);
                total_new_time += new_times[i];
            }

            while (total_new_time > hyperperiod) {
                auto max_it = std::max_element(new_times.begin(), new_times.end());
                if (*max_it > 1) {
                    (*max_it)--;
                    total_new_time--;
                } else {
                    break;
                }
            }
        }

        // Update tasks with new execution times
        for (size_t i = 0; i < tids.size(); ++i) {
            variant->tasks[tids[i]]->executionTime = new_times[i];
        }

        // Recalculate critical path and set deadline
        std::vector<NameType> critical_path = CPCModel::findCriticalPath(variant);
        int critical_path_length = variant->criticalLength;

        if (critical_path_length >= hyperperiod) {
            variant->relativeDeadline = hyperperiod;
        } else {
            // More varied deadline assignment
            std::uniform_real_distribution<double> deadline_factor(0.7, 0.95);
            int deadline_range = hyperperiod - critical_path_length;
            variant->relativeDeadline = critical_path_length +
                static_cast<int>(deadline_range * deadline_factor(rng));
        }

        variants.push_back(variant);
    }

    return variants;
}

// Function to calculate statistics
void calculateStatistics(const std::vector<double>& values, double& mean, double& stddev,
                        double& ci95_lower, double& ci95_upper) {
    if (values.empty()) {
        mean = stddev = ci95_lower = ci95_upper = 0.0;
        return;
    }

    mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();

    double sq_sum = std::inner_product(values.begin(), values.end(), values.begin(), 0.0);
    stddev = std::sqrt(sq_sum / values.size() - mean * mean);

    double ci95_margin = 1.96 * stddev / std::sqrt(values.size());
    ci95_lower = mean - ci95_margin;
    ci95_upper = mean + ci95_margin;
}

// Enhanced TDP-varying experiment runner
void tdp_varying_scheduling_experiment(const std::string& dag_folder,
                                     double min_tdp = 80.0,
                                     double max_tdp = 200.0,
                                     double tdp_step = 1.0,
                                     int iterations_per_tdp = 3,
                                     int num_strategies = 5) {

    std::cout << "[INFO] Starting TDP-varying automated scheduling experiment..." << std::endl;
    std::cout << "[INFO] DAG Folder: " << dag_folder << std::endl;
    std::cout << "[INFO] TDP Range: " << min_tdp << "W to " << max_tdp << "W (step: " << tdp_step << "W)" << std::endl;
    std::cout << "[INFO] Iterations per TDP: " << iterations_per_tdp << std::endl;
    std::cout << "[INFO] Strategies: " << num_strategies << std::endl;

    // Load base DAGs from folder
    std::vector<Applications*> base_dags = read_dags_from_folder(dag_folder);
    if (base_dags.size() != 5) {
        std::cerr << "[ERROR] Expected 5 DAGs in folder, found " << base_dags.size() << std::endl;
        return;
    }

    std::cout << "[INFO] Successfully loaded " << base_dags.size() << " DAGs" << std::endl;

    // Create temp directory if it doesn't exist
    std::filesystem::create_directories("TempModels");

    // Calculate TDP values
    std::vector<double> tdp_values;
    for (double tdp = min_tdp; tdp <= max_tdp + 0.001; tdp += tdp_step) {
        tdp_values.push_back(tdp);
    }

    std::cout << "[INFO] Will test " << tdp_values.size() << " TDP values" << std::endl;

    // Storage for all results
    std::vector<ExperimentResult> all_results;
    std::vector<AggregatedResult> aggregated_results;

    auto experiment_start = std::chrono::steady_clock::now();
    int total_experiments = tdp_values.size() * iterations_per_tdp * num_strategies;
    int completed_experiments = 0;

    // Main experiment loop - iterate over TDP values
    for (size_t tdp_idx = 0; tdp_idx < tdp_values.size(); ++tdp_idx) {
        double current_tdp = tdp_values[tdp_idx];

        std::cout << "\n[TDP " << (tdp_idx + 1) << "/" << tdp_values.size()
                  << "] Testing TDP = " << current_tdp << "W" << std::endl;

        // Storage for this TDP's results
        std::vector<std::vector<ExperimentResult>> tdp_results(num_strategies,
            std::vector<ExperimentResult>(iterations_per_tdp));

        // Run iterations for this TDP
        for (int iter = 1; iter <= iterations_per_tdp; ++iter) {

            // Generate DAG variants for this iteration
            std::vector<Applications*> dags = generate_dag_variants(base_dags, iter);

            // Calculate baseline metrics for this iteration
            ExperimentResult baseline_metrics = calculateDAGMetrics(dags);

            // Write variants to temp folder for scheduling system to pick up
            for (Applications* dag : dags) {
                saveTempDAG(dag, dag->appName);
            }

            // Test each scheduling strategy
                for (int strat = 0; strat < num_strategies; ++strat) {
                    auto strategy_start = std::chrono::high_resolution_clock::now();

                    try {
                        // Use CPCModel::offlineCoreAssignment as the main scheduling function
                        auto [coreAssignments, combinedSchedule, combinedWdm, combinedPpd, blockingInterval, successRate] =
                            CPCModel::offlineCoreAssignment(9, current_tdp, strat);

                        auto strategy_end = std::chrono::high_resolution_clock::now();
                        auto runtime = std::chrono::duration_cast<std::chrono::milliseconds>(
                            strategy_end - strategy_start);

                        // Use the returned successRate directly instead of calculating schedulability ratio
                        ExperimentResult result = baseline_metrics;
                        result.scheduledTasks = combinedSchedule.size();
                        result.successRate = successRate;
                        result.makespan = combinedWdm.size();
                        result.algorithmRuntime = runtime;
                        result.tdp = current_tdp;
                        result.strategy = strat;
                        // NEW: Calculate Application Set Schedulability
                        result.appSetSchedulability = (successRate >= 1.0) ? 1.0 : 0.0;

                        // Store result
                        tdp_results[strat][iter - 1] = result;
                        all_results.push_back(result);

                    } catch (const std::exception& e) {
                        auto strategy_end = std::chrono::high_resolution_clock::now();
                        auto runtime = std::chrono::duration_cast<std::chrono::milliseconds>(
                            strategy_end - strategy_start);

                        std::cerr << "[ERROR] Strategy " << strat << " failed in iteration "
                                  << iter << " for TDP " << current_tdp << ": " << e.what() << std::endl;

                        ExperimentResult failed_result = baseline_metrics;
                        failed_result.successRate = 0.0;
                        failed_result.scheduledTasks = 0;
                        failed_result.makespan = 0;
                        failed_result.algorithmRuntime = runtime;
                        failed_result.tdp = current_tdp;
                        failed_result.strategy = strat;
                        // NEW: Failed scheduling means app set schedulability is 0
                        failed_result.appSetSchedulability = 0.0;

                        tdp_results[strat][iter - 1] = failed_result;
                        all_results.push_back(failed_result);
                    }

                    completed_experiments++;
                }

            // Cleanup temp files after each iteration
            for (const auto& entry : std::filesystem::directory_iterator("TempModels")) {
                std::filesystem::remove(entry);
            }

            // Free memory for DAG variants
            for (Applications* dag : dags) {
                delete dag;
            }

            // Progress reporting
            if (iter % 10 == 0 || iter == iterations_per_tdp) {
                auto now = std::chrono::steady_clock::now();
                double elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - experiment_start).count();
                double progress = (double)completed_experiments / total_experiments;
                double eta = elapsed * (1.0 - progress) / progress;

                double tdp_progress = (double)iter / iterations_per_tdp * 100.0;
                double overall_progress = ((double)tdp_idx * iterations_per_tdp + iter) /
                                        (tdp_values.size() * iterations_per_tdp) * 100.0;

                std::cout << "[Progress] TDP " << current_tdp << "W: " << iter << "/" << iterations_per_tdp
                          << " (" << std::fixed << std::setprecision(1) << tdp_progress << "%), "
                          << "Overall: " << std::setprecision(1) << overall_progress << "%, "
                          << "ETA: " << (int)eta << "s" << std::endl;
            }
        }

        // Calculate aggregated statistics for this TDP
        for (int strat = 0; strat < num_strategies; ++strat) {
            std::vector<double> success_rates, makespans, runtimes, app_set_schedulabilities;

            for (int iter = 0; iter < iterations_per_tdp; ++iter) {
                const auto& result = tdp_results[strat][iter];
                success_rates.push_back(result.successRate);
                makespans.push_back(static_cast<double>(result.makespan));
                runtimes.push_back(static_cast<double>(result.algorithmRuntime.count()));
                app_set_schedulabilities.push_back(result.appSetSchedulability); // NEW
            }

            AggregatedResult agg_result;
            agg_result.tdp = current_tdp;
            agg_result.strategy = strat;
            agg_result.totalIterations = iterations_per_tdp;

            // Calculate success rate statistics
            calculateStatistics(success_rates, agg_result.meanSuccessRate,
                              agg_result.stdSuccessRate,
                              agg_result.ci95_lower_success, agg_result.ci95_upper_success);

            // Calculate makespan statistics
            calculateStatistics(makespans, agg_result.meanMakespan,
                              agg_result.stdMakespan,
                              agg_result.ci95_lower_makespan, agg_result.ci95_upper_makespan);

            // Calculate runtime statistics
            double dummy_lower, dummy_upper;
            calculateStatistics(runtimes, agg_result.meanRuntime,
                              agg_result.stdRuntime, dummy_lower, dummy_upper);

            // NEW: Calculate Application Set Schedulability statistics
            calculateStatistics(app_set_schedulabilities, agg_result.meanAppSetSchedulability,
                              agg_result.stdAppSetSchedulability,
                              agg_result.ci95_lower_appset, agg_result.ci95_upper_appset);

            aggregated_results.push_back(agg_result);
        }


        std::cout << "[TDP Complete] TDP " << current_tdp << "W completed with "
                  << iterations_per_tdp << " iterations per strategy" << std::endl;
    }

    // Export comprehensive results
    std::cout << "\n[INFO] Generating comprehensive results and analysis..." << std::endl;

    // Export raw data (updated column names)
    std::ofstream raw_csv("tdp_experiment_raw_results.csv");
    raw_csv << "TDP,Strategy,Iteration,SuccessRate,AppSetSchedulability,TotalExecutionTime,AverageExecutionTime,"
            << "CriticalPathLength,Hyperperiod,TotalTasks,ScheduledTasks,Makespan,AlgorithmRuntimeMs\n";

    int iteration_counter = 0;
    for (const auto& result : all_results) {
        raw_csv << std::fixed << std::setprecision(2) << result.tdp << ","
                << result.strategy << "," << (iteration_counter % iterations_per_tdp + 1) << ","
                << std::setprecision(6) << result.successRate << ","
                << std::setprecision(1) << result.appSetSchedulability << "," // NEW COLUMN
                << result.totalExecutionTime << "," << result.averageExecutionTime << ","
                << result.criticalPathLength << "," << result.hyperperiod << ","
                << result.totalTasks << "," << result.scheduledTasks << ","
                << result.makespan << "," << result.algorithmRuntime.count() << "\n";
        iteration_counter++;
    }
    raw_csv.close();

    // Export aggregated results (updated column names)
    std::ofstream agg_csv("tdp_experiment_aggregated_results.csv");
    agg_csv << "TDP,Strategy,MeanSuccessRate,StdSuccessRate,CI95_Lower_Success,CI95_Upper_Success,"
            << "MeanAppSetSchedulability,StdAppSetSchedulability,CI95_Lower_AppSet,CI95_Upper_AppSet," // NEW COLUMNS
            << "MeanMakespan,StdMakespan,CI95_Lower_Makespan,CI95_Upper_Makespan,"
            << "MeanRuntimeMs,StdRuntimeMs,TotalIterations\n";

    for (const auto& agg : aggregated_results) {
        agg_csv << std::fixed << std::setprecision(2) << agg.tdp << ","
                << agg.strategy << "," << std::setprecision(6)
                << agg.meanSuccessRate << "," << agg.stdSuccessRate << ","
                << agg.ci95_lower_success << "," << agg.ci95_upper_success << ","
                << std::setprecision(6) << agg.meanAppSetSchedulability << "," << agg.stdAppSetSchedulability << "," // NEW COLUMNS
                << agg.ci95_lower_appset << "," << agg.ci95_upper_appset << ","
                << std::setprecision(3) << agg.meanMakespan << "," << agg.stdMakespan << ","
                << agg.ci95_lower_makespan << "," << agg.ci95_upper_makespan << ","
                << std::setprecision(2) << agg.meanRuntime << "," << agg.stdRuntime << ","
                << agg.totalIterations << "\n";
    }
    agg_csv.close();

    // Generate plotting-ready data files (updated file names)
    std::ofstream plot_success("plotting_success_rate.csv");  // Changed from plotting_schedulability.csv
    plot_success << "TDP,Strategy0,Strategy1,Strategy2,Strategy3,Strategy4\n";

    std::ofstream plot_makespan("plotting_makespan.csv");
    plot_makespan << "TDP,Strategy0,Strategy1,Strategy2,Strategy3,Strategy4\n";

    for (double tdp : tdp_values) {
        plot_success << std::fixed << std::setprecision(2) << tdp;
        plot_makespan << std::fixed << std::setprecision(2) << tdp;

        for (int strat = 0; strat < num_strategies; ++strat) {
            // Find aggregated result for this TDP and strategy
            auto it = std::find_if(aggregated_results.begin(), aggregated_results.end(),
                [tdp, strat](const AggregatedResult& r) {
                    return std::abs(r.tdp - tdp) < 0.001 && r.strategy == strat;
                });

            if (it != aggregated_results.end()) {
                plot_success << "," << std::setprecision(6) << it->meanSuccessRate;  // Use meanSuccessRate
                plot_makespan << "," << std::setprecision(3) << it->meanMakespan;
            } else {
                plot_success << ",0.0";
                plot_makespan << ",0.0";
            }
        }
        plot_success << "\n";
        plot_makespan << "\n";
    }
    plot_success.close();
    plot_makespan.close();


    // Generate plotting-ready data files for Application Set Schedulability (NEW)
    std::ofstream plot_appset("plotting_app_set_schedulability.csv");
    plot_appset << "TDP,Strategy0,Strategy1,Strategy2,Strategy3,Strategy4\n";

    for (double tdp : tdp_values) {
        plot_appset << std::fixed << std::setprecision(2) << tdp;

        for (int strat = 0; strat < num_strategies; ++strat) {
            // Find aggregated result for this TDP and strategy
            auto it = std::find_if(aggregated_results.begin(), aggregated_results.end(),
                [tdp, strat](const AggregatedResult& r) {
                    return std::abs(r.tdp - tdp) < 0.001 && r.strategy == strat;
                });

            if (it != aggregated_results.end()) {
                plot_appset << "," << std::setprecision(6) << it->meanAppSetSchedulability;
            } else {
                plot_appset << ",0.0";
            }
        }
        plot_appset << "\n";
    }
    plot_appset.close();

    // Generate comprehensive summary (updated text)
    std::ofstream summary("tdp_experiment_summary.txt");
    summary << "=== TDP-Varying Automated Scheduling Experiment Summary ===\n";
    summary << "DAG Folder: " << dag_folder << "\n";
    summary << "TDP Range: " << min_tdp << "W to " << max_tdp << "W (step: " << tdp_step << "W)\n";
    summary << "Iterations per TDP: " << iterations_per_tdp << "\n";
    summary << "Number of Strategies: " << num_strategies << "\n";
    summary << "Total TDP Values Tested: " << tdp_values.size() << "\n";
    summary << "Total Experiments: " << total_experiments << "\n\n";

    // Best performing strategies analysis (updated to use success rate)
    summary << "=== BEST PERFORMING STRATEGIES BY TDP ===\n";
    summary << "TDP,BestStrategy_SuccessRate,BestSuccessRate,BestStrategy_Makespan,BestMakespan\n";  // Updated column names

    for (double tdp : tdp_values) {
        double best_success_rate = -1.0;  // Changed from best_sched_ratio
        int best_success_strategy = -1;   // Changed from best_sched_strategy
        double best_makespan = std::numeric_limits<double>::max();
        int best_makespan_strategy = -1;

        for (int strat = 0; strat < num_strategies; ++strat) {
            auto it = std::find_if(aggregated_results.begin(), aggregated_results.end(),
                [tdp, strat](const AggregatedResult& r) {
                    return std::abs(r.tdp - tdp) < 0.001 && r.strategy == strat;
                });

            if (it != aggregated_results.end()) {
                if (it->meanSuccessRate > best_success_rate) {  // Use meanSuccessRate
                    best_success_rate = it->meanSuccessRate;
                    best_success_strategy = strat;
                }
                if (it->meanMakespan < best_makespan && it->meanMakespan > 0) {
                    best_makespan = it->meanMakespan;
                    best_makespan_strategy = strat;
                }
            }
        }

        summary << std::fixed << std::setprecision(2) << tdp << ","
                << best_success_strategy << "," << std::setprecision(6) << best_success_rate << ","
                << best_makespan_strategy << "," << std::setprecision(3) << best_makespan << "\n";
    }

    summary.close();

    // Cleanup base DAGs
    for (Applications* dag : base_dags) {
        delete dag;
    }

    // Remove temp directory
    std::filesystem::remove_all("TempModels");

    auto experiment_end = std::chrono::steady_clock::now();
    double total_time = std::chrono::duration_cast<std::chrono::seconds>(experiment_end - experiment_start).count();

    std::cout << "\n[DONE] TDP-varying automated scheduling experiment completed successfully!" << std::endl;
    std::cout << "[INFO] Total execution time: " << (int)total_time << " seconds ("
              << std::setprecision(2) << total_time/60.0 << " minutes)" << std::endl;
    std::cout << "[INFO] Total experiments conducted: " << total_experiments << std::endl;
    // UPDATE the final console output section to include the new file:

    std::cout << "[INFO] Results saved to:" << std::endl;
    std::cout << "       - tdp_experiment_raw_results.csv (all individual results)" << std::endl;
    std::cout << "       - tdp_experiment_aggregated_results.csv (statistical summaries)" << std::endl;
    std::cout << "       - plotting_success_rate.csv (ready for plotting success rate)" << std::endl;
    std::cout << "       - plotting_app_set_schedulability.csv (ready for plotting application set schedulability)" << std::endl; // NEW
    std::cout << "       - plotting_makespan.csv (ready for plotting makespan)" << std::endl;
    std::cout << "       - tdp_experiment_summary.txt (comprehensive analysis)" << std::endl;
    std::cout << "\n[PLOTTING GUIDE]" << std::endl;
    std::cout << "For plotting success rate vs TDP:" << std::endl;
    std::cout << "  - X-axis: TDP values (" << min_tdp << "W to " << max_tdp << "W)" << std::endl;
    std::cout << "  - Y-axis: Mean Success Rate (0.0 to 1.0+)" << std::endl;
    std::cout << "  - Different curves for each strategy (0 to " << (num_strategies-1) << ")" << std::endl;
    std::cout << "For plotting application set schedulability vs TDP:" << std::endl; // NEW
    std::cout << "  - X-axis: TDP values (" << min_tdp << "W to " << max_tdp << "W)" << std::endl;
    std::cout << "  - Y-axis: Application Set Schedulability Percentage (0.0 to 1.0)" << std::endl;
    std::cout << "  - Different curves for each strategy (0 to " << (num_strategies-1) << ")" << std::endl;
    std::cout << "For plotting makespan vs TDP:" << std::endl;
    std::cout << "  - X-axis: TDP values (" << min_tdp << "W to " << max_tdp << "W)" << std::endl;
    std::cout << "  - Y-axis: Mean Makespan" << std::endl;
    std::cout << "  - Different curves for each strategy (0 to " << (num_strategies-1) << ")" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <dag_folder> [min_tdp] [max_tdp] [tdp_step] [iterations_per_tdp] [num_strategies]" << std::endl;
        std::cout << "Example: " << argv[0] << " ./DAGs 20.0 80.0 5.0 100 5" << std::endl;
        std::cout << "Default values: min_tdp=20.0, max_tdp=80.0, tdp_step=5.0, iterations_per_tdp=100, num_strategies=5" << std::endl;
        return 1;
    }

    std::string dag_folder = argv[1];
    double min_tdp = (argc > 2) ? std::stod(argv[2]) : 10.0;
    double max_tdp = (argc > 3) ? std::stod(argv[3]) : 50.0;
    double tdp_step = (argc > 4) ? std::stod(argv[4]) : 20.0;
    int iterations_per_tdp = (argc > 5) ? std::stoi(argv[5]) : 50;
    int num_strategies = (argc > 6) ? std::stoi(argv[6]) : 5;

    // Validate parameters
    if (min_tdp >= max_tdp) {
        std::cerr << "Error: min_tdp must be less than max_tdp" << std::endl;
        return 1;
    }
    if (tdp_step <= 0) {
        std::cerr << "Error: tdp_step must be positive" << std::endl;
        return 1;
    }
    if (iterations_per_tdp <= 0) {
        std::cerr << "Error: iterations_per_tdp must be positive" << std::endl;
        return 1;
    }

    tdp_varying_scheduling_experiment(dag_folder, min_tdp, max_tdp, tdp_step, iterations_per_tdp, num_strategies);
    return 0;
}
