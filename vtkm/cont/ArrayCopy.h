//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayCopy_h
#define vtk_m_cont_ArrayCopy_h

#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

// Compile-time check to make sure that an `ArrayHandle` passed to `ArrayCopy`
// can be passed to a `UnknownArrayHandle`. This function does nothing
// except provide a compile error that is easier to understand than if you
// let it go and error in `UnknownArrayHandle`. (Huh? I'm not using that.)
template <typename T>
inline void ArrayCopyValueTypeCheck()
{
  VTKM_STATIC_ASSERT_MSG(vtkm::HasVecTraits<T>::value,
                         "An `ArrayHandle` that has a special value type that is not supported "
                         "by the precompiled version of `ArrayCopy` has been used. If this array "
                         "must be deep copied, consider using `ArrayCopyDevice`. Look at the "
                         "compile error for the type assigned to template parameter `T` to "
                         "see the offending type.");
}

}

/// \brief Does a deep copy from one array to another array.
///
/// Given a source `ArrayHandle` and a destination `ArrayHandle`, this
/// function allocates the destination `ArrayHandle` to the correct size and
/// deeply copies all the values from the source to the destination.
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment, the runtime device tracker is used to try to find another
/// device.
///
/// This should work on some non-writable array handles as well, as long as
/// both \a source and \a destination are the same type.
///
/// This version of array copy uses a precompiled version of copy that is
/// efficient for most standard memory layouts. However, there are some
/// types of fancy `ArrayHandle` that cannot be handled directly, and
/// the fallback for these arrays can be slow. If you see a warning in
/// the log about an inefficient memory copy when extracting a component,
/// pay heed and look for a different way to copy the data (perhaps
/// using `ArrayCopyDevice`).
///
/// @{
///
inline void ArrayCopy(const vtkm::cont::UnknownArrayHandle& source,
                      vtkm::cont::UnknownArrayHandle& destination)
{
  destination.DeepCopyFrom(source);
}
inline void ArrayCopy(const vtkm::cont::UnknownArrayHandle& source,
                      const vtkm::cont::UnknownArrayHandle& destination)
{
  destination.DeepCopyFrom(source);
}

template <typename T, typename S>
void ArrayCopy(const vtkm::cont::UnknownArrayHandle& source,
               vtkm::cont::ArrayHandle<T, S>& destination)
{
  detail::ArrayCopyValueTypeCheck<T>();

  using DestType = vtkm::cont::ArrayHandle<T, S>;
  if (source.CanConvert<DestType>())
  {
    destination.DeepCopyFrom(source.AsArrayHandle<DestType>());
  }
  else
  {
    vtkm::cont::UnknownArrayHandle destWrapper(destination);
    vtkm::cont::ArrayCopy(source, destWrapper);
    // Destination array should not change, but just in case.
    destWrapper.AsArrayHandle(destination);
  }
}

template <typename T, typename S, typename DestArray>
void ArrayCopy(const vtkm::cont::ArrayHandle<T, S>& source, DestArray& destination)
{
  detail::ArrayCopyValueTypeCheck<T>();

  vtkm::cont::ArrayCopy(vtkm::cont::UnknownArrayHandle{ source }, destination);
}

// Special case of copying data when type is the same.
template <typename T, typename S>
void ArrayCopy(const vtkm::cont::ArrayHandle<T, S>& source,
               vtkm::cont::ArrayHandle<T, S>& destination)
{
  destination.DeepCopyFrom(source);
}

// Invalid const ArrayHandle in destination, which is not allowed because it will
// not work in all cases.
template <typename T, typename S>
void ArrayCopy(const vtkm::cont::UnknownArrayHandle&, const vtkm::cont::ArrayHandle<T, S>&)
{
  VTKM_STATIC_ASSERT_MSG(sizeof(T) == 0, "Copying to a constant ArrayHandle is not allowed.");
}

/// @}

/// \brief Copies from an unknown to a known array type.
///
/// Often times you have an array of an unknown type (likely from a data set),
/// and you need it to be of a particular type (or can make a reasonable but uncertain
/// assumption about it being a particular type). You really just want a shallow
/// copy (a reference in a concrete `ArrayHandle`) if that is possible.
///
/// `ArrayCopyShallowIfPossible` pulls an array of a specific type from an
/// `UnknownArrayHandle`. If the type is compatible, it will perform a shallow copy.
/// If it is not possible, a deep copy is performed to get it to the correct type.
///
template <typename T, typename S>
VTKM_CONT void ArrayCopyShallowIfPossible(const vtkm::cont::UnknownArrayHandle source,
                                          vtkm::cont::ArrayHandle<T, S>& destination)
{
  using DestType = vtkm::cont::ArrayHandle<T, S>;
  if (source.CanConvert<DestType>())
  {
    source.AsArrayHandle(destination);
  }
  else
  {
    vtkm::cont::UnknownArrayHandle destWrapper(destination);
    vtkm::cont::ArrayCopy(source, destWrapper);
    // Destination array should not change, but just in case.
    destWrapper.AsArrayHandle(destination);
  }
}

} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_ArrayCopy_h
