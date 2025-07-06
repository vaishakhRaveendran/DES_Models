#include "models.h"
#include "graphs.h"
#include <filesystem>
#include <set>
#include <iostream>
#include <queue>
#include <thread>
#include <chrono>
#include <bits/stdc++.h>
using namespace std;

// STATE CONSTRUCTOR
State::State(NameType stateId, bool isMarked) : id(stateId), marked(isMarked) {}

// ADD A TRANSITION TO A STATE
void State::addTransition(NameType target, NameType label, bool isControlled) {
    transitions.push_back({target, label, isControlled});
}

// REMOVE A SPECIFIC TRANSITION
void State::removeTransition(NameType target, NameType label) {
    transitions.erase(
        std::remove_if(transitions.begin(), transitions.end(), [&](const Transition& t) {
            return t.targetState == target && t.label == label;
        }),
        transitions.end()
    );
}

// DESTRUCTOR TO FREE MEMORY
DES::~DES() {
    for (auto& pair : des) {
        delete pair.second;
    }
    des.clear();
}

// ADD A STATE TO THE DES
void DES::addState(NameType id, bool marked) {
    if (des.find(id) == des.end()) {
        des[id] = new State(id, marked);
    }
}

// REMOVE A STATE AND ITS ASSOCIATED TRANSITIONS
void DES::removeState(NameType id) {
    // Find and delete the state first
    auto it = des.find(id);
    if (it != des.end()) {
        delete it->second;
        des.erase(it);
    }

    // Now iterate through all states and remove transitions pointing to `id`
    for (auto& pair : des) {
        State* state = pair.second;
        state->transitions.erase(
            std::remove_if(state->transitions.begin(), state->transitions.end(),
                [&](const Transition& t) { return t.targetState == id; }),
            state->transitions.end()
        );
    }
}



std::unordered_set<NameType>* DES::getMarkedStates() {
    auto* markedStates = new std::unordered_set<NameType>();

    for (const auto& pair : des) {
        State* state = pair.second;
        if (state->marked) {
            markedStates->insert(state->id);
        }
    }

    return markedStates;
}





// Function to visualize paths reaching marked states using DFS
void DES::visualizePathsToMarkedStates(int n, DES* des) {
    std::unordered_set<NameType> markedStates = *des->getMarkedStates();
    std::vector<std::vector<std::pair<NameType, NameType>>> paths; // Stores state and transition label
    std::vector<std::pair<NameType, NameType>> currentPath;
    std::set<NameType> visited;

    std::function<void(NameType)> dfs = [&](NameType state) {
        if (paths.size() >= static_cast<size_t>(n)) return;

        visited.insert(state);

        if (markedStates.find(state) != markedStates.end()) {
            paths.push_back(currentPath);
        } else {
            for (const auto& transition : des->des[state]->transitions) {
                if (!visited.count(transition.targetState)) {
                    currentPath.push_back({state, transition.label});
                    dfs(transition.targetState);
                    currentPath.pop_back();
                }
            }
        }

        visited.erase(state);
    };

    dfs(des->startState);

    // Create and visualize DES for each path
    for (size_t i = 0; i < paths.size(); ++i) {
        DES* pathDES = new DES();
        if (!paths[i].empty()) {
            pathDES->startState = paths[i].front().first;
        }

        for (size_t j = 0; j < paths[i].size(); ++j) {
            pathDES->addState(paths[i][j].first);
            if (j + 1 < paths[i].size()) {
                pathDES->des[paths[i][j].first]->addTransition(paths[i][j + 1].first, paths[i][j].second, true);
            }
        }

        if (!paths[i].empty()) {
            pathDES->addState(paths[i].back().first, true);
        }

        Graph::visualizeDES(pathDES, "Path_" + std::to_string(i + 1));

        delete pathDES;
    }
}
