//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/kokkos/internal/DeviceAdapterAlgorithmKokkos.h>

#include <string>

namespace
{

constexpr static vtkm::Id ErrorMessageMaxLength = 1024;

using ErrorMessageView = vtkm::cont::kokkos::internal::KokkosViewExec<char>;

ErrorMessageView GetErrorMessageViewInstance()
{
  static thread_local ErrorMessageView local(std::string("ErrorMessageViewInstance"),
                                             ErrorMessageMaxLength);
  return local;
}

} // anonymous namespace

namespace vtkm
{
namespace cont
{

vtkm::exec::internal::ErrorMessageBuffer
DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagKokkos>::GetErrorMessageBufferInstance()
{
  return vtkm::exec::internal::ErrorMessageBuffer(GetErrorMessageViewInstance().data(),
                                                  ErrorMessageMaxLength);
}

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagKokkos>::CheckForErrors()
{
  static thread_local char hostBuffer[ErrorMessageMaxLength] = "";

  auto deviceView = GetErrorMessageViewInstance();

  if (Kokkos::SpaceAccessibility<Kokkos::HostSpace, decltype(deviceView)::memory_space>::accessible)
  {
    vtkm::cont::kokkos::internal::GetExecutionSpaceInstance().fence();
    if (deviceView(0) != '\0')
    {
      auto excep = vtkm::cont::ErrorExecution(deviceView.data());
      deviceView(0) = '\0'; // clear
      vtkm::cont::kokkos::internal::GetExecutionSpaceInstance().fence();
      throw excep;
    }
  }
  else
  {
    vtkm::cont::kokkos::internal::KokkosViewCont<char> hostView(hostBuffer, ErrorMessageMaxLength);
    Kokkos::deep_copy(
      vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), hostView, deviceView);
    vtkm::cont::kokkos::internal::GetExecutionSpaceInstance().fence();

    if (hostView(0) != '\0')
    {
      auto excep = vtkm::cont::ErrorExecution(hostView.data());
      hostView(0) = '\0'; // clear
      Kokkos::deep_copy(
        vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), deviceView, hostView);
      throw excep;
    }
  }
}

}
} // vtkm::cont
