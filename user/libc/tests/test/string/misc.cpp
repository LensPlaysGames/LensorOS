#include <testing.hh>
#include <string>

int main() {
    const char in[] = "2-439t2-49tj234-9tj2-49tj2w4-tj249tj24-tj24t-jk24t-92wj4t9j";
    std::string a = "1234";
    std::string b = in;
    std::string c;

    ensure a.size() == 4;
    ensure b.size() == sizeof in - 1;
    ensure c.size() == 0;
    ensure a.capacity() >= 4;
    ensure b.capacity() >= sizeof in - 1;
    ensure a[a.size()] == 0;
    ensure b[b.size()] == 0;
    ensure c[0] == 0;
    ensure a.data() == a.c_str();
    ensure b.data() == b.c_str();
    ensure (a.begin() + ssize_t(a.size())) == a.end();
    ensure (b.begin() + ssize_t(b.size())) == b.end();
    ensure c.begin() == c.end();
    ensure not a.empty();
    ensure not b.empty();
    ensure c.empty();
    ensure a.front() == '1';
    ensure b.front() == '2';
    ensure a.back() == '4';
    ensure b.back() == 'j';
}
