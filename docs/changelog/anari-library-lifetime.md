## Fix ANARI library being unloaded while its device is in use

`viskores::interop::anari::ANARILoadDevice` unloaded the `ANARILibrary` immediately
after creating the device from it. The ANARI specification requires a library to
remain loaded until all devices created from it are released, and newer versions
of the ANARI-SDK enforce this. The loaded library is now kept for the life of the
program (and reused if the same library is requested again).
