#ifndef MODELS_H_
#define MODELS_H_

#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_set>


using NameType = std::string; // State names and transition labels are strings

// A TRANSITION IDENTIFIED USING START STATE, NEXT STATE, AND LABEL.
struct Transition {
    NameType targetState;
    NameType label;
    bool isControlled;

    Transition(NameType target, NameType lbl="", bool ctrl="True")
        : targetState(target), label(lbl), isControlled(ctrl) {}
};


// STATE IDENTIFIED BY ID, MARKED STATUS, AND TRANSITIONS.
class State {
public:
    NameType id;
    bool marked;
    std::vector<Transition> transitions;
    State(NameType stateId, bool isMarked = false);
    void addTransition(NameType target, NameType label, bool isControlled);
    void removeTransition(NameType target, NameType label);
};

// DES MODEL REPRESENTATION.
class DES {
public:
    std::unordered_map<NameType, State*> des;
    ~DES();
    NameType startState;
    void addState(NameType id, bool marked = false);
    void removeState(NameType id);
    std::unordered_set<NameType>* getMarkedStates();
    void visualizePathsToMarkedStates(int n, DES* des);
};

#endif /* MODELS_H_ */
