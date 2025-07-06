#include <unordered_set>
#include <string>
#include <queue>
#include <iostream>
#include <thread>
#include <chrono>
#include "sct.h"
#include "models.h"
#include "graphs.h"

std::unordered_set<NameType>* SCT::restrictedBackward(
    DES* des,
    const std::unordered_set<NameType>* Qm,
    const std::unordered_set<NameType>* Qx
) {
    std::unordered_set<NameType>* Qi = new std::unordered_set<NameType>();
    std::queue<NameType> worklist;

    // Start from marked states but ignore forbidden states
    for (const NameType& state : *Qm) {
        if (Qx->find(state) == Qx->end()) {
            Qi->insert(state);
            worklist.push(state);
        }
    }

    // Perform backward search (BFS)
    while (!worklist.empty()) {
        NameType current = worklist.front();
        worklist.pop();

        for (const auto& pair : des->des) {
            NameType predecessor = pair.first;
            State* state = pair.second;

            for (const auto& transition : state->transitions) {
                if (transition.targetState == current &&
                    Qi->find(predecessor) == Qi->end() &&
                    Qx->find(predecessor) == Qx->end()) {
                    Qi->insert(predecessor);
                    worklist.push(predecessor);
                }
            }
        }
    }

    // Filter out forbidden states from the final result
    std::unordered_set<NameType>* result = new std::unordered_set<NameType>();
    for (const NameType& state : *Qi) {
        if (Qx->find(state) == Qx->end()) {
            result->insert(state);
        }
    }

    delete Qi;
    return result;
}


std::unordered_set<NameType>* SCT::uncontrolledBackward(
    DES* des,
    const std::unordered_set<NameType>* Qb
) {
    std::unordered_set<NameType>* Qx = new std::unordered_set<NameType>(Qb->begin(), Qb->end());
    std::queue<NameType> worklist;

    for (const NameType& state : *Qb) {
        worklist.push(state);
    }

    while (!worklist.empty()) {
        NameType current = worklist.front();
        worklist.pop();

        for (const auto& pair : des->des) {
            NameType predecessor = pair.first;
            State* state = pair.second;

            for (const auto& transition : state->transitions) {
                if (transition.targetState == current && !transition.isControlled) {
                    if (Qx->find(predecessor) == Qx->end()) {
                        Qx->insert(predecessor);
                        worklist.push(predecessor);
                    }
                }
            }
        }
    }

    return Qx;
}

std::unordered_set<NameType>* SCT::restrictedForward(
    DES* des,
    const NameType* startState
) {
    std::unordered_set<NameType>* reachableStates = new std::unordered_set<NameType>();
    std::queue<NameType> worklist;

    worklist.push(*startState);
    reachableStates->insert(*startState);

    while (!worklist.empty()) {
        NameType current = worklist.front();
        worklist.pop();

        if (des->des.find(current) == des->des.end()) continue;

        for (const auto& transition : des->des[current]->transitions) {
            NameType nextState = transition.targetState;

            if (reachableStates->find(nextState) == reachableStates->end()) {
                reachableStates->insert(nextState);
                worklist.push(nextState);
            }
        }
    }

    return reachableStates;
}





void SCT::safeStateSynthesis(
    DES* des,
    std::unordered_set<NameType>* forbiddenStates
) {
    std::cout << "Starting safeStateSynthesis..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::unordered_set<NameType>* Q_m = des->getMarkedStates();
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    NameType startState = des->startState;
    std::cout << "Start state: " << startState << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::unordered_set<NameType>* prevForbiddenStates = new std::unordered_set<NameType>();

    do {
        std::cout << "New iteration of forbidden state synthesis..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        prevForbiddenStates->clear();
        prevForbiddenStates->insert(forbiddenStates->begin(), forbiddenStates->end());
        std::cout << "Copied forbidden states." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::unordered_set<NameType>* Q_prime = restrictedBackward(des, Q_m, forbiddenStates);
        std::cout << "Calculated Q_prime." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::unordered_set<NameType>* not_Q_prime = new std::unordered_set<NameType>();
        std::cout << "Identifying not_Q_prime states..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        for (const auto& pair : des->des) {
        const NameType& state = pair.first;
        if (Q_prime->find(state) == Q_prime->end()) {
            not_Q_prime->insert(state);
         }
       }

        std::unordered_set<NameType>* Q_double_prime = uncontrolledBackward(des, not_Q_prime);
        std::cout << "Calculated Q_double_prime." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        forbiddenStates->insert(Q_double_prime->begin(), Q_double_prime->end());
        std::cout << "Updated forbidden states." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));


        delete Q_prime;
        delete not_Q_prime;
        delete Q_double_prime;
        std::cout << "Cleaned up temporary state sets." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    } while (*prevForbiddenStates != *forbiddenStates);



    for (const NameType& state : *forbiddenStates) {
        des->removeState(state);

    }

    std::unordered_set<NameType>* Q_S = restrictedForward(des, &startState);
    std::vector<NameType> toDelete;
    for (const auto& pair : des->des) {
        const NameType& state = pair.first;
        if (Q_S->find(state) == Q_S->end()) {
            toDelete.push_back(state);

        }
    }

    for (const NameType& state : toDelete) {
        des->removeState(state);

    }

    delete Q_m;
    delete prevForbiddenStates;
    delete Q_S;

    std::cout << "Final DES state saved to file." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Graph::saveDESToFile(des, "synthesiser");
    Graph::visualizeDES(des, "synthesiser");
    std::cout << "Final visualization complete." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
