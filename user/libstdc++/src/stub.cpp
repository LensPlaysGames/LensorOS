#include <stdint.h>
#include <sys/syscalls.h>

// See: https://itanium-cxx-abi.github.io/cxx-abi/abi-eh.html#base-personality

typedef int _Unwind_Action;

/// Indicates that the personality routine should check if the current
/// frame contains a handler, and if so return _URC_HANDLER_FOUND, or
/// otherwise return _URC_CONTINUE_UNWIND. _UA_SEARCH_PHASE cannot be
/// set at the same time as _UA_CLEANUP_PHASE.
static const _Unwind_Action _UA_SEARCH_PHASE = 1;

/// Indicates that the personality routine should perform cleanup for
/// the current frame. The personality routine can perform this cleanup
/// itself, by calling nested procedures, and return
/// _URC_CONTINUE_UNWIND. Alternatively, it can setup the registers
/// ( including the IP) for transferring control to a "landing pad",
/// and return _URC_INSTALL_CONTEXT.
static const _Unwind_Action _UA_CLEANUP_PHASE = 2;

/// During phase 2, indicates to the personality routine that the
/// current frame is the one which was flagged as the handler frame
/// during phase 1. The personality routine is not allowed to change
/// its mind between phase 1 and phase 2, i.e. it must handle the
/// exception in this frame in phase 2.
static const _Unwind_Action _UA_HANDLER_FRAME = 4;

/// During phase 2, indicates that no language is allowed to "catch"
/// the exception. This flag is set while unwinding the stack for
/// longjmp or during thread cancellation. User-defined code in a catch
/// clause may still be executed, but the catch clause must resume
/// unwinding with a call to _Unwind_Resume when finished.
static const _Unwind_Action _UA_FORCE_UNWIND = 8;

typedef enum {
    _URC_NO_REASON = 0,
    _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
    _URC_FATAL_PHASE2_ERROR = 2,
    _URC_FATAL_PHASE1_ERROR = 3,
    _URC_NORMAL_STOP = 4,
    _URC_END_OF_STACK = 5,
    _URC_HANDLER_FOUND = 6,
    _URC_INSTALL_CONTEXT = 7,
    _URC_CONTINUE_UNWIND = 8
} _Unwind_Reason_Code;

/// The unwind interface uses a pointer to an exception header object
/// as its representation of an exception being thrown. In general, the
/// full representation of an exception object is language- and
/// implementation-specific, but it will be prefixed by a header
/// understood by the unwind interface, defined as follows:
typedef void (*_Unwind_Exception_Cleanup_Fn)
(_Unwind_Reason_Code reason,
 struct _Unwind_Exception *exc);

struct _Unwind_Exception {
    uint64_t exception_class;
    _Unwind_Exception_Cleanup_Fn exception_cleanup;
    uint64_t private_1;
    uint64_t private_2;
};

typedef uint64_t _Unwind_Exception_Class;

/*
// PERSONALITY FUNCTION
extern "C" {
_Unwind_Reason_Code
__gxx_personality_v0
(
 /// Version number of the unwinding runtime, used to detect a mis-
 /// match between the unwinder conventions and the personality
 /// routine, or to provide backward compatibility.
 int version,

 /// Indicates what processing the personality routine is expected to
 /// perform, as a bit mask. The possible actions are described below.
 _Unwind_Action actions,

 /// An 8-byte identifier specifying the type of the thrown exception.
 /// By convention, the high 4 bytes indicate the vendor (for instance
 /// HP\0\0), and the low 4 bytes indicate the language.
 /// The low 4 bytes are expected to be "C++\0" for any C++ language
 /// exceptions.
 _Unwind_Exception_Class exception_class,

 /// The pointer to a memory location recording the necessary
 /// information for processing the exception according to the
 /// semantics of a given language (see the Exception Header section).
 struct _Unwind_Exception *ue_header,

 /// Unwinder state information for use by the personality routine.
 /// This is an opaque handle used by the personality routine in
 /// particular to access the frame's registers
 /// (see the Unwind Context section).
 struct _Unwind_Context *context
 ) noexcept
{
    std::__detail::syscall(SYS_exit, -1);
    return _URC_NORMAL_STOP;
}
} // extern "C"
*/
