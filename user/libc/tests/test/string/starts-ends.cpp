#include <testing.hh>
#include <string>

int main() {
    std::string a = "123456";

    ensure a.starts_with("123456");
    ensure a.starts_with("123");
    ensure not a.starts_with("1234567");
    ensure not a.starts_with("123457");
    ensure not a.starts_with("234");

    ensure a.ends_with("123456");
    ensure a.ends_with("456");
    ensure not a.ends_with("1234567");
    ensure not a.ends_with("123457");
    ensure not a.ends_with("345");
}
