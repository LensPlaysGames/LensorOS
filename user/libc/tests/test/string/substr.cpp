#include <testing.hh>
#include <string>

int main() {
    std::string a = "1234567890";
    std::string_view av = a;

    ensure a.substr(4) == "567890";
    ensure a.substr() == a;
    ensure a.substr(1) == "234567890";
    ensure a.substr(2, 5) == "34567";

    /*ensure av.substr(4) == "567890";
    ensure av.substr() == av;
    ensure av.substr(1) == "234567890";
    ensure av.substr(2, 5) == "34567";*/
}
