#ifndef TIMING_SPECIFICATION_MODELS_H_
#define TIMING_SPECIFICATION_MODELS_H_

#include "applications.h"
#include <vector>
#include <string>

using NameType = std::string;

class TimingSpecificationModels {
public:
    static void generateTimingSpecificationModels(Applications* app,std::vector<NameType>* topologicalOrder);
};

#endif /* TIMING_SPECIFICATION_MODELS_H_ */
