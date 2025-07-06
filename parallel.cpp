#include "models.h"
#include "graphs.h"
#include "parallel.h"
#include <stack>
#include <set>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

struct StatePair {
    NameType state1;
    NameType state2;
    bool operator==(const StatePair &other) const {
        return state1 == other.state1 && state2 == other.state2;
    }
};

struct StatePairHash {
    std::size_t operator()(const StatePair &pair) const {
        return std::hash<NameType>()(pair.state1) ^ std::hash<NameType>()(pair.state2);
    }
};

DES* Parallel::parallelComposition(DES* des1, DES* des2) {
    DES* composedDES = new DES();
    std::stack<StatePair> stateStack;
    std::unordered_map<StatePair, int, StatePairHash> stateMapping;
    std::unordered_map<NameType, bool> eventControlStatus;
    std::set<NameType> events1, events2;
    int numStates = 0;

    // Collect event labels from both DES and determine control status
    for (const auto &entry : des1->des) {
        for (const auto &transition : entry.second->transitions) {
            events1.insert(transition.label);
            eventControlStatus[transition.label] = transition.isControlled;
        }
    }
    for (const auto &entry : des2->des) {
        for (const auto &transition : entry.second->transitions) {
            events2.insert(transition.label);
            if (eventControlStatus.find(transition.label) == eventControlStatus.end()) {
                eventControlStatus[transition.label] = transition.isControlled;
            }
        }
    }

    // Initial state
    StatePair initialState = {des1->startState, des2->startState};
    int initialStateName = numStates++;
    composedDES->addState(std::to_string(initialStateName), des1->des[des1->startState]->marked && des2->des[des2->startState]->marked);
    composedDES->startState=std::to_string(initialStateName);
    stateStack.push(initialState);
    stateMapping[initialState] = initialStateName;

    while (!stateStack.empty()) {
        StatePair current = stateStack.top();
        stateStack.pop();
        int composedStateName = stateMapping[current];

        // Get current states from each DES
        State *state1 = des1->des[current.state1];
        State *state2 = des2->des[current.state2];

        // Process transitions
        std::unordered_map<NameType, NameType> transitions1, transitions2;
        for (const auto &t : state1->transitions) transitions1[t.label] = t.targetState;
        for (const auto &t : state2->transitions) transitions2[t.label] = t.targetState;

        // Process common events
        for (const auto &event : events1) {
            if (events2.find(event) != events2.end()) {
                if (transitions1.count(event) && transitions2.count(event)) {
                    StatePair nextState = {transitions1[event], transitions2[event]};
                    if (!stateMapping.count(nextState)) {
                        int nextName = numStates++;
                        composedDES->addState(std::to_string(nextName), des1->des[nextState.state1]->marked && des2->des[nextState.state2]->marked);
                        stateMapping[nextState] = nextName;
                        stateStack.push(nextState);
                    }
                    composedDES->des[std::to_string(composedStateName)]->addTransition(std::to_string(stateMapping[nextState]), event, eventControlStatus[event]);
                }
            }
        }

        // Process exclusive events for DES1
        for (const auto &event : events1) {
            if (!events2.count(event) && transitions1.count(event)) {
                StatePair nextState = {transitions1[event], current.state2};
                if (!stateMapping.count(nextState)) {
                    int nextName = numStates++;
                    composedDES->addState(std::to_string(nextName), des1->des[nextState.state1]->marked && des2->des[nextState.state2]->marked);
                    stateMapping[nextState] = nextName;
                    stateStack.push(nextState);
                }
                composedDES->des[std::to_string(composedStateName)]->addTransition(std::to_string(stateMapping[nextState]), event, eventControlStatus[event]);
            }
        }

        // Process exclusive events for DES2
        for (const auto &event : events2) {
            if (!events1.count(event) && transitions2.count(event)) {
                StatePair nextState = {current.state1, transitions2[event]};
                if (!stateMapping.count(nextState)) {
                    int nextName = numStates++;
                    composedDES->addState(std::to_string(nextName), des1->des[nextState.state1]->marked && des2->des[nextState.state2]->marked);
                    stateMapping[nextState] = nextName;
                    stateStack.push(nextState);
                }
                composedDES->des[std::to_string(composedStateName)]->addTransition(std::to_string(stateMapping[nextState]), event, eventControlStatus[event]);
            }
        }
    }

    return composedDES;
}


void Parallel::performComposition() {
    std::string folderName = "Models";

    std::vector<std::string> desFiles;
    for (const auto &entry : fs::directory_iterator(folderName)) {
        if (entry.path().extension() == ".des") {
            desFiles.push_back(entry.path().stem().string());
        }
    }

    if (desFiles.size() < 2) {
        std::cout << "Not enough DES files for composition." << std::endl;
        return;
    }

    DES *composedDES = Graph::readDESFromFile(desFiles[0]);
    if (!composedDES) {
        std::cout << "Failed to read initial DES file." << std::endl;
        return;
    }

    for (size_t i = 1; i < desFiles.size(); ++i) {
        std::cout << "Composing: " << desFiles[i] << " with current composition." << std::endl;

        DES *nextDES = Graph::readDESFromFile(desFiles[i]);
        if (!nextDES) {
            std::cout << "Failed to read DES file: " << desFiles[i] << std::endl;
            delete composedDES;
            return;
        }

        DES *newComposedDES = Parallel::parallelComposition(composedDES, nextDES);
        delete composedDES;
        delete nextDES;
        if (!newComposedDES) {
            std::cout << "Parallel composition failed at step " << i << std::endl;
            return;
        }

        composedDES = newComposedDES;
    }

    std::string outputFileName = "final_composed";
    Graph::saveDESToFile(composedDES, outputFileName);
    Graph::visualizeDES(composedDES, outputFileName);

    delete composedDES;
    std::cout << "Final parallel composition completed and saved." << std::endl;
}
