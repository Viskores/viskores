//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_count_ArrayHandleRandomUniformReal_h
#define vtk_m_count_ArrayHandleRandomUniformReal_h

#include <vtkm/cont/ArrayHandleRandomUniformBits.h>
#include <vtkm/cont/ArrayHandleTransform.h>

namespace vtkm
{
namespace cont
{

namespace detail
{
template <typename Real>
struct CanonicalFunctor;

template <>
struct CanonicalFunctor<vtkm::Float64>
{
  /// \brief VTKm's equivalent of std::generate_canonical, turning a random bit source into
  /// random real number in the range of [0, 1).
  // We take 53 bits (number of bits in mantissa in a double) from the 64 bits random source
  // and divide it by (1 << 53).
  static constexpr vtkm::Float64 DIVISOR = static_cast<vtkm::Float64>(vtkm::UInt64{ 1 } << 53);
  static constexpr vtkm::UInt64 MASK = (vtkm::UInt64{ 1 } << 53) - vtkm::UInt64{ 1 };

  VTKM_EXEC_CONT
  vtkm::Float64 operator()(vtkm::UInt64 bits) const { return (bits & MASK) / DIVISOR; }
};

template <>
struct CanonicalFunctor<vtkm::Float32>
{
  // We take 24 bits (number of bits in mantissa in a double) from the 64 bits random source
  // and divide it by (1 << 24).
  static constexpr vtkm::Float32 DIVISOR = static_cast<vtkm::Float32>(vtkm::UInt32{ 1 } << 24);
  static constexpr vtkm::UInt32 MASK = (vtkm::UInt32{ 1 } << 24) - vtkm::UInt32{ 1 };

  VTKM_EXEC_CONT
  vtkm::Float32 operator()(vtkm::UInt64 bits) const { return (bits & MASK) / DIVISOR; }
};
} // detail

template <typename Real = vtkm::Float64>
class VTKM_ALWAYS_EXPORT ArrayHandleRandomUniformReal
  : public vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleRandomUniformBits,
                                            detail::CanonicalFunctor<Real>>
{
public:
  using SeedType = vtkm::Vec<vtkm::UInt32, 1>;

  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleRandomUniformReal,
    (ArrayHandleRandomUniformReal<Real>),
    (vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleRandomUniformBits,
                                      detail::CanonicalFunctor<Real>>));

  explicit ArrayHandleRandomUniformReal(vtkm::Id length, SeedType seed = { std::random_device{}() })
    : Superclass(vtkm::cont::ArrayHandleRandomUniformBits{ length, seed },
                 detail::CanonicalFunctor<Real>{})
  {
  }
};

} // cont
} // vtkm
#endif //vtk_m_count_ArrayHandleRandomUniformReal_h
