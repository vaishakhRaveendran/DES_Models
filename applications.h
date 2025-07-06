#ifndef APPLICATIONS_H_
#define APPLICATIONS_H_

#include "models.h"

using NameType = std::string;

// TASK REPRESENTATION (CHILD OF STATE)
class Task : public State {
public:
    int executionTime;
    Task(NameType taskId, int execTime, bool isMarked = false);
};

// APPLICATION MODEL REPRESENTATION (CHILD OF DES)
class Applications : public DES {
public:
    std::unordered_map<NameType, Task*> tasks;
    std::string appName;
    int relativeDeadline;
    int period;
    int criticalLength;
    int arrivalTime;

    Applications() : relativeDeadline(0), period(0), criticalLength(0), arrivalTime(0) {}
    void addTask(NameType taskId, int executionTime, bool marked = false);
    void removeTask(NameType taskId);
};

#endif /* APPLICATIONS_H_ */
