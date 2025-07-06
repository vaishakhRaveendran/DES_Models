#ifndef TASK_EXECUTION_MODELS_H_
#define TASK_EXECUTION_MODELS_H_

#include "applications.h"
#include <vector>
#include <string>

using NameType = std::string;

class TaskExecutionModels :public Applications{
public:
    static void getTopologicalOrder(Applications* app,std::vector<NameType>* topologicalOrder);
    static void generateTaskExecutionModels(Applications* app, std::vector<NameType>* topologicalOrder);
    static std::vector<NameType> getImmediatePredecessors(Applications* app,NameType stateId);
};

#endif /* TASK_EXECUTION_MODELS_H_ */
