## Fix ANARI ViskoresLibrary getDeviceExtensions to return device extensions
The ANARI device returned a nullptr for `getDeviceExtensions` which meant 
that any client (e.g. Paraview, but also ANARI-SDK examples) checking 
features via `anariGetDeviceExtensions` failed.

Example from calling `anariInfo` from ANARI-SDK:

Before:
```bash
➜  bin ./anariInfo -l viskores | grep -A 10 Extensions
[DEBUG] destroying Viskores device (0x10445ea20)
```

After fix:
```bash
➜  bin ./anariInfo -l viskores | grep -A 10 Extensions
[DEBUG] destroying Viskores device (0x10557ea20)
   Extensions:
      ANARI_KHR_CAMERA_ORTHOGRAPHIC
      ANARI_KHR_CAMERA_PERSPECTIVE
      ANARI_KHR_GEOMETRY_CYLINDER
      ANARI_KHR_GEOMETRY_QUAD
      ANARI_KHR_GEOMETRY_SPHERE
      ANARI_KHR_GEOMETRY_TRIANGLE
      ANARI_KHR_MATERIAL_MATTE
      ANARI_KHR_SAMPLER_IMAGE1D
      ANARI_KHR_VOLUME_TRANSFER_FUNCTION1D
   Subtypes:

```
