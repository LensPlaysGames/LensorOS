#undef TARGET_LENSOR
#define TARGET_LENSOR 1

/* This file is meant to end up as gcc-VERSION/gcc/config/lensor.h.
 * This file is included in the relavant GCC patch; it is only for
 * developmental use.
 */

/* Default arguments
 * `-- `-lc`
 *         Link to C Standard Library
 */
#undef LIB_SPEC
#define LIB_SPEC "-lc"

/* Default linker arguments
 * `-- `-z max-page-size=4096`
 *        Use 4KiB pages by default, even on 64-bit systems.
 */
#undef LINK_SPEC
#define LINK_SPEC "-z max-page-size=4096"

/* Files that are linked before user code.
 * The prefix is to match one of Binutils'
 * /ld/configure.tgt `NATIVE_LIB_DIRS`.
 * The %s tells GCC to look for these files in the library directory.
 */
#undef STANDARD_STARTFILE_PREFIX
#define STANDARD_STARTFILE_PREFIX "/lib/"
#undef STARTFILE_SPEC
#define STARTFILE_SPEC "crt0.o%s crti.o%s crtbegin.o%s"

/* Files that are linked after user code. */
#undef ENDFILE_SPEC
#define ENDFILE_SPEC "crtend.o%s crtn.o%s"

/* Additional predefined macros. */
#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()                \
  do {                                          \
    builtin_define ("__lensor__");              \
    builtin_define ("__lensoros__");            \
    builtin_define ("__unix__");                \
    builtin_assert ("system=lensor");           \
    builtin_assert ("system=lensoros");         \
    builtin_assert ("system=unix");             \
    builtin_assert ("system=posix");            \
  } while(0);
