//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_interop_anari_ANARILoadDevice_h
#define viskores_interop_anari_ANARILoadDevice_h

#include <viskores/interop/anari/ViskoresANARITypes.h>
#include <viskores/interop/anari/viskores_anari_export.h>

#include <string>

namespace viskores
{
namespace interop
{
namespace anari
{

class ANARILoadedDevice;

/// @brief Loads an ANARI device from the library of the provided name.
///
/// Given the name of an ANARI device library, attempts to find the associated
/// library (named `anari_library_` plus the provided `libraryName` with the
/// standard shared object library prefix and extension for the current system),
/// opens it, and returns an object that owns both the ANARI device and its
/// library. This is a convenience function that essentially uses
/// `anari::loadLibrary` to open the library and `anari::newDevice` to create
/// the device.
///
/// An additional feature of this method is that if you provide the string
/// `viskores` for `libraryName`, this function will be able to load the device
/// without having to find the device on the system. Typically, to find the
/// library you need an environment variable such as `LD_LIBRARY_PATH` or an
/// internal library rpath pointing to the directory containing the library. For
/// the "viskores" library, the device will be loaded directly regardless of its
/// location.
///
/// This method will also link the ANARI device's logging to the Viskores logging
/// so the ANARI logging can be controlled through Viskores' configuration.
///
/// @param libraryName The name of the library to load.
/// @return An owning loaded device, or an invalid object if the library or
/// device cannot be opened.
VISKORES_ANARI_EXPORT VISKORES_CONT ANARILoadedDevice
ANARILoadDevice(const std::string& libraryName);

/// @brief Owns an ANARI device and the library from which it was loaded.
///
/// This move-only object releases its device before unloading the associated
/// dynamic library. The raw device returned by `GetDevice` is valid only while
/// this object remains alive. All objects and additional retained references
/// associated with the raw device must be released before this object.
class VISKORES_ANARI_EXPORT ANARILoadedDevice
{
public:
  /// Creates an invalid loaded device.
  VISKORES_CONT ANARILoadedDevice() noexcept = default;

  /// Releases the device, then unloads its dynamic library, if any.
  VISKORES_CONT ~ANARILoadedDevice();

  /// Loaded devices cannot be copied because they uniquely own their handles.
  ANARILoadedDevice(const ANARILoadedDevice&) = delete;
  ANARILoadedDevice& operator=(const ANARILoadedDevice&) = delete;

  /// Transfers ownership from another loaded device.
  VISKORES_CONT ANARILoadedDevice(ANARILoadedDevice&& other) noexcept;

  /// Releases current ownership and transfers ownership from another loaded device.
  VISKORES_CONT ANARILoadedDevice& operator=(ANARILoadedDevice&& other) noexcept;

  /// Returns whether this object owns a valid ANARI device.
  VISKORES_CONT explicit operator bool() const noexcept;

  /// Returns the non-owning raw ANARI device handle.
  VISKORES_CONT anari_cpp::Device GetDevice() const noexcept;

private:
  friend ANARILoadedDevice ANARILoadDevice(const std::string& libraryName);

  VISKORES_CONT ANARILoadedDevice(anari_cpp::Library library, anari_cpp::Device device) noexcept;
  VISKORES_CONT void Reset() noexcept;

  anari_cpp::Library LibraryHandle = nullptr;
  anari_cpp::Device DeviceHandle = nullptr;
};

}
}
} // namespace viskores::interop::anari


#endif //viskores_interop_anari_ANARILoadDevice_h
