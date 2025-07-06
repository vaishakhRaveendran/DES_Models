#include "taskExecutionModels.h"
#include <queue>
#include <filesystem>
#include "models.h"
#include "graphs.h"


std::vector<NameType> TaskExecutionModels::getImmediatePredecessors(Applications* app, NameType stateId) {
    std::vector<NameType> predecessors;

    // Iterate over tasks in the application
    for (const auto& [currentId, task] : app->tasks) {
        // Check if this task has transitions leading to stateId
        for (const auto& transition : task->transitions) {
            if (transition.targetState == stateId) {
                predecessors.push_back(currentId);
                break;  // Stop checking further transitions for this task
            }
        }
    }

    return predecessors;
}



void TaskExecutionModels::getTopologicalOrder(Applications* app, std::vector<NameType>* topologicalOrder) {
    if (!topologicalOrder) {
        throw std::invalid_argument("Null pointer provided for topologicalOrder");
    }

    std::unordered_map<NameType, int> inDegree;
    std::queue<NameType> zeroInDegreeQueue;
    topologicalOrder->clear();  // Ensure it's empty before use

    // Initialize in-degree count for each task
    for (const auto& [taskId, task] : app->tasks) {
        inDegree[taskId] = 0;
    }

    // Compute in-degrees
    for (const auto& [taskId, task] : app->tasks) {
        for (const auto& transition : task->transitions) {
            inDegree[transition.targetState]++;
        }
    }

    // Enqueue tasks with zero in-degree
    for (const auto& [taskId, degree] : inDegree) {
        if (degree == 0) {
            zeroInDegreeQueue.push(taskId);
        }
    }

    // Perform topological sorting
    while (!zeroInDegreeQueue.empty()) {
        NameType currentTask = zeroInDegreeQueue.front();
        zeroInDegreeQueue.pop();
        topologicalOrder->push_back(currentTask);

        for (const auto& transition : app->tasks[currentTask]->transitions) {
            inDegree[transition.targetState]--;
            if (inDegree[transition.targetState] == 0) {
                zeroInDegreeQueue.push(transition.targetState);
            }
        }
    }

    // Check for cycles
    if (topologicalOrder->size() != app->tasks.size()) {
        throw std::runtime_error("Cycle detected in the DAG!");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TaskExecutionModels::generateTaskExecutionModels(Applications* app, std::vector<NameType>* topologicalOrder) {
    if (!app || !topologicalOrder) {
        std::cerr << "Error: Invalid application or topological order pointer.\n";
        return;
    }

    const std::string appName = app->appName;
    std::string appFolder = "output/" + appName;
    std::filesystem::create_directories(appFolder);
     // Identify the last task in the topological order
    NameType lastTask = topologicalOrder->back();

    // Generate model for each task in topological order
    for (const auto& taskId : *topologicalOrder) {
        DES* taskModel = new DES();
        int stateCount = 0;

        // Initial state
        std::string currentState = std::to_string(stateCount);
        taskModel->addState(currentState, true);  // Initial state is marked
        taskModel->des[currentState]->addTransition(currentState, "t", true);  // time can pass before the arrival of application
        taskModel->startState=currentState;
        // Add arrival transition (α_app)
        std::string nextState = std::to_string(++stateCount);
        taskModel->des[currentState]->addTransition(nextState, "α_" + appName, false);

        // Get the immediate predecessors of taskId
        std::vector<NameType> predecessors = getImmediatePredecessors(app,taskId);
        // Create len(predecessors) number of states
        for (size_t i = 0; i < predecessors.size(); ++i) {
            taskModel->addState(nextState, false);
            currentState = nextState;
            taskModel->des[currentState]->addTransition(nextState, "t", true);
            nextState = std::to_string(++stateCount);
            // Add completion transitions for all predecessors to this state
            for (const auto& pred : predecessors) {
                taskModel->des[currentState]->addTransition(nextState, "C_" + pred, false);
            }
        }

        // Add ready queue state
        taskModel->addState(nextState, false);
        currentState = nextState;
        taskModel->des[currentState]->addTransition(nextState, "t", true);

        // Add task start transition (S_task)
        nextState = std::to_string(++stateCount);
        taskModel->des[currentState]->addTransition(nextState, "S_" + taskId, true);

        // Add execution time transitions
        for (int i = 0; i < app->tasks[taskId]->executionTime; ++i) {
            taskModel->addState(nextState, false);
            currentState = nextState;
            nextState = std::to_string(++stateCount);
            taskModel->des[currentState]->addTransition(nextState, "t", true);
        }

        // Add task completion transition (C_task)
        taskModel->addState(nextState, false);
        currentState = nextState;
        if (taskId == lastTask) {
            // If it's the last task, directly transition back to state "0"
            taskModel->des[currentState]->addTransition("0", "C_" + taskId, false);
        } else {
            nextState = std::to_string(++stateCount);
            taskModel->des[currentState]->addTransition(nextState, "C_" + taskId, false);
            taskModel->addState(nextState, false);
            currentState = nextState;

            // Add time transition
            taskModel->des[currentState]->addTransition(nextState, "t", true);

            // Application completion transition (θ_app)
            taskModel->des[currentState]->addTransition("0", "C_" + lastTask, false);
        }
        // Save and visualize the model
        std::string filename = taskId;
        std::string Imagename = taskId;
        Graph::visualizeDES(taskModel, Imagename);
        Graph::saveDESToFile(taskModel, filename);

        delete taskModel;
    }
}

