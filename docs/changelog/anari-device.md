## New ANARI device enabled by Viskores

Viskores now provides an ANARI device that can be used by applications that use
ANARI for rendering. The intention of this device is to provide a simple,
portable, accelerated ANARI device that will be available in HPC systems
regardless of vendor support. We hope this will help jumpstart the support of
ANARI for scientific visualization applications at HPC centers.

The anari interop library now contains a class named `ANARIDevice` that manages
an ANARI device. In addition to making sure resources get created and destroyed
correctly, it can find the built in Viskores device without having to rely on
the user's environment.

This device is still in its experimental phase. Although functional, it is
missing many features that applications will expect to be supported. The
inclusion of the device into Viskores in this early phase should help promote
development and simplify the integration of any changes necessary in the
rendering library.
