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
#include <vtkm/cont/ErrorBadDevice.h>

//Bring in each device adapters runtime class
#include <vtkm/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterRuntimeDetectorOpenMP.h>
#include <vtkm/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterRuntimeDetectorTBB.h>

#include <cctype> //for tolower

namespace
{
class DeviceAdapterMemoryManagerInvalid
  : public vtkm::cont::internal::DeviceAdapterMemoryManagerBase
{
public:
  VTKM_CONT virtual ~DeviceAdapterMemoryManagerInvalid() override {}

  /// Allocates a buffer of the specified size in bytes and returns a BufferInfo object
  /// containing information about it.
  VTKM_CONT virtual std::shared_ptr<vtkm::cont::internal::BufferInfo> Allocate(
    vtkm::BufferSizeType) override
  {
    throw vtkm::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  /// Manages the provided array. Returns a `BufferInfo` object that contains the data.
  /// The deleter in the `shared_ptr` is expected to correctly free the data.
  VTKM_CONT virtual std::shared_ptr<vtkm::cont::internal::BufferInfo> ManageArray(
    std::shared_ptr<vtkm::UInt8>,
    vtkm::BufferSizeType) override
  {
    throw vtkm::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  /// Reallocates the provided buffer to a new size. The passed in `BufferInfo` should be
  /// modified to reflect the changes.
  VTKM_CONT virtual void Reallocate(std::shared_ptr<vtkm::cont::internal::BufferInfo>,
                                    vtkm::BufferSizeType) override
  {
    throw vtkm::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  /// Copies data from the host buffer provided onto the device and returns a buffer info
  /// object holding the pointer for the device.
  VTKM_CONT virtual std::shared_ptr<vtkm::cont::internal::BufferInfo> CopyHostToDevice(
    std::shared_ptr<vtkm::cont::internal::BufferInfoHost>) override
  {
    throw vtkm::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  /// Copies data from the device buffer provided to the host. The passed in `BufferInfo` object
  /// was created by a previous call to this object.
  VTKM_CONT virtual std::shared_ptr<vtkm::cont::internal::BufferInfoHost> CopyDeviceToHost(
    std::shared_ptr<vtkm::cont::internal::BufferInfo>) override
  {
    throw vtkm::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }

  /// Copies data from one device buffer to another device buffer. The passed in `BufferInfo` object
  /// was created by a previous call to this object.
  VTKM_CONT virtual std::shared_ptr<vtkm::cont::internal::BufferInfo> CopyDeviceToDevice(
    std::shared_ptr<vtkm::cont::internal::BufferInfo>) override
  {
    throw vtkm::cont::ErrorBadDevice("Tried to manage memory on an invalid device.");
  }
};

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

struct VTKM_NEVER_EXPORT InitializeDeviceMemoryManagers
{
  std::unique_ptr<vtkm::cont::internal::DeviceAdapterMemoryManagerBase>* Managers;

  VTKM_CONT
  InitializeDeviceMemoryManagers(
    std::unique_ptr<vtkm::cont::internal::DeviceAdapterMemoryManagerBase>* managers)
    : Managers(managers)
  {
  }

  template <typename Device>
  VTKM_CONT void CreateManager(Device device, std::true_type)
  {
    auto id = device.GetValue();

    if (id > 0 && id < VTKM_MAX_DEVICE_ADAPTER_ID)
    {
      auto name = vtkm::cont::DeviceAdapterTraits<Device>::GetName();
      this->Managers[id].reset(new vtkm::cont::internal::DeviceAdapterMemoryManager<Device>);
    }
  }

  template <typename Device>
  VTKM_CONT void CreateManager(Device, std::false_type)
  {
    // No manager for invalid devices.
  }

  template <typename Device>
  VTKM_CONT void operator()(Device device)
  {
    this->CreateManager(device, std::integral_constant<bool, device.IsEnabled>{});
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

class RuntimeDeviceNames
{
public:
  static const vtkm::cont::DeviceAdapterNameType& GetDeviceName(vtkm::Int8 id)
  {
    return Instance().DeviceNames[id];
  }

  static const vtkm::cont::DeviceAdapterNameType& GetLowerCaseDeviceName(vtkm::Int8 id)
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

  vtkm::cont::DeviceAdapterNameType DeviceNames[VTKM_MAX_DEVICE_ADAPTER_ID];
  vtkm::cont::DeviceAdapterNameType LowerCaseDeviceNames[VTKM_MAX_DEVICE_ADAPTER_ID];
};

class RuntimeDeviceMemoryManagers
{
public:
  static vtkm::cont::internal::DeviceAdapterMemoryManagerBase& GetDeviceMemoryManager(
    vtkm::cont::DeviceAdapterId device)
  {
    const auto id = device.GetValue();

    if (device.IsValueValid())
    {
      auto&& manager = Instance().DeviceMemoryManagers[id];
      if (manager)
      {
        return *manager.get();
      }
      else
      {
        return Instance().InvalidManager;
      }
    }
    else
    {
      return Instance().InvalidManager;
    }
  }

private:
  static RuntimeDeviceMemoryManagers& Instance()
  {
    static RuntimeDeviceMemoryManagers instance;
    return instance;
  }

  RuntimeDeviceMemoryManagers()
  {
    InitializeDeviceMemoryManagers functor(this->DeviceMemoryManagers);
    vtkm::ListForEach(functor, VTKM_DEFAULT_DEVICE_ADAPTER_LIST());
  }

  friend struct InitializeDeviceMemoryManagers;

  std::unique_ptr<vtkm::cont::internal::DeviceAdapterMemoryManagerBase>
    DeviceMemoryManagers[VTKM_MAX_DEVICE_ADAPTER_ID];
  DeviceAdapterMemoryManagerInvalid InvalidManager;
};
}

namespace vtkm
{
namespace cont
{
namespace detail
{
}

VTKM_CONT
DeviceAdapterNameType RuntimeDeviceInformation::GetName(DeviceAdapterId device) const
{
  const auto id = device.GetValue();

  if (device.IsValueValid())
  {
    return RuntimeDeviceNames::GetDeviceName(id);
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
  return RuntimeDeviceNames::GetDeviceName(0);
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
    if (name == RuntimeDeviceNames::GetLowerCaseDeviceName(id))
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

VTKM_CONT vtkm::cont::internal::DeviceAdapterMemoryManagerBase&
RuntimeDeviceInformation::GetMemoryManager(DeviceAdapterId device) const
{
  if (device.IsValueValid())
  {
    return RuntimeDeviceMemoryManagers::GetDeviceMemoryManager(device);
  }
  else
  {
    throw vtkm::cont::ErrorBadValue(
      "Attempted to get a DeviceAdapterMemoryManager for an invalid device.");
  }
}
}
} // namespace vtkm::cont
