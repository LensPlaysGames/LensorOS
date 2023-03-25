#include <filesystem>
#include <stdio.h>

namespace std {
namespace filesystem {
bool exists (path path) {
    FILE* f = fopen(path.string().data(), "r");
    if (!f) return false;
    fclose(f);
    return true;
}
}
}
