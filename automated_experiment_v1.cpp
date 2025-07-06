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

// Structure to hold experiment results
struct ExperimentResult {
    double schedulabilityRatio;
    double totalExecutionTime;
    double averageExecutionTime;
    double criticalPathLength;
    double hyperperiod;
    int totalTasks;
    int scheduledTasks;
    int makespan;  // Added makespan metric
    std::chrono::milliseconds algorithmRuntime;
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
    std::cout << "Temp DAG saved successfully to " << filename << std::endl;
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
    std::cout << "Temp DAG loaded successfully from " << tempFilename << "\n";
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

// Enhanced experiment runner with detailed metrics
void automated_scheduling_experiment(const std::string& dag_folder,
                                   int iterations = 50,
                                   int num_strategies = 5,
                                   double chipTDP = 60.0) {

    std::cout << "[INFO] Starting enhanced automated scheduling experiment..." << std::endl;
    std::cout << "[INFO] DAG Folder: " << dag_folder << std::endl;
    std::cout << "[INFO] Iterations: " << iterations << std::endl;
    std::cout << "[INFO] Strategies: " << num_strategies << std::endl;
    std::cout << "[INFO] TDP: " << chipTDP << "W" << std::endl;

    // Load base DAGs from folder
    std::vector<Applications*> base_dags = read_dags_from_folder(dag_folder);
    if (base_dags.size() != 3) {
        std::cerr << "[ERROR] Expected 3 DAGs in folder, found " << base_dags.size() << std::endl;
        return;
    }

    std::cout << "[INFO] Successfully loaded " << base_dags.size() << " DAGs" << std::endl;

    // Create temp directory if it doesn't exist
    std::filesystem::create_directories("TempModels");

    // Enhanced results storage
    std::vector<std::vector<ExperimentResult>> results(num_strategies,
        std::vector<ExperimentResult>(iterations));
    std::mutex result_mutex;
    auto start_time = std::chrono::steady_clock::now();

    // Main experiment loop
    for (int iter = 1; iter <= iterations; ++iter) {

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
                auto [coreAssignments, combinedSchedule, combinedWdm, combinedPpd, blockingInterval] =
                    CPCModel::offlineCoreAssignment(5, chipTDP, strat);

                auto strategy_end = std::chrono::high_resolution_clock::now();
                auto runtime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    strategy_end - strategy_start);

                // Calculate comprehensive metrics including makespan
                ExperimentResult result = baseline_metrics;
                result.scheduledTasks = combinedSchedule.size();
                result.schedulabilityRatio = (result.totalTasks > 0) ?
                    (double)result.scheduledTasks / result.totalTasks : 0.0;
                result.makespan = combinedWdm.size();  // Extract makespan from combinedWdm
                result.algorithmRuntime = runtime;

                // Store result
                {
                    std::lock_guard<std::mutex> lock(result_mutex);
                    results[strat][iter - 1] = result;
                }

            } catch (const std::exception& e) {
                auto strategy_end = std::chrono::high_resolution_clock::now();
                auto runtime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    strategy_end - strategy_start);

                std::cerr << "[ERROR] Strategy " << strat << " failed in iteration "
                          << iter << ": " << e.what() << std::endl;

                ExperimentResult failed_result = baseline_metrics;
                failed_result.schedulabilityRatio = 0.0;
                failed_result.scheduledTasks = 0;
                failed_result.makespan = 0;  // Set makespan to 0 for failed results
                failed_result.algorithmRuntime = runtime;

