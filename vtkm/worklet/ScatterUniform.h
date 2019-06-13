//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_ScatterUniform_h
#define vtk_m_worklet_ScatterUniform_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/worklet/internal/ScatterBase.h>

namespace vtkm
{
namespace worklet
{

namespace detail
{

template <vtkm::IdComponent Modulus>
struct FunctorModulus
{
  VTKM_EXEC_CONT
  vtkm::IdComponent operator()(vtkm::Id index) const
  {
    return static_cast<vtkm::IdComponent>(index % Modulus);
  }
};

template <vtkm::IdComponent Divisor>
struct FunctorDiv
{
  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id index) const { return index / Divisor; }
};
}

/// \brief A scatter that maps input to some constant numbers of output.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterUniform establishes a 1 to N
/// mapping from input to output. That is, every input element generates N
/// elements associated with it where N is the same for every input. The output
/// elements are grouped by the input associated.
///
template <vtkm::IdComponent NumOutputsPerInput>
struct ScatterUniform : internal::ScatterBase
{
  VTKM_CONT ScatterUniform() = default;

  VTKM_CONT
  vtkm::Id GetOutputRange(vtkm::Id inputRange) const { return inputRange * NumOutputsPerInput; }
  VTKM_CONT
  vtkm::Id GetOutputRange(vtkm::Id3 inputRange) const
  {
    return this->GetOutputRange(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  using OutputToInputMapType =
    vtkm::cont::ArrayHandleImplicit<detail::FunctorDiv<NumOutputsPerInput>>;
  template <typename RangeType>
  VTKM_CONT OutputToInputMapType GetOutputToInputMap(RangeType inputRange) const
  {
    return OutputToInputMapType(detail::FunctorDiv<NumOutputsPerInput>(),
                                this->GetOutputRange(inputRange));
  }

  using VisitArrayType =
    vtkm::cont::ArrayHandleImplicit<detail::FunctorModulus<NumOutputsPerInput>>;
  template <typename RangeType>
  VTKM_CONT VisitArrayType GetVisitArray(RangeType inputRange) const
  {
    return VisitArrayType(detail::FunctorModulus<NumOutputsPerInput>(),
                          this->GetOutputRange(inputRange));
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_ScatterUniform_h
