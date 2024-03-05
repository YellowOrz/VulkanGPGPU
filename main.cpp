#include "src/context.hpp"
#include <iostream>
using namespace std;


int main() {
    uint32_t buffer_num = 1;
    string spv_path = "sum_two_vec.spv";
    Context computer(spv_path, buffer_num);
    return 0;
}

