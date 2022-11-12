#include <testing.hh>
#include <string>

int main() {
    std::string a = "1234";
    std::string b = "abcd";

    /// Short string concatenation.
    a += b;
    ensure a.size() == 8;
    ensure eq(a.data(), "1234abcd", 8);

    const char in1[] = "2-439t2-49tj234-9tj2-49tj2w4-tj249tj24-tj24t-jk24t-92wj4t9j";
    std::string c = in1;

    /// Long string concatenation.
    a += c;
    ensure a.size() == 8 + sizeof in1 - 1;
    ensure eq(a.data(), "1234abcd2-439t2-49tj234-9tj2-49tj2w4-tj249tj24-tj24t-jk24t-92wj4t9j", 8 + sizeof in1 - 1);

    /// Operator +
    std::string d = a + b;
    std::string e = c + c;

    ensure d.size() == a.size() + b.size();
    ensure eq(d.data(), "1234abcd2-439t2-49tj234-9tj2-49tj2w4-tj249tj24-tj24t-jk24t-92wj4t9jabcd", d.size());
    ensure e.size() == c.size() + c.size();
    ensure eq(e.data(), "2-439t2-49tj234-9tj2-49tj2w4-tj249tj24-tj24t-jk24t-92wj4t9j2-439t2-49tj234-9tj2-49tj2w4-tj249tj24-tj24t-jk24t-92wj4t9j", e.size());
}
