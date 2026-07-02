//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/interop/anari/ANARILoadDevice.h>

#include <viskores/cont/Logging.h>

#include <viskores/rendering/anari-device/ViskoresDevice.h>

#include <utility>

namespace
{

static void AnariStatusFunc(const void* viskoresNotUsed(userData),
                            ANARIDevice viskoresNotUsed(device),
                            ANARIObject source,
                            ANARIDataType viskoresNotUsed(sourceType),
                            ANARIStatusSeverity severity,
                            ANARIStatusCode viskoresNotUsed(code),
                            const char* message)
{
  viskores::cont::LogLevel level;
  switch (severity)
  {
    case ANARI_SEVERITY_FATAL_ERROR:
      level = viskores::cont::LogLevel::Fatal;
      break;
    case ANARI_SEVERITY_ERROR:
      level = viskores::cont::LogLevel::Error;
      break;
    case ANARI_SEVERITY_WARNING:
    case ANARI_SEVERITY_PERFORMANCE_WARNING:
      level = viskores::cont::LogLevel::Warn;
      break;
    case ANARI_SEVERITY_INFO:
      level = viskores::cont::LogLevel::Info;
      break;
    case ANARI_SEVERITY_DEBUG:
    default:
      level = viskores::cont::LogLevel::UserVerboseFirst;
      break;
  }

  VISKORES_LOG_S(level, "[ANARI Object " << source << "] " << message);
}

} // anonymous namespace

namespace viskores
{
namespace interop
{
namespace anari
{

ANARILoadedDevice::ANARILoadedDevice(anari_cpp::Library library, anari_cpp::Device device) noexcept
  : LibraryHandle(library)
  , DeviceHandle(device)
{
}

ANARILoadedDevice::~ANARILoadedDevice()
{
  this->Reset();
}

ANARILoadedDevice::ANARILoadedDevice(ANARILoadedDevice&& other) noexcept
  : LibraryHandle(std::exchange(other.LibraryHandle, nullptr))
  , DeviceHandle(std::exchange(other.DeviceHandle, nullptr))
{
}

ANARILoadedDevice& ANARILoadedDevice::operator=(ANARILoadedDevice&& other) noexcept
{
  if (this != &other)
  {
    this->Reset();
    this->LibraryHandle = std::exchange(other.LibraryHandle, nullptr);
    this->DeviceHandle = std::exchange(other.DeviceHandle, nullptr);
  }
  return *this;
}

ANARILoadedDevice::operator bool() const noexcept
{
  return this->DeviceHandle != nullptr;
}

anari_cpp::Device ANARILoadedDevice::GetDevice() const noexcept
{
  return this->DeviceHandle;
}

void ANARILoadedDevice::Reset() noexcept
{
  if (this->DeviceHandle != nullptr)
  {
    anari_cpp::release(this->DeviceHandle, this->DeviceHandle);
    this->DeviceHandle = nullptr;
  }
  if (this->LibraryHandle != nullptr)
  {
    anari_cpp::unloadLibrary(this->LibraryHandle);
    this->LibraryHandle = nullptr;
  }
}

ANARILoadedDevice ANARILoadDevice(const std::string& libraryName)
{
  if (libraryName == "viskores")
  {
    VISKORES_LOG_F(viskores::cont::LogLevel::Info,
                   "Directly loading internal ANARI device named `viskores`");
    auto device = reinterpret_cast<anari_cpp::Device>(
      new viskores_device::ViskoresDevice(AnariStatusFunc, nullptr));
    return ANARILoadedDevice(nullptr, device);
  }
  else
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "Loading ANARI library named `" << libraryName << "`");
    anari_cpp::Library library =
      anari_cpp::loadLibrary(libraryName.c_str(), AnariStatusFunc, nullptr);
    if (library == nullptr)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Failed to load ANARI library named `" << libraryName << "`");
      return {};
    }
    anari_cpp::Device device = anari_cpp::newDevice(library, "default");
    if (device == nullptr)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Failed to load ANARI device from library named `" << libraryName << "`");
      anari_cpp::unloadLibrary(library);
      return {};
    }
    return ANARILoadedDevice(library, device);
  }
}

}
}
} // namespace viskores::interop::anari
