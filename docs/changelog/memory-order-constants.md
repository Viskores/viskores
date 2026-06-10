## Fixed memory_order constants for C++20

C++20 changed the implementation of the `std::memory_order` enum to be scoped.
As part of that, the contents of the enum have different names. Instead, you
reference the identifiers as constants in the `std` namespace (which works the
same with the non-scoped version).

At any rate, the code now works for both C++17 and C++20.
