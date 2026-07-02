## Keep dynamically loaded ANARI devices valid

`viskores::interop::anari::ANARILoadDevice` now returns a move-only
`ANARILoadedDevice` owner instead of a raw `anari_cpp::Device`. The owner keeps
the dynamic ANARI library loaded for the lifetime of its device, then releases
the device before unloading the library.

Keep the returned owner alive while using its raw device handle:

```cpp
auto loadedDevice = viskores::interop::anari::ANARILoadDevice("helide");
if (loadedDevice)
{
  auto device = loadedDevice.GetDevice();
  // Use device, releasing all objects before loadedDevice is destroyed.
}
```

Code using the previous return type must retain the `ANARILoadedDevice`, call
`GetDevice()` when a raw handle is needed, and stop releasing the device
directly. An implicit conversion to a raw handle is intentionally unavailable:
such a conversion would allow a temporary owner to unload the library while
the raw handle remains in use.
