#include <string>
#include <testing.hh>

int main() {
    std::string a;
    std::string b = "123456";
    std::string c = "23456789012345678923456789234567823456782345678345678";

    ensure a.size() == 0;
    ensure b.size() == 6;
    ensure c.size() == 53;
    ensure eq(b.data(), "123456", 6);
    ensure eq(c.data(), "23456789012345678923456789234567823456782345678345678", 53);
}