// Function to visualize the DES as a DAG
#include "graphs.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

void Graph::visualizeDES(DES* des, const std::string& filename) {
    if (!des) {
        std::cerr << "Invalid DES pointer provided.\n";
        return;
    }
    std::string dotFilename = "Dot/" + filename + ".dot";
    std::string imageFilename = "Images/" + filename + ".jpeg";

    std::ofstream file(dotFilename);
    if (!file) {
        std::cerr << "Error opening file for writing Graphviz DOT file.\n";
        return;
    }
    file << "digraph DES {\n";
    file << "    rankdir=LR;\n";

    if (!des->startState.empty()) {
        file << "    \"START\" [shape=point, style=invis];\n";
        file << "    \"START\" -> \"" << des->startState << "\";\n";
    }

    for (const auto& [key, state] : des->des) {
        file << "    \"" << state->id << "\"";

        // Marked states get 'doublecircle'
        if (state->marked) {
            file << " [shape=doublecircle]";
        }

        file << ";\n";

        for (const auto& transition : state->transitions) {
            file << "    \"" << state->id << "\""
                 << " -> \"" << transition.targetState << "\""
                 << " [label=\"" << transition.label << "\"];\n";
        }
    }

    file << "}\n";
    file.close();

    if (system(("dot -Tpng \"" + dotFilename + "\" -o \"" + imageFilename + "\"").c_str()) != 0) {
        std::cerr << "Error generating graph visualization.\n";
    } else {
        std::cout << "Graph visualization saved as " << imageFilename << std::endl;
    }
}


void Graph::saveDESToFile(DES* des, const std::string& filename) {
    if (!des) {
        std::cerr << "Invalid DES pointer provided.\n";
        return;
    }

    std::string modelFilename = "Models/" + filename + ".des";
    std::ofstream file(modelFilename);
    if (!file) {
        std::cerr << "Error opening file for writing DES data.\n";
        return;
    }

    file << des->des.size() << "\n";
    file << des->startState << "\n";
    for (const auto& [key, state] : des->des) {
        file << state->id << " " << state->marked << "\n";
        file << state->transitions.size() << "\n";
        for (const auto& transition : state->transitions) {
            file << transition.targetState << " " << transition.label << " " << transition.isControlled << "\n";
        }
    }
    file.close();
    std::cout << "DES saved successfully to " << filename << std::endl;
}

DES* Graph::readDESFromFile(const std::string& filename) {
    std::string modelFilename = "Models/" + filename + ".des";
    std::ifstream file(modelFilename);

    if (!file) {
        std::cerr << "Error opening file: " << modelFilename << " for reading DES data.\n";
        return nullptr;
    }

    DES* des = new DES();
    int numStates;
    if (!(file >> numStates)) {
        std::cerr << "Error reading number of states from file.\n";
        delete des;
        return nullptr;
    }

    if (!(file >> des->startState)) {
        std::cerr << "Error reading start states from file.\n";
        delete des;
        return nullptr;
    }

    for (int i = 0; i < numStates; i++) {
        NameType stateId;
        bool isMarked;
        if (!(file >> stateId >> isMarked)) {
            std::cerr << "Error reading state data from file.\n";
            delete des;
            return nullptr;
        }
        des->addState(stateId, isMarked);

        int numTransitions;
        if (!(file >> numTransitions)) {
            std::cerr << "Error reading number of transitions for state " << stateId << "\n";
            delete des;
            return nullptr;
        }

        for (int j = 0; j < numTransitions; j++) {
            NameType target, label;
            bool isControlled;
            if (!(file >> target >> label >> isControlled)) {
                std::cerr << "Error reading transition data for state " << stateId << "\n";
                delete des;
                return nullptr;
            }
            des->des[stateId]->addTransition(target, label, isControlled);
        }
    }

    file.close();
    std::cout << "DES loaded successfully from " << modelFilename << "\n";
    return des;
}
