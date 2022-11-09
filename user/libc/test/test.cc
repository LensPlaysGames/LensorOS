#include <format>
#include <stdio.h>

int main() {
    //std::print("Hello, {}!\n{:x} {{+}} {:x} = {}\n", "world", 13, 29, .45678);
    std::print("[kstage1]: \033[32mProgrammable Interval Timer Initialized\033[0m\r\n"
               "  Channel 0, H/L Bit Access\r\n"
               "  Rate Generator, BCD Disabled\r\n"
               "  Periodic interrupts at \033[33m{}hz\033[0m.\r\n"
               "\r\n", static_cast<double>(20));
}