## Fixed winding of triangles of flying edges on GPUs

The flying edges implementation has an optimization where it will traverse
meshes in the Y direction rather than the X direction on the GPU. It
created mostly correct results, but the triangles' winding was the opposite
from the CPU. This was mostly problematic when normals were generated from
the gradients. In this case, the gradient would point from the "back" of
the face, and that can cause shading problems with some renderers.

This has been fixed to make the windings consistent on the GPU with the CPU
and the gradients.
