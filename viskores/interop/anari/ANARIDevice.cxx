//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/interop/anari/ANARIDevice.h>

#include <viskores/cont/ErrorBadDevice.h>
#include <viskores/cont/Logging.h>

#include <viskores/rendering/anari-device/ViskoresDevice.h>

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

void ANARIDevice::SetDeviceName(const std::string& deviceName)
{
  if (deviceName != this->DeviceName)
  {
    this->Unload();
    this->DeviceName = deviceName;
  }
  else
  {
    // NoOp
  }
}

bool ANARIDevice::IsValid()
{
  this->AttemptLoad();
  return this->IsLoaded();
}

void ANARIDevice::EnsureLoad()
{
  this->AttemptLoad();
  if (!this->IsLoaded())
  {
    throw viskores::cont::ErrorBadDevice("Failed to load ANARI device named " + this->DeviceName);
  }
}

void ANARIDevice::Reload()
{
  this->Unload();
  this->EnsureLoad();
}

void ANARIDevice::Unload()
{
  this->Device.reset();
}

void ANARIDevice::AttemptLoad()
{
  if (this->IsLoaded())
  {
    return;
  }

  if (this->DeviceName == "viskores")
  {
    VISKORES_LOG_F(viskores::cont::LogLevel::Info,
                   "Directly loading internal ANARI device named `viskores`");
    this->Device.reset(reinterpret_cast<anari_cpp::Device>(
      new viskores_device::ViskoresDevice(AnariStatusFunc, nullptr)));
  }
  else
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "Loading ANARI library named `" << this->DeviceName << "`");
    anari_cpp::Library library =
      anari_cpp::loadLibrary(this->DeviceName.c_str(), AnariStatusFunc, nullptr);
    if (library == nullptr)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Failed to load ANARI library named `" << this->DeviceName << "`");
      return;
    }
    anari_cpp::Device device = anari_cpp::newDevice(library, "default");
    anari_cpp::unloadLibrary(library);
    if (device != nullptr)
    {
      this->Device.reset(device);
    }
    else
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Failed to load ANARI device from library named `" << this->DeviceName << "`");
    }
  }
}

}
}
} // namespace viskores::interop::anari
