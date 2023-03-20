extern "C" {

// Totally guessing here... Hopefully they are all standard enums...
typedef int _Unwind_Reason_Code;
typedef int _Unwind_Action;
typedef int _Unwind_Exception_Class;

_Unwind_Reason_Code
__gxx_personality_v0
(int version,
 _Unwind_Action actions,
 _Unwind_Exception_Class exception_class,
 struct _Unwind_Exception *ue_header,
 struct _Unwind_Context *context
 )
{
    // Literally have no clue what this is. Guess and checking it...
    return 7;
}


}
