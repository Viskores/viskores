//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_kokkos_internal_ArrayManagerExecutionKokkos_h
#define vtk_m_cont_kokkos_internal_ArrayManagerExecutionKokkos_h

#include <vtkm/cont/internal/ArrayManagerExecution.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/cont/kokkos/internal/DeviceAdapterTagKokkos.h>
#include <vtkm/cont/kokkos/internal/ViewTypes.h>

#include <vtkm/internal/ArrayPortalBasic.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <Kokkos_Core.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <limits>

// These must be placed in the vtkm::cont::internal namespace so that
// the template can be found.

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename T, class StorageTag>
class ArrayManagerExecution<T, StorageTag, vtkm::cont::DeviceAdapterTagKokkos>
{
public:
  using ValueType = T;
  using PortalType = vtkm::internal::ArrayPortalBasicWrite<T>;
  using PortalConstType = vtkm::internal::ArrayPortalBasicRead<T>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

  VTKM_CONT
  ArrayManagerExecution(StorageType* storage)
    : Storage(storage)
  {
  }

  VTKM_CONT
  ~ArrayManagerExecution() { this->ReleaseResources(); }

  /// Returns the size of the array.
  ///
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  VTKM_CONT
  PortalConstType PrepareForInput(bool updateData, vtkm::cont::Token&)
  {
    if (updateData)
    {
      this->CopyToExecution();
    }

    return PortalConstType(this->DeviceArray, this->DeviceArrayLength);
  }

  VTKM_CONT
  PortalType PrepareForInPlace(bool updateData, vtkm::cont::Token&)
  {
    if (updateData)
    {
      this->CopyToExecution();
    }

    return PortalType(this->DeviceArray, this->DeviceArrayLength);
  }

  VTKM_CONT
  PortalType PrepareForOutput(vtkm::Id numberOfValues, vtkm::cont::Token&)
  {
    if (numberOfValues > this->DeviceArrayLength)
    {
      this->ReallocDeviceArray(numberOfValues);
    }
    this->DeviceArrayLength = numberOfValues;
    return PortalType(this->DeviceArray, this->DeviceArrayLength);
  }

  /// Allocates enough space in \c storage and copies the data in the
  /// device vector into it.
  ///
  VTKM_CONT
  void RetrieveOutputData(StorageType* storage) const
  {
    VTKM_LOG_F(vtkm::cont::LogLevel::MemTransfer,
               "Copying Kokkos dev --> host: %s",
               vtkm::cont::GetSizeString(this->DeviceArrayLength).c_str());

    vtkm::cont::kokkos::internal::KokkosViewConstExec<T> deviceView(
      this->DeviceArray, static_cast<std::size_t>(this->DeviceArrayLength));
    auto hostView = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace{}, deviceView);

    storage->Allocate(this->DeviceArrayLength);
    std::copy_n(hostView.data(),
                this->DeviceArrayLength,
                vtkm::cont::ArrayPortalToIteratorBegin(storage->GetPortal()));
  }

  /// Resizes the device vector.
  ///
  VTKM_CONT void Shrink(vtkm::Id numberOfValues)
  {
    // The operation will succeed even if this assertion fails, but this
    // is still supposed to be a precondition to Shrink.
    VTKM_ASSERT(numberOfValues <= this->DeviceArrayLength);
    this->ReallocDeviceArray(numberOfValues);
    this->DeviceArrayLength = numberOfValues;
  }

  /// Frees all memory.
  ///
  VTKM_CONT void ReleaseResources()
  {
    Kokkos::kokkos_free(this->DeviceArray);
    this->DeviceArray = nullptr;
    this->DeviceArrayLength = 0;
  }

private:
  ArrayManagerExecution(ArrayManagerExecution&) = delete;
  void operator=(ArrayManagerExecution&) = delete;

  void ReallocDeviceArray(vtkm::Id numberOfValues)
  {
    size_t size = static_cast<std::size_t>(numberOfValues) * sizeof(T);
    try
    {
      if (!this->DeviceArray)
      {
        this->DeviceArray = static_cast<T*>(Kokkos::kokkos_malloc(size));
      }
      else
      {
        this->DeviceArray = static_cast<T*>(Kokkos::kokkos_realloc(this->DeviceArray, size));
      }
    }
    catch (...)
    {
      std::ostringstream err;
      err << "Failed to allocate " << size << " bytes on Kokkos device";
      throw vtkm::cont::ErrorBadAllocation(err.str());
    }
  }

  VTKM_CONT
  static void CopyToExecutionImpl(
    ArrayManagerExecution<T, vtkm::cont::StorageTagBasic, vtkm::cont::DeviceAdapterTagKokkos>* self)
  {
    self->ReallocDeviceArray(self->Storage->GetNumberOfValues());
    self->DeviceArrayLength = self->Storage->GetNumberOfValues();

    vtkm::cont::kokkos::internal::KokkosViewConstCont<T> hostView(
      self->Storage->GetArray(), self->Storage->GetNumberOfValues());
    vtkm::cont::kokkos::internal::KokkosViewExec<T> deviceView(self->DeviceArray,
                                                               self->DeviceArrayLength);

    Kokkos::deep_copy(deviceView, hostView);
  }

  template <typename S>
  VTKM_CONT static void CopyToExecutionImpl(
    ArrayManagerExecution<T, S, vtkm::cont::DeviceAdapterTagKokkos>* self)
  {
    std::vector<T> buffer(static_cast<std::size_t>(self->Storage->GetNumberOfValues()));
    std::copy(vtkm::cont::ArrayPortalToIteratorBegin(self->Storage->GetPortalConst()),
              vtkm::cont::ArrayPortalToIteratorEnd(self->Storage->GetPortalConst()),
              buffer.begin());

    self->ReallocDeviceArray(self->Storage->GetNumberOfValues());
    self->DeviceArrayLength = self->Storage->GetNumberOfValues();

    vtkm::cont::kokkos::internal::KokkosViewConstCont<T> hostView(buffer.data(), buffer.size());
    vtkm::cont::kokkos::internal::KokkosViewExec<T> deviceView(
      self->DeviceArray, static_cast<std::size_t>(self->DeviceArrayLength));

    Kokkos::deep_copy(deviceView, hostView);
  }

  VTKM_CONT
  void CopyToExecution() { CopyToExecutionImpl(this); }

  StorageType* Storage;

  T* DeviceArray = nullptr;
  vtkm::Id DeviceArrayLength = 0;
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_kokkos_internal_ArrayManagerExecutionKokkos_h
