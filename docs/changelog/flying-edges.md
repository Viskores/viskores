# Flying Edges

Added the flying edges contouring algorithm to VTK-m. This algorithm only
works on structured grids, but operates much faster than the traditional
Marching Cubes algorithm.

The speed of VTK-m's flying edges is comprable to VTK's running on the same
CPUs. VTK-m's implementation also works well on CUDA hardware.

The Flying Edges algorithm was introduced in this paper:

Schroeder, W.; Maynard, R. & Geveci, B.
"Flying edges: A high-performance scalable isocontouring algorithm."
Large Data Analysis and Visualization (LDAV), 2015.
DOI 10.1109/LDAV.2015.7348069
