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
#ifndef vtk_m_cont_ArrayContainerControlImplicit
#define vtk_m_cont_ArrayContainerControlImplicit

#include <vtkm/Types.h>

#include <vtkm/cont/ArrayContainerControl.h>
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/internal/ArrayTransfer.h>

namespace vtkm {
namespace cont {

/// \brief An implementation for read-only implicit arrays.
///
/// It is sometimes the case that you want VTKm to operate on an array of
/// implicit values. That is, rather than store the data in an actual array, it
/// is gerenated on the fly by a function. This is handled in VTKm by creating
/// an ArrayHandle in VTKm with an ArrayContainerControlTagImplicit type of
/// ArrayContainerControl. This tag itself is templated to specify an
/// ArrayPortal that generates the desired values. An ArrayHandle created with
/// this tag will raise an error on any operation that tries to modify it.
///
/// \todo The ArrayHandle currently copies the array in cases where the control
/// and environment do not share memory. This is wasteful and should be fixed.
///
template<class ArrayPortalType>
struct ArrayContainerControlTagImplicit
{
  typedef ArrayPortalType PortalType;
};

namespace internal {

template<class ArrayPortalType>
class ArrayContainerControl<
    typename ArrayPortalType::ValueType,
    ArrayContainerControlTagImplicit<ArrayPortalType> >
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
    // ArrayPortal, not the container.
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit container does not store array portal.  "
          "Perhaps you did not set the ArrayPortal when "
          "constructing the ArrayHandle.");
  }
  vtkm::Id GetNumberOfValues() const
  {
    // This does not work because the ArrayHandle holds the constant
    // ArrayPortal, not the container.
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit container does not store array portal.  "
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

//// If you are using this specalization of ArrayContainerControl, it means the
//// type of an ArrayHandle does not match the type of the array portal for the
//// array portal in an implicit array. Currently this use is invalid, but there
//// could be a case for implementing casting where appropriate.
//template<typename T, typename ArrayPortalType>
//class ArrayContainerControl<T, ArrayContainerControlTagImplicit<ArrayPortalType> >
//{
//public:
//  // This is meant to be invalid because this class should not actually be used.
//  struct PortalType
//  {
//    typedef void *ValueType;
//    typedef void *IteratorType;
//  };
//  // This is meant to be invalid because this class should not actually be used.
//  struct PortalConstType
//  {
//    typedef void *ValueType;
//    typedef void *IteratorType;
//  };
//};

template<typename T, class ArrayPortalType, class DeviceAdapterTag>
class ArrayTransfer<
    T, ArrayContainerControlTagImplicit<ArrayPortalType>, DeviceAdapterTag>
{
private:
  typedef ArrayContainerControlTagImplicit<ArrayPortalType>
      ArrayContainerControlTag;
  typedef vtkm::cont::internal::ArrayContainerControl<T,ArrayContainerControlTag>
      ContainerType;

public:
  typedef T ValueType;

  typedef typename ContainerType::PortalType PortalControl;
  typedef typename ContainerType::PortalConstType PortalConstControl;
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

  VTKM_CONT_EXPORT void LoadDataForInput(const ContainerType &controlArray)
  {
    this->LoadDataForInput(controlArray.GetPortalConst());
  }

  VTKM_CONT_EXPORT
  void LoadDataForInPlace(ContainerType &vtkmNotUsed(controlArray))
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit arrays cannot be used for output or in place.");
  }

  VTKM_CONT_EXPORT void AllocateArrayForOutput(
      ContainerType &vtkmNotUsed(controlArray),
      vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Implicit arrays cannot be used for output.");
  }
  VTKM_CONT_EXPORT void RetrieveOutputData(
      ContainerType &vtkmNotUsed(controlArray)) const
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

#endif //vtk_m_cont_ArrayContainerControlImplicit
