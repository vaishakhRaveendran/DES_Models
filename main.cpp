#include <iostream>
#include <cstdlib>
#include "models.h"
#include "dags.h"
#include "graphs.h"
#include "applications.h"
#include "taskExecutionModels.h"
#include "timingSpecificationModels.h"
#include "parallel.h"
#include "sct.h"
#include "cpc.h"
#include "resources.h"
#include <thread>
#include <chrono>

void printBox(const std::string& text) {
    int len = text.length();
    std::cout << "\u2554";
    for (int i = 0; i < len + 2; i++) std::cout << "\u2550";
    std::cout << "\u2557\n\u2551 " << text << " \u2551\n\u255a";
    for (int i = 0; i < len + 2; i++) std::cout << "\u2550";
    std::cout << "\u255d\n";
}

void printMainMenu() {
    system("clear");
    printBox("Application Manager");
    std::cout << "\n";
    std::cout << "\u250c" << "──────────────────────────" << "\u2510\n";
    std::cout << "│ [1] Create Application   │\n";
    std::cout << "│ [2] Generate Image (APP) │\n";
    std::cout << "│ [3] Generate Image (DES) │\n";
    std::cout << "│ [4] Save to File (APP)   │\n";
    std::cout << "│ [5] Save to File (DES)   │\n";
    std::cout << "│ [6] Load from File (APP) │\n";
    std::cout << "│ [7] Load from File (DES) │\n";
    std::cout << "│ [8] Task Exec Models     │\n";
    std::cout << "│ [9] Time Spec Models     │\n";
    std::cout << "│ [#] Resoruce Constraints │\n";
    std::cout << "│ [$] Parallel Composition │\n";
    std::cout << "│ [@] Generate Supervisor  │\n";
    std::cout << "│ [-] Heuristic Schedule   │\n";
    std::cout << "│ [+] Visualise Paths      │\n";
    std::cout << "│ [%] Exit                 │\n";
    std::cout << "\u2514" << "──────────────────────────" << "\u2518\n";
}

NameType getNameTypeInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::cin >> input;
    return input;
}

int main() {
    Applications* app = new Applications();
    DES* des = new DES();
    std::string filename;
    char choice;

    while (true) {
        printMainMenu();
        std::cout << "\nEnter choice [1-11]: ";
        std::cin >> choice;

        switch (choice) {
            case '1': {  // Create Application
                printBox("Create New Application");
                int numTasks;
                delete app;
                app = new Applications();
                std::cout << "Enter name of application: ";
                std::cin >> app->appName;
                std::cout << "Enter number of tasks: ";
                std::cin >> numTasks;
                std::cout << "Enter start state for application: ";
                std::cin >> app->startState;
                std::cout << "Enter relative deadline for application: ";
                std::cin >> app->relativeDeadline;
                std::cout << "Enter period of application: ";
                std::cin >> app->period;

                for (int i = 0; i < numTasks; i++) {
                    NameType taskId = getNameTypeInput("\nEnter task name: ");
                    bool isMarked;
                    int executionTime;
                    std::cout << "Enter execution time: ";
                    std::cin >> executionTime;
                    std::cout << "Is task marked? (1 for Yes, 0 for No): ";
                    std::cin >> isMarked;
                    app->addTask(taskId, executionTime, isMarked);

                    int numTransitions;
                    std::cout << "Enter number of transitions for task " << taskId << ": ";
                    std::cin >> numTransitions;

                    for (int j = 0; j < numTransitions; j++) {
                        NameType target = getNameTypeInput("  Target task: ");
                        NameType label = getNameTypeInput("  Label: ");
                        bool isControlled;
                        std::cout << "  Is the transition controlled? (1 for Yes, 0 for No): ";
                        std::cin >> isControlled;
                        app->tasks[taskId]->addTransition(target, label, isControlled);
                    }
                }
                printBox("Application Created Successfully");
                break;
            }

            case '2': {
                printBox("Generate Image (APP)");
                std::cout << "Enter output filename (PNG format): ";
                std::cin >> filename;
                DAG::visualizeDAG(app, filename);
                printBox("Application Graph Generated Successfully");
                break;
            }

            case '3': {
                printBox("Generate Image (DES)");
                std::cout << "Enter output filename (PNG format): ";
                std::cin >> filename;
                Graph::visualizeDES(des, filename);
                printBox("DES Graph Generated Successfully");
                break;
            }

            case '4': {
                printBox("Save to File (APP)");
                std::cout << "Enter filename to save: ";
                std::cin >> filename;
                DAG::saveDAGToFile(app, filename);
                printBox("Application Saved Successfully");
                break;
            }

            case '5': {
                printBox("Save to File (DES)");
                std::cout << "Enter filename to save: ";
                std::cin >> filename;
                Graph::saveDESToFile(des, filename);
                printBox("DES Saved Successfully");
                break;
            }

            case '6': {
                printBox("Load from File (APP)");
                std::cout << "Enter filename to load: ";
                std::cin >> filename;
                delete app;
                app = DAG::readDAGFromFile(filename);
                printBox("Application Loaded Successfully");
                break;
            }

            case '7': {
                printBox("Load from File (DES)");
                std::cout << "Enter filename to load: ";
                std::cin >> filename;
                delete des;
                des = Graph::readDESFromFile(filename);
                printBox("DES Loaded Successfully");
                break;
            }

            case '8': {
                // Task Execution
                printBox("Generating Task Execution Models");
                try {
                    std::vector<NameType> order;
                    TaskExecutionModels::getTopologicalOrder(app,&order);
                    TaskExecutionModels::generateTaskExecutionModels(app,&order);
                } catch (const std::exception& e) {
                    std::cerr << e.what() << "\n";
                }

                break;
            }

            case '9': {
                std::vector<NameType> order;
                TaskExecutionModels::getTopologicalOrder(app,&order);
                printBox("Generating Timing Specification Models");
                TimingSpecificationModels::generateTimingSpecificationModels(app,&order);
                break;
            }
            case '#': {
                std::vector<NameType> order;
                int numCores;
                std::cout << "Enter No of cores available: ";
                std::cin >> numCores;
                printBox("Generating Resource Constriant  Models");
                Resources::generateResourceConstraintModel(numCores);
                break;
            }

            case '$': {
                printBox("Perform Composition");
                Parallel::performComposition();
                break;
            }

           case '@': {
                printBox("Generate Supervisor");
                std::unordered_set<NameType>* forbiddenStates = new std::unordered_set<NameType>();
                SCT::safeStateSynthesis(des, forbiddenStates);
                delete forbiddenStates;
                break;
            }

            case '+': {
                printBox("Visualizing Paths to Marked States");
                int n;
                std::cout << "Enter number of paths to visualize: ";
                std::cin >> n;
                des->visualizePathsToMarkedStates(n, des);
                break;
            }
            case '-': {
                printBox("Get Heuristic Schedule");
                CPCModel::constructCPCModel();
                break;
            }
            case '%': {
                printBox("Exiting Program");
                delete app;
                delete des;
                return 0;
            }

            default:
                printBox("Error: Invalid Choice");
        }
    }
    return 0;
}
