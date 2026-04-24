#include "printf.hpp"

int main() {
    // Basic types
    sjtu::printf("%s %d %u %%\n", "str", -1, 1u);
    sjtu::printf("%_\n", 42);
    sjtu::printf("%_\n", 42u);
    sjtu::printf("%_\n", "str");
    return 0;
}
