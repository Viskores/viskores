# Add support for deferred freeing of cuda memory

A new function, `void CudaAllocator::FreeDeferred(void* ptr, std::size_t numBytes)` has
been added that can be used to defer the freeing of cuda memory to a later point.
This is useful because `cudaFree` causes a global sync across all cuda streams. This function
internally maintains a pool of to-be-freed pointers that are freed together when a
size threshold is reached. This way a number of global syncs are collected together at
one point.
