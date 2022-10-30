/* Copyright 2022, Contributors To LensorOS.
* All rights reserved.
*
* This file is part of LensorOS.
*
* LensorOS is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* LensorOS is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with LensorOS. If not, see <https://www.gnu.org/licenses/>.
*/


#include "bits/decls.h"
#include "stdlib.h"
#include "unistd.h"

using init_cb = void (*)();

__BEGIN_DECLS__

/// Global constructors and destructors. These symbols are provided by the linker.
extern init_cb __preinit_array_start[];
extern init_cb __preinit_array_end[];
extern init_cb __init_array_start[];
extern init_cb __init_array_end[];
extern init_cb __fini_array_start[];
extern init_cb __fini_array_end[];

/// We need to pass envp to main too.
int main(int argc, char** argv, char** envp);

/// Call global constructors.
void __libc_init() noexcept {
    for (init_cb* cb = __preinit_array_start; cb != __preinit_array_end; ++cb) { (*cb)(); }
    for (init_cb* cb = __init_array_start; cb != __init_array_end; ++cb) { (*cb)(); }
}

/// Call global destructors.
void __libc_fini() noexcept {
    for (init_cb* cb = __fini_array_start; cb != __fini_array_end; ++cb) { (*cb)(); }
}

/// Run the program.
int __libc_run_main(int argc, char** argv, char** envp) {
    __libc_init();
    int ret = __extension__ main(argc, argv, envp);
    __libc_fini();
    return ret;
}

/// Call global destructors and exit the program.
__attribute__((noreturn)) void __libc_exit(int status) {
    __libc_fini();
    _Exit(status);
}
__END_DECLS__