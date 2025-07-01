#include "benchmark.h"

using namespace std;

int main() {
  int elem_num = 1<<24;
  Benchmark benchmark(elem_num);
  if (!benchmark.CreateSuccess()) {
    printf("[FATAL] Create Benchmark Failed!\n");
    return -1;
  }
  benchmark.run();
  return 0;
}