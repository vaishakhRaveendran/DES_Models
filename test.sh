#!/bin/bash

# Set the output binary name

OUTPUT="test_system"

# Compile the program

g++ -std=c++17 -o $OUTPUT automated_experiment.cpp models.cpp graphs.cpp applications.cpp dags.cpp taskExecutionModels.cpp parallel.cpp timingSpecificationModels.cpp sct.cpp resources.cpp cpc.cpp visual.cpp -I.

# Check if compilation was successful

if [ $? -eq 0 ]; then

    echo "Compilation successful! Running the program..."

    ./test_system ./Models

else

    echo "Compilation failed. Please check your code."

fi


