//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_kokkos_internal_RuntimeDeviceConfigurationKokkos_h
#define vtk_m_cont_kokkos_internal_RuntimeDeviceConfigurationKokkos_h

#include <vtkm/cont/ErrorInternal.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>
#include <vtkm/cont/kokkos/internal/DeviceAdapterTagKokkos.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <Kokkos_Core.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <cstring>
#include <vector>

namespace vtkm
{
namespace cont
{
namespace internal
{

namespace
{
VTKM_CONT
RuntimeDeviceConfigReturnCode GetArgFromList(const std::vector<std::string>& argList,
                                             const std::string& argName,
                                             vtkm::Id& value)
{
  size_t pos;
  try
  {
    for (auto argItr = argList.rbegin(); argItr != argList.rend(); argItr++)
    {
      if (argItr->rfind(argName, 0) == 0)
      {
        if (argItr->size() == argName.size())
        {
          value = std::stoi(*(--argItr), &pos, 10);
          return RuntimeDeviceConfigReturnCode::SUCCESS;
        }
        else
        {
          value = std::stoi(argItr->substr(argName.size() + 1), &pos, 10);
          return RuntimeDeviceConfigReturnCode::SUCCESS;
        }
      }
    }
  }
  catch (const std::invalid_argument&)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "Unable to get arg " + argName +
                 "from kokkos argList, invalid argument thrown... This shouldn't have happened");
    return RuntimeDeviceConfigReturnCode::INVALID_VALUE;
  }
  catch (const std::out_of_range&)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "Unable to get arg " + argName +
                 "from kokkos argList, out of range thrown... This shouldn't have happened");
    return RuntimeDeviceConfigReturnCode::INVALID_VALUE;
  }
  return RuntimeDeviceConfigReturnCode::NOT_APPLIED;
}

} // namespace anonymous

template <>
class RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagKokkos>
  : public vtkm::cont::internal::RuntimeDeviceConfigurationBase
{
public:
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override final
  {
    return vtkm::cont::DeviceAdapterTagKokkos{};
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(const vtkm::Id& value) override final
  {
    if (Kokkos::is_initialized())
    {
      VTKM_LOG_S(
        vtkm::cont::LogLevel::Warn,
        "SetThreads was called but Kokkos was already initailized! Updates will not be applied.");
      return RuntimeDeviceConfigReturnCode::NOT_APPLIED;
    }
    this->KokkosArguments.insert(this->KokkosArguments.begin(),
                                 "--kokkos-num-threads=" + std::to_string(value));
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetDeviceInstance(
    const vtkm::Id& value) override final
  {
    if (Kokkos::is_initialized())
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "SetDeviceInstance was called but Kokkos was already initailized! Updates will "
                 "not be applied.");
      return RuntimeDeviceConfigReturnCode::NOT_APPLIED;
    }
    this->KokkosArguments.insert(this->KokkosArguments.begin(),
                                 "--kokkos-device-id=" + std::to_string(value));
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(vtkm::Id& value) const override final
  {
    return GetArgFromList(this->KokkosArguments, "--kokkos-num-threads", value);
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetDeviceInstance(
    vtkm::Id& value) const override final
  {
    return GetArgFromList(this->KokkosArguments, "--kokkos-device-id", value);
  }

protected:
  /// Store a copy of the current arguments when initializing the Kokkos subsystem later
  /// Appends a copy of the argv values in the KokkosArguments vector: this assumes the
  /// argv values contain kokkos command line arguments (like --kokkos-num-threads, etc)
  VTKM_CONT virtual void ParseExtraArguments(int& argc, char* argv[]) override final
  {
    if (argc > 0 && argv)
    {
      this->KokkosArguments.insert(this->KokkosArguments.end(), argv, argv + argc);
    }
  }

  /// Calls kokkos initiailze if kokkos has not been initialized yet and sets up an atexit
  /// to call kokkos finalize. Converts the KokkosArguments vector to a standard argc/argv
  /// list of arguments when calling kokkos initialize.
  ///
  /// When using vtkm::Initialize, the standard order for kokkos argument priority is as
  /// follows (this assumes kokkos still prioritizes arguments found at the end of the
  /// argv list over similarly named arguements found earlier in the list):
  ///   1. Environment Variables
  ///   2. Kokkos Command Line Arguments
  ///   3. VTK-m Interpreted Command Line Arguements
  VTKM_CONT virtual void InitializeSubsystem() override final
  {
    if (!Kokkos::is_initialized())
    {
      std::vector<char*> argv;
      for (auto& arg : this->KokkosArguments)
      {
        argv.push_back(&arg[0]);
      }
      int size = argv.size();
      Kokkos::initialize(size, argv.data());
      std::atexit(Kokkos::finalize);
    }
    else
    {
      VTKM_LOG_S(
        vtkm::cont::LogLevel::Warn,
        "Attempted to Re-initialize Kokkos! The Kokkos subsystem can only be initialized once");
    }
  }

private:
  std::vector<std::string> KokkosArguments;
};

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_kokkos_internal_RuntimeDeviceConfigurationKokkos_h
