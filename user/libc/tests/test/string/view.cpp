#include <testing.hh>
#include <string>

int main() {
    const char in[] = "2-439t2-49tj234-9tj2-49tj2w4-tj249tj24-tj24t-jk24t-92wj4t9j";
    std::string a = "1234";
    std::string b = in;

    std::string_view av = a;
    std::string_view bv = b;

    ensure av.data() == a.data();
    ensure bv.data() == b.data();

    ensure av.size() == a.size();
    ensure bv.size() == b.size();
/*
    ensure av == a;
    ensure bv == b;

    ensure av == "1234";
    ensure bv == in;*/
}
