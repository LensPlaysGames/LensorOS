#include <format>
#include <filesystem>

int main(int argc, const char **argv) {
    for (int i = 1; i < argc; ++i) {
        std::print("{}", argv[i]);
        if (i + 1 < argc) std::print(" ");
    }
    std::print("\n");
}
