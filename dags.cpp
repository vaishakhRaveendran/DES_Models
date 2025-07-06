#include "dags.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

void DAG::visualizeDAG(Applications* app, const std::string& filename) {
    if (!app) {
        std::cerr << "Invalid Applications pointer provided.\n";
        return;
    }

    std::string dotFilename = "Dot/" + filename + ".dot";
    std::string imageFilename = "Images/" + filename + ".jpeg";

    std::ofstream file(dotFilename);
    if (!file) {
        std::cerr << "Error opening file for writing Graphviz DOT file.\n";
        return;
    }

    file << "digraph DAG {\n";
    file << "    rankdir=LR;\n"; // Left-to-right layout for better visualization

    // Add a dummy node for the start state with an incident arrow
    if (!app->startState.empty()) {
        file << "    \"START\" [shape=point, style=invis];\n"; // Invisible start node
        file << "    \"START\" -> \"" << app->startState << "\";\n"; // Incident arrow
    }

    for (const auto& [key, task] : app->tasks) {
        file << "    \"" << task->id << "\" [label=\"" << task->id
             << "\\nExec Time: " << task->executionTime << "\"";

        // Marked states get 'doublecircle'
        if (task->marked) {
            file << ", shape=doublecircle";
        }

        file << "];\n";

        for (const auto& transition : task->transitions) {
            file << "    \"" << task->id << "\""
                 << " -> \"" << transition.targetState << "\""
                 << " [label=\"" << transition.label << "\"];\n";
        }
    }

    file << "}\n";
    file.close();

    if (system(("dot -Tpng \"" + dotFilename + "\" -o \"" + imageFilename + "\"").c_str()) != 0) {
        std::cerr << "Error generating DAG visualization.\n";
    } else {
        std::cout << "DAG visualization saved as " << imageFilename << std::endl;
    }
}


void DAG::saveDAGToFile(Applications* app, const std::string& filename) {
    if (!app) {
        std::cerr << "Invalid Applications pointer provided.\n";
        return;
    }

    std::string modelFilename = "Models/" + filename + ".dag";
    std::ofstream file(modelFilename);
    if (!file) {
        std::cerr << "Error opening file for writing DAG data.\n";
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
    std::cout << "DAG saved successfully to " << filename << std::endl;
}

Applications* DAG::readDAGFromFile(const std::string& filename) {
    std::string modelFilename = "Models/" + filename + ".dag";
    std::ifstream file(modelFilename);

    if (!file) {
        std::cerr << "Error opening file: " << modelFilename << " for reading DAG data.\n";
        return nullptr;
    }

    Applications* app = new Applications();
    int numTasks;
    if (!(file >> app->appName)) {
        std::cerr << "Error reading name of application from file.\n";
        delete app;
        return nullptr;
    }

    if (!(file >> numTasks)) {
        std::cerr << "Error reading number of tasks from file.\n";
        delete app;
        return nullptr;
    }

    if (!(file >> app->startState)) {
        std::cerr << "Error start state of applications from file.\n";
        delete app;
        return nullptr;
    }

    if (!(file>>app->relativeDeadline)) {
        std::cerr << "Error relativeDeadline of application from file.\n";
        delete app;
        return nullptr;
    }

    if (!(file>>app->period)) {
        std::cerr << "Error reading period of application from file.\n";
        delete app;
        return nullptr;
    }



    for (int i = 0; i < numTasks; i++) {
        NameType taskId;
        bool isMarked;
        int executionTime;
        if (!(file >> taskId >> isMarked >> executionTime)) {
            std::cerr << "Error reading task data from file.\n";
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
    std::cout << "DAG loaded successfully from " << modelFilename << "\n";
    return app;
}
