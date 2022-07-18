# Divided the mesh quality filter

The original implementation of the `MeshQuality` filter created one large
kernel with a switch statement that jumped to the code of the metric
actually desired. This is problematic for a couple of reasons. First, it
takes the compiler a long time to optimize for all the inlined cases of a
large kernel. Second, it creates a larger than necessary function that has
to be loaded onto the GPU to execute.

The code was modified to move the switch statement outside of the GPU
kernel. Instead, the routine for each metric is compiled into its own
kernel. For convenience, each routine is wrapped into its own independent
filter (e.g., `MeshQualityArea`, `MeshQualityVolume`). The uber
`MeshQuality` filter still exists, and its use is still encouraged even if
you only need a particular metric. However, internally the switch statement
now occurs on the host to select the appropriate specific filter that loads
a more targeted kernel.
