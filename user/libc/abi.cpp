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


#include <bits/decls.h>
#include <bits/abi.h>
#include <stdlib.h>
#include <unistd.h>


/// ===========================================================================
///  Extern symbols.
/// ===========================================================================
__BEGIN_DECLS__
using init_cb = void (*)();

/// Global constructors and destructors. These symbols are provided by the linker.
extern init_cb __preinit_array_start[];
extern init_cb __preinit_array_end[];
extern init_cb __init_array_start[];
extern init_cb __init_array_end[];
extern init_cb __fini_array_start[];
extern init_cb __fini_array_end[];

bool __in_quick_exit = false;
__END_DECLS__

/// ===========================================================================
///  Internal state and implementation.
/// ===========================================================================
namespace {
struct dso_atexit_callback {
    __dso_cb cb;
    __dso_cb_arg arg;
    dso_atexit_callback* next;
};

struct dso_atexit_list {
    __dso_handle_t dso;
    bool processed;
    dso_atexit_callback* callbacks;
    dso_atexit_list* next;
};

dso_atexit_list* dso_atexit_list_head = nullptr;

dso_atexit_list* find_dso_atexit_list(__dso_handle_t dso) {
    for (dso_atexit_list* list = dso_atexit_list_head; list; list = list->next) {
        if (list->dso == dso) {
            return list;
        }
    }
    return nullptr;
}

} // namespace

/// ===========================================================================
///  Interface.
/// ===========================================================================
__BEGIN_DECLS__

/// TODO: Figure out why this *is* defined by the compiler when compiling
///       for LensorOS, but not when compiling for the host.
#ifndef __lensor__
[[gnu::used]] void* __dso_handle;
#endif

/// Malloc *must* be initialised before anything else.
void __libc_init_malloc();
void __libc_fini_malloc();

/// We need to pass envp to main too.
int main(int argc, char** argv, char** envp);

/// Registers a function to be called when `dso` is unloaded.
void __cxa_atexit(__dso_cb func, __dso_cb_arg arg, __dso_handle_t dso) {
    /// Find the dso atexit list.
    auto* list = find_dso_atexit_list(dso);
    if (!list) {
        list = new dso_atexit_list;
        list->dso = dso;
        list->callbacks = nullptr;
        list->next = dso_atexit_list_head;
        dso_atexit_list_head = list;
    }

    /// Add the callback to the list.
    auto* callback = new dso_atexit_callback;
    callback->cb = func;
    callback->arg = arg;
    callback->next = list->callbacks;
    list->callbacks = callback;
}

/// Unload `dso` and call all its atexit callbacks.
///
/// The callbacks are called in reverse order of registration. Due to the way
/// the list is constructed, we can just iterate over the list and call the
/// callbacks in iteration order.
void __cxa_finalize(__dso_handle_t dso) {
    /// If dso is nullptr, call all atexit callbacks.
    if (!dso) {
        for (auto* list = dso_atexit_list_head; list && !list->processed; list = list->next) {
            list->processed = true;
            for (auto* callback = list->callbacks; callback; callback = callback->next) {
                callback->cb(callback->arg);
            }
        }
        return;
    }

    /// Find the dso atexit list.
    auto* list = find_dso_atexit_list(dso);
    if (!list || list->processed) { return; }

    /// Call all the callbacks.
    list->processed = true;
    for (auto* callback = list->callbacks; callback; callback = callback->next) {
        callback->cb(callback->arg);
    }
}

/// Call global constructors.
void __libc_init() noexcept {
    __libc_init_malloc();
    for (init_cb* cb = __preinit_array_start; cb != __preinit_array_end; ++cb) { (*cb)(); }
    for (init_cb* cb = __init_array_start; cb != __init_array_end; ++cb) {
        (*cb)();
    }
}

/// Call global destructors.
void __libc_fini() noexcept {
    for (init_cb* cb = __fini_array_start; cb != __fini_array_end; ++cb) { (*cb)(); }
    __cxa_finalize(nullptr);
    __libc_fini_malloc();
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

/// Call global destructors and exit the program.
__attribute__((noreturn)) void __libc_quick_exit(int status) {
    __in_quick_exit = true;
    __libc_exit(status);
}

__END_DECLS__