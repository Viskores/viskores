## Add GPU and CPU compile job pools

When using Ninja, Viskores targets with CUDA or HIP device sources now compile
in `viskores_pool_gpu` and all other targets in `viskores_pool_cpu`. Set
`viskores_gpu_pool_size` or `viskores_cpu_pool_size` to change the pool sizes.
The `USE_VISKORES_JOB_POOL` option of `viskores_library` and
`viskores_unit_tests` and the `viskores_pool` it selected were removed.
