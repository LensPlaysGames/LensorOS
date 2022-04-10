# Contributing to LensorOS
If you see something you would like to work on, do it! Pull requests will be reviewed and merged.

---

## Table of Contents
- [C++ Style Guide](#cpp)
  - [Indentation](#indentation)
  - [Spacing](#spacing)
  - [Line Breaks](#line-breaks)
  - [Braces](#braces)
  - [Null, false, and zero](#nil)
  - [Names](#names)
  - [Types](#types)
  - [Other](#other)

---

### C++ Style Guide <a name="cpp"></a>
These are some rough guidelines to help any submitted code be consistent with the rest of the codebase. It is completely okay to make a PR with code that doesn't strictly follow these guidelines, but it does increase the time that it will take to get the PR accepted and merged.

---

#### Indentation <a name="indentation"></a>

- Use spaces, not tabs.
- The indent size is **four spaces**.
- The contents of top level namespaces should be indented, but not nested namespaces.

Valid:
```cpp
namespace ACPI {
    struct RSDP2 {
        RSDP2() {};
        ...
    };
    
    namespace Port {
    class Controller {
        Controller() {};
        ...
    };
    }
} // namespace ACPI
```

Invalid:
```cpp
namespace ACPI {
struct RSDP2 {
    RSDP2() {};
    ...
};

namespace Port {
class Controller {
    Controller() {};
    ...
};
}
} // namespace ACPI
```

---

- Boolean expressions at the same nesting level that span multiple lines should have their operators on the left side of the line instead of the right.

Valid:
```cpp
while (newTime.second     != Time.second
       || newTime.minute  != Time.minute
       || newTime.hour    != Time.hour
       || newTime.weekday != Time.weekday
       || newTime.date    != Time.date
       || newTime.month   != Time.month
       || newTime.year    != Time.year
       || newTime.century != Time.century);
```

Valid:
```cpp
return c == '+'
    || c == '-'
    || c == '*'
    || c == '/';
```

Invalid:
```cpp
return c == '+' ||
    c == '-' ||
    c == '*' ||
    c == '/';
```

---

- `case` labels should line up with `switch` statement. The case statement is indented.

Valid:
```cpp
switch(condition) {
case fooCondition:
case barCondition:
    break;
default:
    return 0;
}
```

Invalid:
```cpp
switch(condition) {
    case fooCondition:
    case barCondition:
        break;
    default:
        return 0;
}
```

---

#### Spacing  <a name="spacing"></a>
- Do not place spaces around unary operators.

Valid:
```cpp
i++
```

Invalid:
```cpp
i ++;
```

---

- Do place spaces around binary and ternary operators.

Valid:
```cpp
y = x * 2;
z = foobar(x, y);
c = (a | b) & d;
baz(z, x, y, c);
return c ? 1 : 0;
```

Invalid:
```cpp
y=x *2;
z = foobar(x,y);
c = (a |b)& d;
baz( z,x,y,c );
return c? 1:0;
```

---

- Place spaces between control statements and their parentheses.

Valid:
```cpp
if (condition)
    do_thing();
```

Invalid:
```cpp
if(condition)
    do_thing();
```

---

- Do not place spaces between a function and its parentheses, or between a parenthesis and its content.

Valid:
```cpp
func(a, b);
```

Invalid:
```cpp
func (a,b);
func( a , b );
func ( a, b );
```

---

- Do not place spaces between square brackets and parentheses of a lambda function but do place a space before braces.

Valid:
```cpp
[&capturedReference](int x) {
    capturedReference.do_it(x);
    return capturedReference;
};
[this] { return Member; }
```

Invalid:
```cpp
[&capturedReference] (int x){
    capturedReference.doIt(x);
    return capturedReference;
};
[this]{ return Member; }
```

---

- When initializing an object, place a space before the leading brace as well as between the braces and their content.

Valid:
```cpp
Foo foo { bar };
```

Invalid:
```cpp
Foo foo{ bar };
Foo foo {bar};
```

---

- Access keywords (like `public`) should be aligned with the `class`/`struct` keyword.

Valid:
```cpp
class Foo {
public:
    Foo() {}
    
private:
    Bar bar;
};
```

#### Line Breaks <a name="line-breaks"></a>
- Each statement should get it's own line.

Valid:
```cpp
int x1 = 5;
int y1 = 7;
int x2 = 420;
int y2 = 69;
unsigned i { 0 };
```

Invalid:
```cpp
int x1 = 5; int y1 = 7;
int x = 5, y = 7;
unsigned i { 0 };
```

---

- `if`/`else if`/`else` statements on the same nesting level should be aligned with the `if` statement.


Valid:
```cpp
if (condition) {
    ...
    do_that_thing();
}
else if (otherCondition) {
    ...
    do_something();
}
else {
    ...
    do_other_thing();
}
```

Invalid:
```cpp
if (condition) {
    ...
    do_that_thing();
} else if (otherCondition) {
    ...
    do_something();
} else {
    ...
    do_other_thing();
}
```

---

- An `else if` statement should be written as an `if` statement when the prior `if` concludes with a return statement.

Valid:
```cpp
if (condition) {
    ...
    return aValue;
}
if (condition) {
    ...
}
```

Invalid:
```cpp
if (condition) {
    ...
    return aValue;
}
else if (condition) {
    ...
}
```

---

#### Braces <a name="braces"></a>
- Blocks (denoted by curly braces `{}`) should open on the same line but end on a new line.

Valid:
```cpp
int main() {
    return 0;
}
```

Invalid:
```cpp
int main() 
{
    return 0; 
}
```

Invalid:
```cpp
int main() 
{ return 0; }
```

---

- Single-line control clauses should not use braces unless comments are included or a single statement spans multiple lines.
- Single-line control clauses may optionally be included on the same line as the control flow keyword if the resulting line is less than eighty characters.

Valid:
```cpp
if (condition)
    do_thing();
    
if (condition) {
    // This is a comment.
    do_something();
}

if (condition) {
    long_function_name(parameterWithLongName, anotherParameter()
                       , theThirdParameterPassed, final);
}

if (condition) do_that_thing();
else do_that_other_thing();
```

Invalid:
```cpp
if (condition) {
    do_thing();
}

if (condition)
    // This is a comment.
    do_something();

if (condition)
    long_function_name(parameterWithLongName, anotherParameter()
                       , theThirdParameterPassed, final);
```

---

#### Null, false, zero <a name="nil"></a>
- A null pointer value (pointer to address zero) should be written as `nullptr`. In C, use `NULL`.
- Boolean values should be written as `true` and `false`.

- Favor using explicit comparisons when testing for null/non-null, zero/non-zero, etc. Explicit comparisons are not required for true/false tests.

Valid:
```cpp
if (condition)
    do_thing();

if (ptr == nullptr)
    return nullptr;
    
if (number != 0)
    number *= 2;
```

Invalid:
```cpp
if (condition == true)
    do_thing();

if (!ptr)
    return nullptr;
    
if (number)
    number *= 2;
```

---

#### Names <a name="names"></a>
- `PascalCase` classes, structs, enums, and member fields.
- `camelCase` local variables and parameters.
- `snake_case` file and function names.
- Suffix implementation files with `.cpp` and declaration files with `.h`.
- If a name begins with an acronym, capitalize it unless using `snake_case`.

Valid:
```cpp
class PIT {
public:
    PIT();

    void tick() { Ticks += 1; }
    u64 ticks() { return Ticks; }

private:
    u64 Ticks { 0 };
};
```

Invalid:
```cpp
class pit {
public:
    pit();
    
    void Tick() { m_ticks += 1; }
    u64 getTicks() { return m_ticks; }
    
private:
    u64 m_ticks;
}
```

---

- `SCREAMING_SNAKE_CASE` all preprocessor directives.
- Use `define` header include guard with a blank line between it and the contents of the header file.
```cpp
#ifndef LENSOR_OS_NAME_OF_FILE_H
#define LENSOR_OS_NAME_OF_FILE_H

... header contents ...

#endif /* LENSOR_OS_NAME_OF_FILE_H */
```

---

- Use full words, except in the case where the abbreviation or acronym is more canonical.

Valid:
```cpp
u64 characterSize { 0 };
u64 length { 0 };
u64 tabCount { 0 };
```

Invalid:
```cpp
u64 charSz { 0 };
u64 len { 0 };
u64 tabulationCount { 0 }; // Who says tabulation?
```

---

- Data members in C++ classes should be private. To expose a data member publicly, use getter/setter function(s).
  - Use the `snake_case` version of the member identifier as the getter function name.
  - Call setters `set_` followed by the name of the getter.
  - If a getter must return it's value through an argument, precede it with `get_`.

Valid:
```cpp
class Timer {
public:
    ...
    
    u64 ticks() { return Ticks; };
    void get_ticks(u64& ticks) { ticks = Ticks; }

    u64 set_ticks(u64 t) { Ticks = t; }
    

private:
    u64 Ticks;
};
```

Invalid:
```cpp
class Timer {
public:
    void ticks(u64& ticks) { ticks = Ticks; }
    u64 get_ticks() { return Ticks; };

    u64 Ticks;
};
```

---

- If a getter function for a member variable automatically creates an object if it doesn't exist, suffix the getter that **not create** a new object with `_if_exists`.

Valid:
```cpp
FilesystemDriver* filesystem_driver();
```

Valid:
```cpp
FilesystemDriver* filesystem_driver();
FilesystemDriver* filesystem_driver_if_exists();
```

Invalid:
```cpp
FilesystemDriver* filesystem_driver_if_exists();
```

Invalid:
```cpp
FilesystemDriver* filesystem_driver();
FilesystemDriver* ensure_filesystem_driver();
```

---

- Leave meaningless variable names out of function declarations.
  - Rule of thumb is if the type name includes the variable name, it's not necessary. Most primitive types will require a name.
  
Valid:
```cpp
void configure_channel(Channel, Access, Mode, u64 frequency);
```

Invalid:
```cpp
void configure_channel(Channel channel, Access access, Mode mode, u64);
```

---

- Prefer enums to bools on function parameters if callers are likely to be passing constants, since named constants are easier to read at the call site. This does not apply if the meaning of the bool is encoded in the name of the function (i.e. `set_valid(true)` is fine).

Valid:
```cpp
void do_something(Thing::DoIt);
```

Invalid:
```cpp
void do_something(bool should);
```

---

- Prefer `constexpr` to `#define`, `const`.

- Macros that expand to function calls or other non-constant computation: these should be named like functions, and should have parentheses at the end, even if they take no arguments (with the exception of some special macros like ASSERT). Note that usually it is preferable to use an inline function in such cases instead of a macro.

---

#### Types <a name="types"></a>

- Use the fixed-width integer aliases defined in `integers.h`.

Valid:
```cpp
#include <integers.h>

u64 largeNumber;
u16 smallNumber;
```

Invalid:
```cpp
long long largeNumber;
short smallNumber;
```

---

- Put pointer and reference type annotations next to the type name.

Valid:
```cpp
void* pointer;
int& reference;
```

Invalid:
```cpp
void *pointer;
int &reference;
```

---

#### Other <a name="other"></a>

- If a declaration or implementation references code from another file, it should `#include` that file.
- Use angle-bracket `<>` notation to include files.
- Within an implementation file, always `#include` the corresponding declaration file on the first line, followed by a blank line. For example, `Timer.cpp` should include `Timer.h` on the first line, followed by the second line being blank. Any further `#include`s should be just after the blank line. Don't bother to organize them in any way other than alphabetical.

Valid:
```cpp
// thing.cpp
#include <thing.h>

#include <other_thing.h>
#include <that_thing.h>
```

Invalid:
```cpp
// thing.cpp
#include "thing.h" // Using quotes.

#include <other_thing.h>
#include <that_thing.h>
```

Invalid:
```cpp
// thing.cpp
#include <other_thing.h> // Wrong order.

#include <that_thing.h>
#include <thing.h>
```

Invalid:
```cpp
// thing.cpp
#include <thing.h> // No blank line.
#include <that_thing.h>
#include <other_thing.h>
```

---

- Write comments starting with `TODO` or `FIXME` without attribution to denote items that need to be addressed in the future.

---

- Base-level virtual methods must be declared with the `virtual` keyword. All derived classed must either specify the `override` or `final` keyword. Never annotate a single function with more than one of `virtual`, `override`, or `final` keywords.
