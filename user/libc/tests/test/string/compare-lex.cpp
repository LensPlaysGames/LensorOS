#include <testing.hh>
#include <string>

int main() {
    std::string a = "foobar";
    std::string b = "foobbr";
    std::string c = "barfoo";
    std::string d = "foob";

    ensure a < b;
    ensure c < a;
    ensure c < b;
    ensure d < a;
    ensure d < b;
    ensure a <= a;
    ensure a >= a;
    ensure a != b;
    ensure a != d;
}
