#include <testing.hh>
#include <string>

int main() {
    std::string a = "1234";
    std::string b = "1234";

    ensure a == "1234";
    ensure a == b;

    const char in[] = "2-439t2-49tj234-9tj2-49tj2w4-tj249tj24-tj24t-jk24t-92wj4t9j";
    std::string c = in;
    std::string d = in;

    ensure c == in;
    ensure c == d;
}
