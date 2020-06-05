//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_CastInvalidValue_h
#define vtk_m_cont_internal_CastInvalidValue_h

#include <vtkm/Math.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// \brief Convert an invalid value to something type-appropriate.
///
/// There are algorithms in VTK-m that require a placeholder for invalid values in an array
/// or field. For example, when probing something, a probe location outside of the source data
/// has to be set to something.
///
/// Often we want to set this to something like NaN to make it clear that this is invalid.
/// However, integer types cannot represent these non-finite numbers.
///
/// For convenience, it is easiest to allow the user to specify the invalid value as a
/// vtkm::Float64 and use this function to convert it to something type-appropriate.
///
template <typename T>
T CastInvalidValue(vtkm::Float64 invalidValue)
{
  using ComponentType = typename vtkm::VecTraits<T>::BaseComponentType;

  if (std::is_same<vtkm::TypeTraitsIntegerTag, typename vtkm::TypeTraits<T>::NumericTag>::value)
  {
    // Casting to integer types
    if (vtkm::IsFinite(invalidValue))
    {
      return T(static_cast<ComponentType>(invalidValue));
    }
    else if (vtkm::IsInf(invalidValue) && (invalidValue > 0))
    {
      return T(std::numeric_limits<ComponentType>::max());
    }
    else
    {
      return T(std::numeric_limits<ComponentType>::min());
    }
  }
  else
  {
    // Not an integer type. Assume can be directly cast
    return T(static_cast<ComponentType>(invalidValue));
  }
}
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_CastInvalidValue_h
