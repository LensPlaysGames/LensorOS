#include <format>
#include <stdio.h>

int main(int argc, char** argv) {
    std::print("argc: {}\n", argc);
    for (int i = 0; i < argc; i++) {
        std::print("argv[{}]: {}\n", i, (const char*) argv[i]);
    }
}