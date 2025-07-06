#include "applications.h"

// Task Constructor
Task::Task(NameType taskId, int execTime, bool isMarked)
    : State(taskId, isMarked), executionTime(execTime) {}

// Add a new task to the Applications model
void Applications::addTask(NameType taskId, int executionTime, bool marked) {
    if (tasks.find(taskId) == tasks.end()) {
        tasks[taskId] = new Task(taskId, executionTime, marked);
    }
}

// Remove a task from the Applications model
void Applications::removeTask(NameType taskId) {
    auto it = tasks.find(taskId);
    if (it != tasks.end()) {
        delete it->second;
        tasks.erase(it);
    }
}

