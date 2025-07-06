#!/bin/bash

# Set the output binary name
OUTPUT="application_system"

# Compile the program
g++ -std=c++17 -o $OUTPUT main.cpp models.cpp graphs.cpp applications.cpp dags.cpp taskExecutionModels.cpp parallel.cpp timingSpecificationModels.cpp sct.cpp resources.cpp cpc.cpp visual.cpp -I.

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful! Running the program..."
    ./application_system
else
    echo "Compilation failed. Please check your code."
fi

