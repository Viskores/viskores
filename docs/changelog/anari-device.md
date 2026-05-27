## New ANARI device enabled by Viskores

Viskores now provides an ANARI device that can be used by applications that use
ANARI for rendering. The intention of this device is to provide a simple,
portable, accelerated ANARI device that will be available in HPC systems
regardless of vendor support. We hope this will help jumpstart the support of
ANARI for scientific visualization applications at HPC centers.

The ANARI interop library now contains a function named `ANARILoadDevice` that
loads an ANARI device. This function has a special mode for the device named
"viskores" such that it will load the device from the library directly, thereby
circumventing any configuration to find the library at runtime. This provides a
fallback that is always available.

This device is still in its experimental phase. Although functional, it is
missing many features that applications will expect to be supported. The
inclusion of the device into Viskores in this early phase should help promote
development and simplify the integration of any changes necessary in the
rendering library.
