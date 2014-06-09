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
#ifndef vtk_m_cont__ArrayContainerControl_h
#define vtk_m_cont__ArrayContainerControl_h

#define VTKM_ARRAY_CONTAINER_CONTROL_ERROR      -2
#define VTKM_ARRAY_CONTAINER_CONTROL_UNDEFINED  -1
#define VTKM_ARRAY_CONTAINER_CONTROL_BASIC       1

#ifndef VTKM_ARRAY_CONTAINER_CONTROL
#define VTKM_ARRAY_CONTAINER_CONTROL VTKM_ARRAY_CONTAINER_CONTROL_BASIC
#endif

#include <boost/static_assert.hpp>

namespace vtkm {
namespace cont {

#ifdef VTKM_DOXYGEN_ONLY
/// \brief A tag specifying client memory allocation.
///
/// An ArrayContainerControl tag specifies how an ArrayHandle allocates and
/// frees memory. The tag ArrayContainerControlTag___ does not actually exist.
/// Rather, this documentation is provided to describe how array containers are
/// specified. Loading the vtkm/cont/ArrayContainerControl.h header will set a
/// default array container. You can specify the default array container by
/// first setting the VTKM_ARRAY_CONTAINER_CONTROL macro.  Currently it can only
/// be set to VTKM_ARRAY_CONTAINER_CONTROL_BASIC.
///
/// User code external to VTKm is free to make its own ArrayContainerControlTag.
/// This is a good way to get VTKm to read data directly in and out of arrays
/// from other libraries. However, care should be taken when creating an
/// ArrayContainerControl. One particular problem that is likely is a container
/// that "constructs" all the items in the array. If done incorrectly, then
/// memory of the array can be incorrectly bound to the wrong process. If you
/// do provide your own ArrayContainerControlTag, please be diligent in
/// comparing its performance to the ArrayContainerControlTagBasic.
///
/// To implement your own ArrayContainerControlTag, you first must create a tag
/// class (an empty struct) defining your tag (i.e. struct
/// ArrayContainerControlTagMyAlloc { };). Then provide a partial template
/// specialization of vtkm::cont::internal::ArrayContainerControl for your new
/// tag.
///
struct ArrayContainerControlTag___ {  };
#endif // VTKM_DOXYGEN_ONLY

namespace internal {

struct UndefinedArrayContainerControl {  };

namespace detail {

// This class should never be used. It is used as a placeholder for undefined
// ArrayContainerControl objects. If you get a compiler error involving this
// object, then it probably comes from trying to use an ArrayHandle with bad
// template arguments.
template<typename T>
struct UndefinedArrayPortal {
  BOOST_STATIC_ASSERT(sizeof(T) == -1);
};

} // namespace detail

/// This templated class must be partially specialized for each
/// ArrayContainerControlTag created, which will define the implementation for
/// that tag.
///
template<typename T, class ArrayContainerControlTag>
class ArrayContainerControl
#ifndef VTKM_DOXYGEN_ONLY
    : public vtkm::cont::internal::UndefinedArrayContainerControl
{
public:
  typedef vtkm::cont::internal::detail::UndefinedArrayPortal<T> PortalType;
  typedef vtkm::cont::internal::detail::UndefinedArrayPortal<T> PortalConstType;
};
#else //VTKM_DOXYGEN_ONLY
{
public:

  /// The type of each item in the array.
  ///
  typedef T ValueType;

  /// \brief The type of portal objects for the array.
  ///
  /// The actual portal object can take any form. This is a simple example of a
  /// portal to a C array.
  ///
  typedef ::vtkm::cont::internal::ArrayPortalFromIterators<ValueType*> PortalType;

  /// \brief The type of portal objects (const version) for the array.
  ///
  /// The actual portal object can take any form. This is a simple example of a
  /// portal to a C array.
  ///
  typedef ::vtkm::cont::internal::ArrayPortalFromIterators<const ValueType*> PortalConstType;

  /// Returns a portal to the array.
  ///
  VTKM_CONT_EXPORT
  PortalType GetPortal();

  /// Returns a portal to the array with immutable values.
  ///
  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const;

  /// Retuns the number of entries allocated in the array.
  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const;

  /// \brief Allocates an array large enough to hold the given number of values.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorControlOutOfMemory if the array cannot be allocated.
  ///
  VTKM_CONT_EXPORT
  void Allocate(vtkm::Id numberOfValues);

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id numberOfValues);

  /// \brief Frees any resources (i.e. memory) stored in this array.
  ///
  /// After calling this method GetNumberOfValues will return 0 and
  /// GetIteratorBegin and GetIteratorEnd will return the same iterator. The
  /// resources should also be released when the ArrayContainerControl class is
  /// destroyed.
  VTKM_CONT_EXPORT
  void ReleaseResources();
};
#endif // VTKM_DOXYGEN_ONLY

} // namespace internal

}
} // namespace vtkm::cont

// This is put at the bottom of the header so that the ArrayContainerControl
// template is declared before any implementations are called.

#if VTKM_ARRAY_CONTAINER_CONTROL == VTKM_ARRAY_CONTAINER_CONTROL_BASIC

#include <vtkm/cont/ArrayContainerControlBasic.h>
#define VTKM_DEFAULT_ARRAY_CONTAINER_CONTROL_TAG \
  ::vtkm::cont::ArrayContainerControlTagBasic

#elif VTKM_ARRAY_CONTAINER_CONTROL == VTKM_ARRAY_CONTAINER_CONTROL_ERROR

#include <vtkm/cont/internal/ArrayContainerControlError.h>
#define VTKM_DEFAULT_ARRAY_CONTAINER_CONTROL_TAG \
  ::vtkm::cont::internal::ArrayContainerControlTagError

#elif (VTKM_ARRAY_CONTAINER_CONTROL == VTKM_ARRAY_CONTAINER_CONTROL_UNDEFINED) || !defined(VTKM_ARRAY_CONTAINER_CONTROL)

#ifndef VTKM_DEFAULT_ARRAY_CONTAINER_CONTROL_TAG
#warning If array container for control is undefined, VTKM_DEFAULT_ARRAY_CONTAINER_CONTROL_TAG must be defined.
#endif

#endif

#endif //vtkm_cont__ArrayContainerControl_h
