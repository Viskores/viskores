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
#ifndef vtk_m_cont_StorageImplicit
#define vtk_m_cont_StorageImplicit

#include <vtkm/Types.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/cont/internal/ArrayTransfer.h>

namespace vtkm
{
namespace cont
{

/// \brief An implementation for read-only implicit arrays.
///
/// It is sometimes the case that you want VTK-m to operate on an array of
/// implicit values. That is, rather than store the data in an actual array, it
/// is gerenated on the fly by a function. This is handled in VTK-m by creating
/// an ArrayHandle in VTK-m with a StorageTagImplicit type of \c Storage. This
/// tag itself is templated to specify an ArrayPortal that generates the
/// desired values. An ArrayHandle created with this tag will raise an error on
/// any operation that tries to modify it.
///
template <class ArrayPortalType>
struct VTKM_ALWAYS_EXPORT StorageTagImplicit
{
  using PortalType = ArrayPortalType;
};

namespace internal
{

template <class ArrayPortalType>
class Storage<typename ArrayPortalType::ValueType, StorageTagImplicit<ArrayPortalType>>
{
public:
  using ValueType = typename ArrayPortalType::ValueType;
  using PortalConstType = ArrayPortalType;

  // This is meant to be invalid. Because implicit arrays are read only, you
  // should only be able to use the const version.
  struct PortalType
  {
    using ValueType = void*;
    using IteratorType = void*;
  };

  VTKM_CONT
  Storage(const PortalConstType& portal = PortalConstType())
    : Portal(portal)
    , NumberOfValues(portal.GetNumberOfValues())
  {
  }

  // All these methods do nothing but raise errors.
  VTKM_CONT
  PortalType GetPortal() { throw vtkm::cont::ErrorBadValue("Implicit arrays are read-only."); }
  VTKM_CONT
  PortalConstType GetPortalConst() const { return this->Portal; }
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }
  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(numberOfValues <= this->Portal.GetNumberOfValues());
    this->NumberOfValues = numberOfValues;
  }
  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(numberOfValues <= this->Portal.GetNumberOfValues());
    this->NumberOfValues = numberOfValues;
  }
  VTKM_CONT
  void ReleaseResources() {}

private:
  PortalConstType Portal;
  vtkm::Id NumberOfValues;
};

template <typename T, class ArrayPortalType, class DeviceAdapterTag>
class ArrayTransfer<T, StorageTagImplicit<ArrayPortalType>, DeviceAdapterTag>
{
private:
  using StorageTag = StorageTagImplicit<ArrayPortalType>;
  using StorageType = vtkm::cont::internal::Storage<T, StorageTag>;

public:
  using ValueType = T;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  using PortalExecution = PortalControl;
  using PortalConstExecution = PortalConstControl;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Storage(storage)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return this->Storage->GetPortalConst();
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    throw vtkm::cont::ErrorBadValue("Implicit arrays cannot be used for output or in place.");
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadValue("Implicit arrays cannot be used for output.");
  }
  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(controlArray)) const
  {
    throw vtkm::cont::ErrorBadValue("Implicit arrays cannot be used for output.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadValue("Implicit arrays cannot be resized.");
  }

  VTKM_CONT
  void ReleaseResources() {}

private:
  StorageType* Storage;
};

} // namespace internal
}
} // namespace vtkm::cont

#endif //vtk_m_cont_StorageImplicit
