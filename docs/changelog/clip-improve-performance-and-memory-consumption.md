## Clip: Improve performance and memory consumption

The following set of improvements have been implemented for the Clip algorithm:

1. Input points that are kept are determined by comparing their scalar value with the isovalue, instead of checking the
   output cells' connectivity.
2. Output arrays are written only once, and they are not transformed. Due to that, no auxiliary arrays are needed to
   perform the transformations.
3. A fast path for discarded and kept cells has been added, which are the most common cell cases.
4. ClipTables are now more descriptive, and the non-inverted case tables have been imported from VTK, such that both VTK
   and VTK-m produce the same results.
5. Employ batching of points and cells to use less memory and perform less and faster computations.

The new `Clip` algorithm:

On the CPU:

1. Batch size = min(1000, max(1, numberOfElements / 250000)).
2. Memory-footprint (the bigger the dataset the greater the benefit):
    1. For almost nothing is kept: 10.22x to 99.67x less memory footprint
    2. For almost half is kept: 2.62x to 4.30x less memory footprint
    3. For almost everything is kept: 2.38x to 3.21x less memory footprint
3. Performance (the bigger the dataset the greater the benefit):
    1. For almost nothing is kept: 1.63x to 7.79x faster
    2. For almost half is kept: 1.75x to 5.28x faster
    3. For almost everything is kept: 1.71x to 5.35x faster

On the GPU:

1. Batch size = 6.
2. Memory-footprint (the bigger the dataset the greater the benefit):
    1. For almost nothing is kept: 1.71x to 7.75x less memory footprint
    2. For almost half is kept: 1.11x to 1.36x less memory footprint
    3. For almost everything is kept: 1.09x to 1.31x less memory footprint
3. Performance (the bigger the dataset the greater the benefit):
    1. For almost nothing is kept: 1.54x to 9.67x faster
    2. For almost half is kept: 1.38x to 4.68x faster
    3. For almost everything is kept: 1.21x to 4.46x faster
