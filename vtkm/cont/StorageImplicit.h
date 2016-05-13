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
//  Copyright 2014 Los Alamos National Security.
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

  VTKM_CONT_EXPORT
  Storage(const PortalConstType &portal = PortalConstType())
    : Portal(portal) {  }

  // All these methods do nothing but raise errors.
  VTKM_CONT_EXPORT
  PortalType GetPortal()
  {
    throw vtkm::cont::ErrorControlBadValue("Implicit arrays are read-only.");
  }
  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const
  {
    return this->Portal;
  }
  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->Portal.GetNumberOfValues();
  }
  VTKM_CONT_EXPORT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorControlBadValue("Implicit arrays are read-only.");
  }
  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorControlBadValue("Implicit arrays are read-only.");
  }
  VTKM_CONT_EXPORT
  void ReleaseResources()
  {
  }

private:
  PortalConstType Portal;
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

  VTKM_CONT_EXPORT
  ArrayTransfer(StorageType *storage) : Storage(storage) {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->Storage->GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return this->Storage->GetPortalConst();
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit arrays cannot be used for output or in place.");
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit arrays cannot be used for output.");
  }
  VTKM_CONT_EXPORT
  void RetrieveOutputData(StorageType *vtkmNotUsed(controlArray)) const
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit arrays cannot be used for output.");
  }

  template <class IteratorTypeControl>
  VTKM_CONT_EXPORT void CopyInto(IteratorTypeControl dest) const
  {
    typedef typename StorageType::PortalConstType::IteratorType IteratorType;
    IteratorType beginIterator = 
                    this->Storage->GetPortalConst().GetIteratorBegin();

    std::copy(beginIterator, 
              beginIterator + this->Storage->GetNumberOfValues(), dest);
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorControlBadValue("Implicit arrays cannot be resized.");
  }

  VTKM_CONT_EXPORT
  void ReleaseResources() {  }

private:
  StorageType *Storage;
};

} // namespace internal

}
} // namespace vtkm::cont

#endif //vtk_m_cont_StorageImplicit
