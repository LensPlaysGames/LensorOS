#include <format>
#include <stdio.h>

int main() {
    std::print("Hello, {}!\n{:x} {{+}} {:x} = {:x}", "world", 13, 29, 13 + 29);
}