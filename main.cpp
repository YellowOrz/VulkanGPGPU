#include "src/context.hpp"
#include <iostream>
using namespace std;


int main() {
    uint32_t num = 1024;
    string spv_path = "array_double.spv";
    Context computer(spv_path, num);
    computer.Run();
    computer.PrintResult(20);
    return 0;
}

