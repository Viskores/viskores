//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandlePhiloxURBG_h
#define vtk_m_cont_ArrayHandlePhiloxURBG_h

#include <random>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/random/Philox.h>

namespace vtkm
{
namespace cont
{

namespace detail
{
struct PhiloxFunctor
{
  using SeedType = vtkm::Vec<vtkm::UInt32, 1>;

  PhiloxFunctor() = default;

  explicit PhiloxFunctor(SeedType seed)
    : Seed(seed)
  {
  }

  VTKM_EXEC_CONT
  vtkm::UInt64 operator()(vtkm::Id index) const
  {
    using philox_functor = vtkm::random::PhiloxFunctor2x32x10;
    using counters_type = typename philox_functor::counters_type;

    // We deliberately use type punning to convert vtkm::Id into counters and then to
    // convert counters to vtkm::UInt64. All we need is a unique bit string as input
    // and output of the functor.
    auto idx = static_cast<vtkm::UInt64>(index);
    counters_type counters = *reinterpret_cast<counters_type*>(&idx);
    counters_type result = philox_functor{}(counters, Seed);
    return *reinterpret_cast<vtkm::UInt64*>(&result);
  }

private:
  const SeedType Seed{};
}; // class PhiloxFunctor
} // namespace detail

/// \brief An \c ArrayHandle that provides a source of random bits
///
/// \c ArrayHandleRandomUniformBits is a specialization of ArrayHandleImplicit.
/// It takes a user supplied seed and hash it with the a given index value. The
/// hashed value is the value of the array at that position.
///
/// Currently, Philox2x32x10 as described in the
///   "Parallel Random Numbers: As Easy as 1, 2, 3," Proceedings of the
///   International Conference for High Performance Computing, Networking,
///   Storage and Analysis (SC11)
/// hash function.
///
/// Note: In contrast to traditional random number generator,
/// ArrayHandleRandomUniformBits does not have "state", i.e. multiple calls
/// the Get() method with the same index will always return the same hash value.
/// To ge a new set of random bits, create a new ArrayHandleRandomUniformBits
/// with a different seed.
class VTKM_ALWAYS_EXPORT ArrayHandleRandomUniformBits
  : public vtkm::cont::ArrayHandleImplicit<detail::PhiloxFunctor>
{
public:
  using SeedType = vtkm::Vec<vtkm::UInt32, 1>;

  VTKM_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleRandomUniformBits,
                                (vtkm::cont::ArrayHandleImplicit<detail::PhiloxFunctor>));

  /// The type of seed is specifically designed to be an vtkm::Vec<> to provide
  /// type safety for the parameters so user will not transpose two integer parameters.
  explicit ArrayHandleRandomUniformBits(vtkm::Id length, SeedType seed = { std::random_device{}() })
    : Superclass(detail::PhiloxFunctor(seed), length)
  {
  }
}; // class ArrayHandleRandomUniformBits
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION

namespace vtkm
{
namespace cont
{
}
} // namespace vtkm::cont
#endif //vtk_m_cont_ArrayHandlePhiloxURBG_h
