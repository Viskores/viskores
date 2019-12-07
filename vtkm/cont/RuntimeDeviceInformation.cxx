//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/RuntimeDeviceInformation.h>

#include <vtkm/List.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/DeviceAdapterTag.h>

//Bring in each device adapters runtime class
#include <vtkm/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterRuntimeDetectorOpenMP.h>
#include <vtkm/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterRuntimeDetectorTBB.h>

#include <cctype> //for tolower

namespace
{
struct VTKM_NEVER_EXPORT InitializeDeviceNames
{
  vtkm::cont::DeviceAdapterNameType* Names;
  vtkm::cont::DeviceAdapterNameType* LowerCaseNames;

  VTKM_CONT
  InitializeDeviceNames(vtkm::cont::DeviceAdapterNameType* names,
                        vtkm::cont::DeviceAdapterNameType* lower)
    : Names(names)
    , LowerCaseNames(lower)
  {
    std::fill_n(this->Names, VTKM_MAX_DEVICE_ADAPTER_ID, "InvalidDeviceId");
    std::fill_n(this->LowerCaseNames, VTKM_MAX_DEVICE_ADAPTER_ID, "invaliddeviceid");
  }

  template <typename Device>
  VTKM_CONT void operator()(Device device)
  {
    auto lowerCaseFunc = [](char c) {
      return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    };

    auto id = device.GetValue();

    if (id > 0 && id < VTKM_MAX_DEVICE_ADAPTER_ID)
    {
      auto name = vtkm::cont::DeviceAdapterTraits<Device>::GetName();
      this->Names[id] = name;
      std::transform(name.begin(), name.end(), name.begin(), lowerCaseFunc);
      this->LowerCaseNames[id] = name;
    }
  }
};

struct VTKM_NEVER_EXPORT RuntimeDeviceInformationFunctor
{
  bool Exists = false;
  template <typename DeviceAdapter>
  VTKM_CONT void operator()(DeviceAdapter, vtkm::cont::DeviceAdapterId device)
  {
    if (DeviceAdapter() == device)
    {
      this->Exists = vtkm::cont::DeviceAdapterRuntimeDetector<DeviceAdapter>().Exists();
    }
  }
};
}

namespace vtkm
{
namespace cont
{
namespace detail
{

class RuntimeDeviceNames
{
public:
  static const DeviceAdapterNameType& GetDeviceName(vtkm::Int8 id)
  {
    return Instance().DeviceNames[id];
  }

  static const DeviceAdapterNameType& GetLowerCaseDeviceName(vtkm::Int8 id)
  {
    return Instance().LowerCaseDeviceNames[id];
  }

private:
  static const RuntimeDeviceNames& Instance()
  {
    static RuntimeDeviceNames instance;
    return instance;
  }

  RuntimeDeviceNames()
  {
    InitializeDeviceNames functor(DeviceNames, LowerCaseDeviceNames);
    vtkm::ListForEach(functor, VTKM_DEFAULT_DEVICE_ADAPTER_LIST());
  }

  friend struct InitializeDeviceNames;

  DeviceAdapterNameType DeviceNames[VTKM_MAX_DEVICE_ADAPTER_ID];
  DeviceAdapterNameType LowerCaseDeviceNames[VTKM_MAX_DEVICE_ADAPTER_ID];
};
}

VTKM_CONT
DeviceAdapterNameType RuntimeDeviceInformation::GetName(DeviceAdapterId device) const
{
  const auto id = device.GetValue();

  if (device.IsValueValid())
  {
    return detail::RuntimeDeviceNames::GetDeviceName(id);
  }
  else if (id == VTKM_DEVICE_ADAPTER_UNDEFINED)
  {
    return vtkm::cont::DeviceAdapterTraits<vtkm::cont::DeviceAdapterTagUndefined>::GetName();
  }
  else if (id == VTKM_DEVICE_ADAPTER_ANY)
  {
    return vtkm::cont::DeviceAdapterTraits<vtkm::cont::DeviceAdapterTagAny>::GetName();
  }

  // Deviceis invalid:
  return detail::RuntimeDeviceNames::GetDeviceName(0);
}

VTKM_CONT
DeviceAdapterId RuntimeDeviceInformation::GetId(DeviceAdapterNameType name) const
{
  // The GetDeviceAdapterId call is case-insensitive so transform the name to be lower case
  // as that is how we cache the case-insensitive version.
  auto lowerCaseFunc = [](char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  };
  std::transform(name.begin(), name.end(), name.begin(), lowerCaseFunc);

  //lower-case the name here
  if (name == "any")
  {
    return vtkm::cont::DeviceAdapterTagAny{};
  }
  else if (name == "undefined")
  {
    return vtkm::cont::DeviceAdapterTagUndefined{};
  }

  for (vtkm::Int8 id = 0; id < VTKM_MAX_DEVICE_ADAPTER_ID; ++id)
  {
    if (name == detail::RuntimeDeviceNames::GetLowerCaseDeviceName(id))
    {
      return vtkm::cont::make_DeviceAdapterId(id);
    }
  }

  return vtkm::cont::DeviceAdapterTagUndefined{};
}


VTKM_CONT
bool RuntimeDeviceInformation::Exists(DeviceAdapterId id) const
{
  if (id == vtkm::cont::DeviceAdapterTagAny{})
  {
    return true;
  }

  RuntimeDeviceInformationFunctor functor;
  vtkm::ListForEach(functor, VTKM_DEFAULT_DEVICE_ADAPTER_LIST(), id);
  return functor.Exists;
}
}
} // namespace vtkm::cont
