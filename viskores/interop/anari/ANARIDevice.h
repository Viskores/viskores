//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_interop_anari_ANARILoadLibrary_h
#define viskores_interop_anari_ANARILoadLibrary_h

#include <viskores/Assert.h>
#include <viskores/interop/anari/ViskoresANARITypes.h>
#include <viskores/interop/anari/viskores_anari_export.h>

#include <memory>
#include <type_traits>

namespace viskores
{
namespace interop
{
namespace anari
{

/// @brief A management object for an ANARI device.
///
/// This class provides a convenience wrapper around an ANARI device object
/// (`anari::Device`). It provides two main functions.
///
/// First, this object behaves as a reference management for the ANARI device
/// object. The ANARI device will be held as long as it is referenced, and it
/// will be deleted once it is no longer used.
///
/// Second, this object will automatically load the ANARI device by name. By
/// default, it will load the device compiled with Viskores, but it can find
/// other libraries by name. As the library is loaded, it will connect its
/// logging to Viskores' logging.
///
/// By default, `ANARIDevice` will load the `viskores` device. However, you can
/// use this class to load other ANARI devices by name.
///
/// This class can be used in place of an `anari::Device` object. When passed
/// to a function that expects an `anari::Device`, the object will get
/// automatically converted.
///
class VISKORES_ANARI_EXPORT ANARIDevice
{
  std::shared_ptr<std::remove_pointer_t<anari_cpp::Device>> Device;
  std::string DeviceName = "viskores";

public:
  ANARIDevice() = default;

  /// @brief Sets the name of the device to load.
  ///
  /// If a device of a different name is already loaded, it will be unloaded.
  void SetDeviceName(const std::string& deviceName);

  /// @brief Returns the name of the device loaded or to be loaded.
  std::string GetDeviceName() const { return this->DeviceName; }

  /// @brief Returns the currently loaded device.
  ///
  /// If the device has not already been loaded, it will be loaded first. If it
  /// fails to load the device, it will throw `viskores::cont::ErrorBadDevice`.
  ///
  /// @return An ANARI device object.
  anari_cpp::Device GetDevice()
  {
    if (!this->IsLoaded())
    {
      this->EnsureLoad();
    }
    VISKORES_ASSERT(this->Device);
    return this->Device.get();
  }

  /// @brief Automatically convert this object to an ANARI device.
  operator anari_cpp::Device() { return this->GetDevice(); }

  /// @brief Returns whether the ANARI device is already loaded.
  bool IsLoaded() const { return static_cast<bool>(this->Device); }

  /// @brief Returns whether the ANARI device is valid.
  ///
  /// If the device is not already loaded, this method will attempt to load it.
  /// No exception will be thrown if the device fails to load.
  bool IsValid();

  /// @brief Ensure that the ANARI device is loaded.
  ///
  /// This method will attempt to load the ANARI device if it is not already
  /// loaded. If the device fails to load, it will throw
  /// `viskores::cont::ErrorBadDevice`.
  void EnsureLoad();

  /// @brief Reloads the ANARI device.
  ///
  /// This method will unload any currently loaded device and attempt to load the
  /// device. If the device fails to load, it will throw
  /// `viskores::cont::ErrorBadDevice`.
  void Reload();

  /// @brief Frees the current device.
  void Unload();

private:
  void AttemptLoad();
};

}
}
} // namespace viskores::interop::anari

#endif //viskores_interop_anari_ANARILoadLibrary_h
