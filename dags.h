#ifndef DAGS_H
#define DAGS_H

#include "graphs.h"
#include "applications.h"

class DAG : public Graph {
public:
    // Function to visualize DAG and save as an image
    static void visualizeDAG(Applications* app, const std::string& filename);

    // Function to save DAG to a file
    static void saveDAGToFile(Applications* app, const std::string& filename);

    // Function to read DAG from a file
    static Applications* readDAGFromFile(const std::string& filename);
};

#endif // DAG_H
