# Contributing to LensorOS
If you see something you would like to work on, do it! Pull requests will be reviewed and merged.

---

### C++ Style Guide:
- 4-space indenting ("Stroustrup" style).
- Curly braces on same line.
```cpp
if (condition) {
    do_thing();
    do_something_else();
}
else if (otherCondition) {
    do_other_thing();
    something_entirely_different();
}
```
- If an `if`/`else` block only includes one line, the curly braces may be excluded, and the line indented.
  - If any code on the same indent level as the `if`/`else` keyword follows this type of block, there must one line of free space following.
```cpp
if (condition)
    // ... do one thing ...
    
// ... continue to do things ...
```
- Given multiple one-line `if`/`else` blocks in a row, the one line of free space following the inner blocks may be left out.
- If an `else` block contains one line, it should be on the same line as the `else` keyword.
```cpp
if (condition)
    // ... do one thing ...
else if (condition)
    // ... do another thing ...
else // ... do other thing ...

// .... continue to do things ...
```
- Always initialize primitive types. This makes it harder to forget initialization within each constructor.
```cpp
u64 number { 0 };
bool isPositive { false };
```
- `snake_case` file and function names.
- `camelCase` local variables and parameters.
- `PascalCase` classes, structs, enums, and member fields.
```cpp
class PIT {
public:
    PIT();
    
    void tick() { Ticks += 1; }
    u64 get() { return Ticks; }
    
private:
    u64 Ticks { 0 };
};
```
- SCREAMING_SNAKE_CASE all preprocessor directives.
- Use `define` header include guard in SCREAMING_SNAKE_CASE:
```cpp
#ifndef LENSOR_OS_NAME_OF_FILE_H
#define LENSOR_OS_NAME_OF_FILE_H
... header contents ...
#endif /* LENSOR_OS_NAME_OF_FILE_H */
```
- When declaring arguments for a function in a header file, only put a name if it is not intrinsically clear.
```cpp
void configure_channel(Channel, Access, Mode, u64 frequency);
```
- For acronyms, use your best judgement, but never allow any capital letters in `snake_case`.
- When declaring a pointer or a reference, put the * or & next to the type name, not the variable identifier.
```cpp
u8* buffer;
```
- Use fixed-width integer types unless absolutely necessary (don't use int/char, use s32/u8).
- Use type aliases for all fixed-width integer types (u8, s8, u64, s16, etc).
