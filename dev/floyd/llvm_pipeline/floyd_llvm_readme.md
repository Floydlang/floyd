#  <#Title#>




/*
# LLVM Context

http://blog.audio-tk.com/2018/09/18/compiling-c-code-in-memory-with-clang/
With LLVM, we also have some things to be careful about. The first is the LLVM context we created before needs to stay alive as long as we use anything from this compilation unit. This is important because everything that is generated with the JIT will have to stay alive after this function and registers itself in the context until it is explicitly deleted.

*/


LLVM returning structs: only support 2 members of 64bit each.


# ANY VALUES

When used in function arguments, we use two uint64_t arguments, first is the value or its pointer, the second is its itype.

When returning DYN from a function we always know at compile time which type so we just return the value, as a WIDE_RETURN_T. This lets us copy a lot of stuff directly in the return value.






# FLOYD TYPES

bool
int
double
typeid

string
function?
struct
vector
dictionary

json



??? Build thin language on top of LLVM backend. It can do more low-level types, like vector_rc[uint8]. Some are directly translated to IR, some are provided by floyd runtime, like heap allocation and RC.


