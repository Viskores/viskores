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
    using philox_functor = vtkm::random::philox_functor2x32x10;
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

class VTKM_ALWAYS_EXPORT ArrayHandlePhiloxURBG
  : public vtkm::cont::ArrayHandleImplicit<detail::PhiloxFunctor>
{
public:
  using SeedType = vtkm::Vec<vtkm::UInt32, 1>;

  VTKM_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandlePhiloxURBG,
                                (vtkm::cont::ArrayHandleImplicit<detail::PhiloxFunctor>));

  explicit ArrayHandlePhiloxURBG(vtkm::Id length, SeedType seed)
    : Superclass(detail::PhiloxFunctor(seed), length)
  {
  }
}; // class ArrayHandlePhiloxURBG

/// A convenience function for creating an ArrayHandlePhiloxURBG. It takes the
/// size of the array and generates an array holding vtkm::Id from
/// [std::numeric_limits<vtkm::UInt64>::min, std::numeric_limits<vtkm::UInt64>::max]
/// The type of seed is specifically designed to be an vtkm::Vec<> to provide
/// type safety for the parameters so user will not transpose two integer parameters.
VTKM_CONT vtkm::cont::ArrayHandlePhiloxURBG make_ArrayHandlePhiloxURBG(
  vtkm::Id length,
  vtkm::Vec<vtkm::UInt32, 1> seed = { std::random_device{}() })
{
  return vtkm::cont::ArrayHandlePhiloxURBG(length, seed);
}
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
