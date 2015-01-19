//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_StorageImplicit
#define vtk_m_cont_StorageImplicit

#include <vtkm/Types.h>

#include <vtkm/cont/Assert.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/cont/internal/ArrayTransfer.h>

namespace vtkm {
namespace cont {

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
template<class ArrayPortalType>
struct StorageTagImplicit
{
  typedef ArrayPortalType PortalType;
};

namespace internal {

template<class ArrayPortalType>
class Storage<
    typename ArrayPortalType::ValueType,
    StorageTagImplicit<ArrayPortalType> >
{
public:
  typedef typename ArrayPortalType::ValueType ValueType;
  typedef ArrayPortalType PortalConstType;

  // This is meant to be invalid. Because implicit arrays are read only, you
  // should only be able to use the const version.
  struct PortalType
  {
    typedef void *ValueType;
    typedef void *IteratorType;
  };

  // All these methods do nothing but raise errors.
  PortalType GetPortal()
  {
    throw vtkm::cont::ErrorControlBadValue("Implicit arrays are read-only.");
  }
  PortalConstType GetPortalConst() const
  {
    // This does not work because the ArrayHandle holds the constant
    // ArrayPortal, not the storage.
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit storage does not store array portal.  "
          "Perhaps you did not set the ArrayPortal when "
          "constructing the ArrayHandle.");
  }
  vtkm::Id GetNumberOfValues() const
  {
    // This does not work because the ArrayHandle holds the constant
    // ArrayPortal, not the Storage.
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit storage does not store array portal.  "
          "Perhaps you did not set the ArrayPortal when "
          "constructing the ArrayHandle.");
  }
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorControlBadValue("Implicit arrays are read-only.");
  }
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorControlBadValue("Implicit arrays are read-only.");
  }
  void ReleaseResources()
  {
    throw vtkm::cont::ErrorControlBadValue("Implicit arrays are read-only.");
  }
};

template<typename T, class ArrayPortalType, class DeviceAdapterTag>
class ArrayTransfer<T, StorageTagImplicit<ArrayPortalType>, DeviceAdapterTag>
{
private:
  typedef StorageTagImplicit<ArrayPortalType> StorageTag;
  typedef vtkm::cont::internal::Storage<T,StorageTag> StorageType;

public:
  typedef T ValueType;

  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;
  typedef PortalControl PortalExecution;
  typedef PortalConstControl PortalConstExecution;

  ArrayTransfer() : PortalValid(false) {  }

  VTKM_CONT_EXPORT vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT_CONT(this->PortalValid);
    return this->Portal.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT void LoadDataForInput(const PortalConstControl& portal)
  {
    this->Portal = portal;
    this->PortalValid = true;
  }

  VTKM_CONT_EXPORT void LoadDataForInput(const StorageType& vtkmNotUsed(controlArray) )
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit arrays have no storage, you need to load from a portal");
  }

  VTKM_CONT_EXPORT
  void LoadDataForInPlace(StorageType &vtkmNotUsed(controlArray))
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit arrays cannot be used for output or in place.");
  }

  VTKM_CONT_EXPORT void AllocateArrayForOutput(
      StorageType &vtkmNotUsed(controlArray),
      vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit arrays cannot be used for output.");
  }
  VTKM_CONT_EXPORT void RetrieveOutputData(
      StorageType &vtkmNotUsed(controlArray)) const
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit arrays cannot be used for output.");
  }

  VTKM_CONT_EXPORT void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorControlBadValue("Implicit arrays cannot be resized.");
  }

  VTKM_CONT_EXPORT PortalExecution GetPortalExecution()
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit arrays are read-only.  (Get the const portal.)");
  }
  VTKM_CONT_EXPORT PortalConstExecution GetPortalConstExecution() const
  {
    VTKM_ASSERT_CONT(this->PortalValid);
    return this->Portal;
  }

  VTKM_CONT_EXPORT void ReleaseResources() {  }

private:
  PortalConstExecution Portal;
  bool PortalValid;
};

} // namespace internal

}
} // namespace vtkm::cont

#endif //vtk_m_cont_StorageImplicit
