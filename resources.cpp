#include "resources.h"
#include "models.h"
#include "dags.h"
#include "applications.h"
#include <filesystem>
#include <set>
#include <iostream>

namespace fs = std::filesystem;

void Resources::generateResourceConstraintModel(int numCores) {
    std::set<std::string> taskIdentifiers;

    // Iterate over all application models in the "models" folder
    for (const auto& entry : fs::directory_iterator("Models")) {
        if (entry.path().extension() == ".dag") { // Assuming application files have .dag extension
            std::string appName = entry.path().stem().string(); // Extract only the filename without extension
            Applications* appModel = DAG::readDAGFromFile(appName);
            if (!appModel) {
                std::cerr << "Error: Failed to load " << appName << "\n";
                continue;
            }

            // Extract task identifiers from each application
            for (const auto& [taskId, task] : appModel->tasks) {
                taskIdentifiers.insert(taskId);
            }

            delete appModel;
        }
    }

    // Create a new Resource Model
    DES* resourceModel = new DES();
    resourceModel->startState = "0";

    // Add states
    for (int i = 0; i <= numCores; ++i) {
        resourceModel->addState(std::to_string(i), true);
    }

    // Add transitions for each task
    for (const auto& taskId : taskIdentifiers) {
        for (int i = 0; i < numCores; ++i) {
            resourceModel->des[std::to_string(i)]->addTransition(std::to_string(i + 1), "S_" + taskId, true);
            resourceModel->des[std::to_string(i + 1)]->addTransition(std::to_string(i), "C_" + taskId, false);
        }
    }

    // Save and visualize
    Graph::saveDESToFile(resourceModel, std::to_string(numCores) + "_cores_model");
    Graph::visualizeDES(resourceModel, std::to_string(numCores) + "_cores_model");

    delete resourceModel;
}
