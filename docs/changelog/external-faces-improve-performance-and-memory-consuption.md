## ExternalFaces: Improve performance and memory consumption

`ExternalFaces` now uses a new algorithm that has the following combination of novel characteristics:

1. employs minimum point id as its face hash function instead of FNV1A of canonicalFaceID, that yields cache-friendly
   memory accesses which leads to enhanced performance.
2. employs an atomic hash counting approach, instead of hash sorting, to perform a reduce-by-key operation on the faces'
   hashes.
3. enhances performance by ensuring that the computation of face properties occurs only once and by avoiding the
   processing of a known internal face more than once.
4. embraces frugality in memory consumption, enabling the processing of larger datasets, especially on GPUs.

When evaluated on Frontier super-computer on 4 large datasets:

The new `ExternalFaces` algorithm:

1. has 4.73x to 5.99x less memory footprint
2. is 4.96x to 7.37x faster on the CPU
3. is 1.54x faster on the GPU, and the original algorithm could not execute on large datasets due to its high memory
   footprint
