# CudaAllocator Managed Memory can be disabled from C++

Previously it was impossible for calling code to explicitly
disable managed memory. This can be desirable for projects
that know they don't need managed memory and are super
performance critical.
