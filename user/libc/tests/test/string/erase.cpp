#include <testing.hh>
#include <string>

int main() {
    std::string s = "1234567890";

    s.erase(2, 1);
    ensure s == "124567890";

    s.erase(2, 3);
    ensure s == "127890";

    s.erase(4);
    ensure s == "1278";

    s.erase();
    ensure s.empty();

    s = "1234567890";

    s.erase(s.begin() + 2, s.begin() + 3);
    ensure s == "124567890";

    s.erase(s.begin() + 2, s.begin() + 5);
    ensure s == "127890";

    s.erase(s.begin() + 4, s.end());
    ensure s == "1278";

    s.erase(s.begin());
    ensure s == "278";

    s.erase(s.begin(), s.end());
    ensure s.empty();
}
