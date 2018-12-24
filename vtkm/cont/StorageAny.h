//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_StorageAny_h
#define vtk_m_cont_StorageAny_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageVirtual.h>

namespace vtkm
{
namespace cont
{
template <typename T, typename S>
class VTKM_ALWAYS_EXPORT StorageAny final : public vtkm::cont::StorageVirtual
{
public:
  VTKM_CONT
  explicit StorageAny(const vtkm::cont::ArrayHandle<T, S>& ah);

  explicit StorageAny(vtkm::cont::ArrayHandle<T, S>&& ah) noexcept;

  VTKM_CONT
  ~StorageAny() = default;

  const vtkm::cont::ArrayHandle<T, S>& GetHandle() const { return this->Handle; }

  vtkm::Id GetNumberOfValues() const { return this->Handle.GetNumberOfValues(); }

  void ReleaseResourcesExecution();
  void ReleaseResources();

private:
  std::unique_ptr<StorageVirtual> MakeNewInstance() const
  {
    return std::unique_ptr<StorageVirtual>(new StorageAny<T, S>{ vtkm::cont::ArrayHandle<T, S>{} });
  }


  void ControlPortalForInput(vtkm::cont::internal::TransferInfoArray& payload) const;
  void ControlPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload);

  void TransferPortalForInput(vtkm::cont::internal::TransferInfoArray& payload,
                              vtkm::cont::DeviceAdapterId devId) const;

  void TransferPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload,
                               OutputMode mode,
                               vtkm::Id numberOfValues,
                               vtkm::cont::DeviceAdapterId devId);

  vtkm::cont::ArrayHandle<T, S> Handle;
};
}
}

#endif //vtk_m_cont_StorageAny_h
