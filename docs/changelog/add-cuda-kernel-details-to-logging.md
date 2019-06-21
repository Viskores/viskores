# VTK-m logs details about each CUDA kernel launch

The VTK-m logging infrastructure has been extended with a new log level
`KernelLaunches` which exists between `MemTransfer` and `Cast`.

This log level reports the number of blocks, threads per block, and the
PTX version of each CUDA kernel launched.

This logging level was primarily introduced to help developers that are
tracking down issues that occur when VTK-m components have been built with
different `sm_XX` flags and help people looking to do kernel performance
tuning.
