## Simplify ANARI point lowering and validate attributes

`ANARIMapperPoints` now binds contiguous `Float32` coordinates directly and
converts other coordinate storage once to `FLOAT32_VEC3`. Uniform sphere size
uses ANARI's global `radius` parameter, with a 0.01 fallback for degenerate or
non-finite bounds. Point fields are exposed only when they have one through
four float-convertible components and exactly one value per coordinate; their
USD names are retained for scalar and vector attributes.

For a representative 65,536-point `Float32` data set, the lowering-owned host
buffers before and after this change are:

| Buffer | Before | After |
| --- | ---: | ---: |
| identity point IDs | 512 KiB | 0 KiB |
| copied positions | 768 KiB | 0 KiB |
| per-point uniform radii | 256 KiB | 0 KiB |
| total lowering-owned storage | 1,536 KiB | 0 KiB |

The mapper now presents one 768 KiB position array to ANARI instead of position
and radius arrays totaling 1,024 KiB. A data-inspection regression test records
the one-array count, byte count, and direct source pointer independently of
rendered pixels.
