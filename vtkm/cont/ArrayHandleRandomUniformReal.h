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

/// @brief An `ArrayHandle` that provides a source of random numbers with uniform distribution.
///
/// `ArrayHandleRandomUniformReal` takes a user supplied seed and hashes it to provide
/// a sequence of numbers drawn from a random uniform distribution in the range [0, 1).
/// `ArrayHandleRandomUniformReal` is built on top of `ArrayHandleRandomUniformBits` so
/// shares its behavior with that array.
///
/// Note: In contrast to traditional random number generator,
/// `ArrayHandleRandomUniformReal` does not have "state", i.e. multiple calls
/// the Get() method with the same index will always return the same hash value.
/// To get a new set of random bits, create a new `ArrayHandleRandomUniformBits`
/// with a different seed.
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

  /// Construct an `ArrayHandleRandomUniformReal`.
  ///
  /// @param length Specifies the length of the generated array.
  /// @param seed Provides a seed to use for the pseudorandom numbers. To prevent confusion
  /// between the seed and the length, the type of the seed is a `vtkm::Vec` of size 1. To
  /// specify the seed, declare it in braces. For example, to construct a random array of
  /// size 50 with seed 123, use `ArrayHandleRandomUniformReal(50, { 123 })`.
  explicit ArrayHandleRandomUniformReal(vtkm::Id length, SeedType seed = { std::random_device{}() })
    : Superclass(vtkm::cont::ArrayHandleRandomUniformBits{ length, seed },
                 detail::CanonicalFunctor<Real>{})
  {
  }
};

} // cont
} // vtkm
#endif //vtk_m_count_ArrayHandleRandomUniformReal_h
