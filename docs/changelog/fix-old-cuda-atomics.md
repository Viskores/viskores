# Fix old cuda atomics

There are some overloads for atomic adds of floating point numbers for
older versions of cuda that do not include them directly. These were
misnamed and thus not properly overloading the generic implementation.
This caused compile errors with older versions of cuda.
