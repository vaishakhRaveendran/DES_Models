#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include "models.h"

class Graph {
public:
    // Function to visualize DES and save as an image
    static void visualizeDES(DES* des, const std::string& filename);

    static void saveDESToFile(DES* des, const std::string& filename);

    static DES* readDESFromFile(const std::string& filename);
};

#endif // GRAPH_H