                results[strat][iter - 1] = failed_result;
            }
        }

        // Cleanup temp files
        for (const auto& entry : std::filesystem::directory_iterator("TempModels")) {
            std::filesystem::remove(entry);
        }

        // Progress reporting with more details
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        double per_iter = elapsed / iter;
        double eta = per_iter * (iterations - iter);

        std::cout << "[Progress] Iteration " << iter << "/" << iterations
                  << " (" << std::fixed << std::setprecision(1) << (100.0 * iter / iterations)
                  << "%), Total Exec Time: " << baseline_metrics.totalExecutionTime
                  << ", Avg Critical Path: " << baseline_metrics.criticalPathLength
                  << ", Elapsed: " << (int)elapsed << "s, ETA: " << (int)eta << "s" << std::endl;

        // Free memory for DAG variants
        for (Applications* dag : dags) {
            delete dag;
        }
    }

    // Enhanced statistical analysis and export results
    std::cout << "[INFO] Generating enhanced results and statistical analysis..." << std::endl;

    // Export comprehensive raw data to CSV
    std::ofstream csv("enhanced_scheduling_experiment_results.csv");
    csv << "Strategy,Iteration,SchedulabilityRatio,TotalExecutionTime,AverageExecutionTime,"
        << "CriticalPathLength,Hyperperiod,TotalTasks,ScheduledTasks,Makespan,AlgorithmRuntimeMs\n";

    for (int strat = 0; strat < num_strategies; ++strat) {
        for (int iter = 0; iter < iterations; ++iter) {
            const ExperimentResult& result = results[strat][iter];
            csv << strat << "," << (iter + 1) << "," << std::fixed << std::setprecision(6)
                << result.schedulabilityRatio << "," << result.totalExecutionTime << ","
                << result.averageExecutionTime << "," << result.criticalPathLength << ","
                << result.hyperperiod << "," << result.totalTasks << ","
                << result.scheduledTasks << "," << result.makespan << ","
                << result.algorithmRuntime.count() << "\n";
        }
    }
    csv.close();

    // Enhanced statistical summary
    std::ofstream summary("enhanced_scheduling_experiment_summary.txt");
    summary << "=== Enhanced Automated Scheduling Experiment Summary ===\n";
    summary << "DAG Folder: " << dag_folder << "\n";
    summary << "Iterations: " << iterations << "\n";
    summary << "Strategies: " << num_strategies << "\n";
    summary << "TDP: " << chipTDP << "W\n\n";

    // Schedulability ratio statistics
    summary << "=== SCHEDULABILITY RATIO STATISTICS ===\n";
    summary << "Strategy,Mean,Median,StdDev,Min,Max,95%CI_Lower,95%CI_Upper\n";

    for (int strat = 0; strat < num_strategies; ++strat) {
        std::vector<double> sched_ratios;
        for (int iter = 0; iter < iterations; ++iter) {
            sched_ratios.push_back(results[strat][iter].schedulabilityRatio);
        }

        // Calculate statistics
        double mean = std::accumulate(sched_ratios.begin(), sched_ratios.end(), 0.0) / sched_ratios.size();

        std::vector<double> sorted = sched_ratios;
        std::sort(sorted.begin(), sorted.end());
        double median = sorted[sched_ratios.size() / 2];
        double min_val = sorted.front();
        double max_val = sorted.back();

        double sq_sum = std::inner_product(sched_ratios.begin(), sched_ratios.end(), sched_ratios.begin(), 0.0);
        double stdev = std::sqrt(sq_sum / sched_ratios.size() - mean * mean);

        double ci95_margin = 1.96 * stdev / std::sqrt(sched_ratios.size());
        double ci95_lower = mean - ci95_margin;
        double ci95_upper = mean + ci95_margin;

        summary << strat << "," << std::fixed << std::setprecision(6)
                << mean << "," << median << "," << stdev << ","
                << min_val << "," << max_val << ","
                << ci95_lower << "," << ci95_upper << "\n";
    }

    // Makespan statistics
    summary << "\n=== MAKESPAN STATISTICS ===\n";
    summary << "Strategy,Mean,Median,StdDev,Min,Max\n";

    for (int strat = 0; strat < num_strategies; ++strat) {
        std::vector<double> makespans;
        for (int iter = 0; iter < iterations; ++iter) {
            makespans.push_back(static_cast<double>(results[strat][iter].makespan));
        }

        double mean = std::accumulate(makespans.begin(), makespans.end(), 0.0) / makespans.size();

        std::vector<double> sorted = makespans;
        std::sort(sorted.begin(), sorted.end());
        double median = sorted[makespans.size() / 2];
        double min_val = sorted.front();
        double max_val = sorted.back();

        double sq_sum = std::inner_product(makespans.begin(), makespans.end(), makespans.begin(), 0.0);
        double stdev = std::sqrt(sq_sum / makespans.size() - mean * mean);

        summary << strat << "," << std::fixed << std::setprecision(2)
                << mean << "," << median << "," << stdev << ","
                << min_val << "," << max_val << "\n";
    }

    // Algorithm runtime statistics
    summary << "\n=== ALGORITHM RUNTIME STATISTICS (milliseconds) ===\n";
    summary << "Strategy,Mean,Median,StdDev,Min,Max\n";

    for (int strat = 0; strat < num_strategies; ++strat) {
        std::vector<double> runtimes;
        for (int iter = 0; iter < iterations; ++iter) {
            runtimes.push_back(static_cast<double>(results[strat][iter].algorithmRuntime.count()));
        }

        double mean = std::accumulate(runtimes.begin(), runtimes.end(), 0.0) / runtimes.size();

        std::vector<double> sorted = runtimes;
        std::sort(sorted.begin(), sorted.end());
        double median = sorted[runtimes.size() / 2];
        double min_val = sorted.front();
        double max_val = sorted.back();

        double sq_sum = std::inner_product(runtimes.begin(), runtimes.end(), runtimes.begin(), 0.0);
        double stdev = std::sqrt(sq_sum / runtimes.size() - mean * mean);

        summary << strat << "," << std::fixed << std::setprecision(2)
                << mean << "," << median << "," << stdev << ","
                << min_val << "," << max_val << "\n";
    }

    // Execution time correlation analysis
    summary << "\n=== EXECUTION TIME ANALYSIS ===\n";
    summary << "Analyzing correlation between total execution time and schedulability...\n\n";

    for (int strat = 0; strat < num_strategies; ++strat) {
        std::vector<double> exec_times, sched_ratios;
        for (int iter = 0; iter < iterations; ++iter) {
            exec_times.push_back(results[strat][iter].totalExecutionTime);
            sched_ratios.push_back(results[strat][iter].schedulabilityRatio);
        }

        // Simple correlation coefficient
        double mean_exec = std::accumulate(exec_times.begin(), exec_times.end(), 0.0) / exec_times.size();
        double mean_sched = std::accumulate(sched_ratios.begin(), sched_ratios.end(), 0.0) / sched_ratios.size();

        double numerator = 0.0, denom_exec = 0.0, denom_sched = 0.0;
        for (size_t i = 0; i < exec_times.size(); ++i) {
            double exec_diff = exec_times[i] - mean_exec;
            double sched_diff = sched_ratios[i] - mean_sched;
            numerator += exec_diff * sched_diff;
            denom_exec += exec_diff * exec_diff;
            denom_sched += sched_diff * sched_diff;
        }

        double correlation = (denom_exec > 0 && denom_sched > 0) ?
            numerator / std::sqrt(denom_exec * denom_sched) : 0.0;

        summary << "Strategy " << strat << " - Execution Time vs Schedulability Correlation: "
                << std::fixed << std::setprecision(4) << correlation << "\n";
    }

    summary.close();

    // Cleanup base DAGs
    for (Applications* dag : base_dags) {
        delete dag;
    }

    // Remove temp directory
    std::filesystem::remove_all("TempModels");

    auto end_time = std::chrono::steady_clock::now();
    double total_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

    std::cout << "[DONE] Enhanced automated scheduling experiment completed successfully!" << std::endl;
    std::cout << "[INFO] Total execution time: " << (int)total_time << " seconds" << std::endl;
    std::cout << "[INFO] Results saved to:" << std::endl;
    std::cout << "       - enhanced_scheduling_experiment_results.csv (comprehensive raw data)" << std::endl;
    std::cout << "       - enhanced_scheduling_experiment_summary.txt (detailed statistics)" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <dag_folder> [iterations] [num_strategies] [tdp]" << std::endl;
        std::cout << "Example: " << argv[0] << " ./DAGs 50 5 60.0" << std::endl;
        return 1;
    }

    std::string dag_folder = argv[1];
    int iterations = (argc > 2) ? std::stoi(argv[2]) : 50;
    int num_strategies = (argc > 3) ? std::stoi(argv[3]) : 5;
    double tdp = (argc > 4) ? std::stod(argv[4]) : 60.0;

    automated_scheduling_experiment(dag_folder, iterations, num_strategies, tdp);
    return 0;
}
