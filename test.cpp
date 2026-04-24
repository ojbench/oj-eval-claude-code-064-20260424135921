#include <iostream>
#include <vector>
#include <string>
#include "printf.hpp"

int main() {
    // Basic types
    sjtu::printf("Hello, %s!\n", "world");
    sjtu::printf("Integer: %d, Unsigned: %u\n", -42, 42u);
    sjtu::printf("Percent: %%\n");

    // Any parameter %_
    sjtu::printf("Any: %_, %_, %_, %_\n", 123, -123, std::string("str"), "const char*");

    // Vector
    std::vector<int> v = {1, 2, 3};
    sjtu::printf("Vector: %_\n", v);

    std::vector<std::string> vs = {"a", "b", "c"};
    sjtu::printf("Vector of strings: %_\n", vs);

    std::vector<std::vector<int>> vv = {{1, 2}, {3, 4}};
    sjtu::printf("Nested vector: %_\n", vv);

    return 0;
}
