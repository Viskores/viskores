//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_count_ArrayHandleRandomStandardNormal_h
#define vtk_m_count_ArrayHandleRandomStandardNormal_h

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleZip.h>

namespace vtkm
{
namespace cont
{
namespace detail
{
struct BoxMuller
{
  VTKM_EXEC_CONT vtkm::Float32 operator()(const vtkm::Pair<vtkm::Float32, vtkm::Float32>& uv) const
  {
    // We take two U(0, 1) and return one N(0, 1)
    return vtkm::Sqrt(-2.0f * vtkm::Log(uv.first)) * vtkm::Cos(2.0f * vtkm::TwoPif() * uv.second);
  }

  VTKM_EXEC_CONT vtkm::Float64 operator()(const vtkm::Pair<vtkm::Float64, vtkm::Float64>& uv) const
  {
    // We take two U(0, 1) and return one N(0, 1)
    return vtkm::Sqrt(-2.0 * vtkm::Log(uv.first)) * vtkm::Cos(2 * vtkm::TwoPi() * uv.second);
  }
};
} //detail

template <typename Real = vtkm::Float64>
class VTKM_ALWAYS_EXPORT ArrayHandleRandomStandardNormal
  : public vtkm::cont::ArrayHandleTransform<
      vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandleRandomUniformReal<Real>,
                                 vtkm::cont::ArrayHandleRandomUniformReal<Real>>,
      detail::BoxMuller>
{
public:
  using SeedType = vtkm::Vec<vtkm::UInt32, 1>;
  using UniformReal = vtkm::cont::ArrayHandleRandomUniformReal<Real>;

  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleRandomStandardNormal,
    (ArrayHandleRandomStandardNormal<Real>),
    (vtkm::cont::ArrayHandleTransform<
      vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandleRandomUniformReal<Real>,
                                 vtkm::cont::ArrayHandleRandomUniformReal<Real>>,
      detail::BoxMuller>));

  explicit ArrayHandleRandomStandardNormal(vtkm::Id length,
                                           SeedType seed = { std::random_device{}() })
    : Superclass(vtkm::cont::make_ArrayHandleZip(UniformReal{ length, seed },
                                                 UniformReal{ length, { ~seed[0] } }),
                 detail::BoxMuller{})
  {
  }
};
}
}
#endif // vtk_m_count_ArrayHandleRandomStandardNormal_h
