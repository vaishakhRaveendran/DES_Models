#ifndef PARALLEL_H
#define PARALLEL_H

#include <string>
#include "models.h"

class Parallel {
public:
  static DES* parallelComposition(DES* des1, DES* des2);
  static void performComposition();
};

#endif // PARALLEL_H

