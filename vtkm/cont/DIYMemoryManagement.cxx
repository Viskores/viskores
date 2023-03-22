//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DIYMemoryManagement.h>

#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#ifdef VTKM_ENABLE_GPU_MPI
#include <vtkm/cont/kokkos/DeviceAdapterKokkos.h>
#endif

namespace
{

thread_local vtkm::cont::DeviceAdapterId DIYCurrentDeviceAdaptor =
  vtkm::cont::DeviceAdapterTagSerial();

vtkm::cont::internal::DeviceAdapterMemoryManagerBase& GetMemoryManager(
  vtkm::cont::DeviceAdapterId device)
{
  return vtkm::cont::RuntimeDeviceInformation().GetMemoryManager(device);
}

vtkmdiy::MemoryManagement GetDIYMemoryManagement(vtkm::cont::DeviceAdapterId device)
{
  return vtkmdiy::MemoryManagement(
    [device](int, size_t n) {
      return static_cast<char*>(GetMemoryManager(device).AllocateRawPointer(n));
    },
    [device](const char* p) { GetMemoryManager(device).DeleteRawPointer(const_cast<char*>(p)); },
    [device](char* dest, const char* src, size_t count) {
      GetMemoryManager(device).CopyDeviceToDeviceRawPointer(src, dest, count);
    });
}

}

namespace vtkm
{
namespace cont
{

vtkm::cont::DeviceAdapterId GetDIYDeviceAdapter()
{
  return DIYCurrentDeviceAdaptor;
}

void DIYMasterExchange(vtkmdiy::Master& master, bool remote)
{
#ifdef VTKM_ENABLE_GPU_MPI
  try
  {
    DIYCurrentDeviceAdaptor = vtkm::cont::DeviceAdapterTagKokkos();
    master.exchange(remote, GetDIYMemoryManagement(vtkm::cont::DeviceAdapterTagKokkos()));
    DIYCurrentDeviceAdaptor = vtkm::cont::DeviceAdapterTagSerial();
  }
  catch (...)
  {
    DIYCurrentDeviceAdaptor = vtkm::cont::DeviceAdapterTagSerial();
    throw;
  }
#else
  DIYCurrentDeviceAdaptor = vtkm::cont::DeviceAdapterTagSerial();
  master.exchange(remote);
#endif
}

}
}
