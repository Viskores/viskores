## Avoid copies on AMD APUs in the Kokkos build of Viskores

APUs, such as the AMD Instinct MI300A, are a special kind of accelerator where the CPU and GPU physically share their memory.
The Kokkos build of Viskores now takes this characteristic into account by defining `VISKORES_PHYSICALLY_SHARED_MEMORY` and performing shallow copies between host and device, if the device is an APU.
