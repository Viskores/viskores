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
#ifndef vtk_m_cont__ArrayContainerControlBasic_h
#define vtk_m_cont__ArrayContainerControlBasic_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayContainerControl.h>
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/ErrorControlOutOfMemory.h>
#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

namespace vtkm {
namespace cont {

/// A tag for the basic implementation of an ArrayContainerControl object.
struct ArrayContainerControlTagBasic {  };

namespace internal {

/// A basic implementation of an ArrayContainerControl object.
///
/// \todo This container does \em not construct the values within the array.
/// Thus, it is important to not use this class with any type that will fail if
/// not constructed. These are things like basic types (int, float, etc.) and
/// the VTKm Tuple classes.  In the future it would be nice to have a compile
/// time check to enforce this.
///
template <typename ValueT>
class ArrayContainerControl<ValueT, vtkm::cont::ArrayContainerControlTagBasic>
{
public:
  typedef ValueT ValueType;
  typedef vtkm::cont::internal::ArrayPortalFromIterators<ValueType*> PortalType;
  typedef vtkm::cont::internal::ArrayPortalFromIterators<const ValueType*> PortalConstType;

private:
  /// The original design of this class provided an allocator as a template
  /// parameters. That messed things up, though, because other templated
  /// classes assume that the \c ArrayContainerControl has one template
  /// parameter. There are other ways to allow you to specify the allocator,
  /// but it is uncertain whether that would ever be useful. So, instead of
  /// jumping through hoops implementing them, just fix the allocator for now.
  ///
  typedef std::allocator<ValueType> AllocatorType;

public:

  ArrayContainerControl() : Array(NULL), NumberOfValues(0), AllocatedSize(0) { }

  ~ArrayContainerControl()
  {
    this->ReleaseResources();
  }

  void ReleaseResources()
  {
    if (this->NumberOfValues > 0)
    {
      VTKM_ASSERT_CONT(this->Array != NULL);
      AllocatorType allocator;
      allocator.deallocate(this->Array, this->AllocatedSize);
      this->Array = NULL;
      this->NumberOfValues = 0;
      this->AllocatedSize = 0;
    }
    else
    {
      VTKM_ASSERT_CONT(this->Array == NULL);
    }
  }

  void Allocate(vtkm::Id numberOfValues)
  {
    if (numberOfValues <= this->AllocatedSize)
    {
      this->NumberOfValues = numberOfValues;
      return;
    }

    this->ReleaseResources();
    try
    {
      if (numberOfValues > 0)
      {
        AllocatorType allocator;
        this->Array = allocator.allocate(numberOfValues);
        this->AllocatedSize  = numberOfValues;
        this->NumberOfValues = numberOfValues;
      }
      else
      {
        // ReleaseResources should have already set AllocatedSize to 0.
        VTKM_ASSERT_CONT(this->AllocatedSize == 0);
      }
    }
    catch (std::bad_alloc err)
    {
      // Make sureour state is OK.
      this->Array = NULL;
      this->NumberOfValues = 0;
      this->AllocatedSize = 0;
      throw vtkm::cont::ErrorControlOutOfMemory(
        "Could not allocate basic control array.");
    }
  }

  vtkm::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  void Shrink(vtkm::Id numberOfValues)
  {
    if (numberOfValues > this->GetNumberOfValues())
    {
      throw vtkm::cont::ErrorControlBadValue(
        "Shrink method cannot be used to grow array.");
    }

    this->NumberOfValues = numberOfValues;
  }

  PortalType GetPortal()
  {
    return PortalType(this->Array, this->Array + this->NumberOfValues);
  }

  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Array, this->Array + this->NumberOfValues);
  }

  /// \brief Take the reference away from this object.
  ///
  /// This method returns the pointer to the array held by this array. It then
  /// clears the internal array pointer to NULL, thereby ensuring that the
  /// ArrayContainerControl will never deallocate the array. This is
  /// helpful for taking a reference for an array created internally by VTKm and
  /// not having to keep a VTKm object around. Obviously the caller becomes
  /// responsible for destroying the memory.
  ///
  ValueType *StealArray()
  {
    ValueType *saveArray =  this->Array;
    this->Array = NULL;
    this->NumberOfValues = 0;
    this->AllocatedSize = 0;
    return saveArray;
  }

private:
  // Not implemented.
  ArrayContainerControl(const ArrayContainerControl<ValueType, ArrayContainerControlTagBasic> &src);
  void operator=(const ArrayContainerControl<ValueType, ArrayContainerControlTagBasic> &src);

  ValueType *Array;
  vtkm::Id NumberOfValues;
  vtkm::Id AllocatedSize;
};

} // namespace internal

}
} // namespace vtkm::cont

#endif //vtk_m_cont__ArrayContainerControlBasic_h
