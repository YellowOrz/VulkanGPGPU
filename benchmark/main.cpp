#include "benchmark.h"

using namespace std;

int main() {
  int elem_num = 1<<20;
  Benchmark benchmark(elem_num);
  if (!benchmark.CreateSuccess()) {
    printf("[FATAL] Create Benchmark Failed!\n");
    return -1;
  }
  benchmark.Run();
  return 0;
}