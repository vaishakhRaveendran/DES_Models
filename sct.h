#ifndef SCT_H
#define SCT_H

#include <unordered_set>
#include <string>
#include "models.h"

class SCT {
public:
    // Functions now take pointers to collections and return pointers to collections
    static std::unordered_set<NameType>* restrictedBackward(
        DES* des,
        const std::unordered_set<NameType>* Qm,
        const std::unordered_set<NameType>* Qx
    );

    static std::unordered_set<NameType>* uncontrolledBackward(
        DES* des,
        const std::unordered_set<NameType>* Qb
    );

    static std::unordered_set<NameType>* restrictedForward(
        DES* des,
        const NameType* startState
    );

    static void safeStateSynthesis(
        DES* des,
        std::unordered_set<NameType>* forbiddenStates
    );
};

#endif // SCT_H
