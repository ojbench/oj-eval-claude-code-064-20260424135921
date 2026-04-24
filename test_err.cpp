#include <iostream>
#include <vector>
#include "printf.hpp"

int main() {
    try {
        sjtu::printf("Too many %s\n", "args", "extra");
    } catch (...) {
        std::cout << "Caught expected error for too many args" << std::endl;
    }
    return 0;
}
