#include <format>

int main(int argc, const char **argv) {
    int i = 0;
    while (argc) {
        std::print("{}", argv[i]);
        if (i + 1 < argc) std::print(" ");
    }
    std::print("\n");
}
