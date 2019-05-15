//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_ScatterPermutation_h
#define vtk_m_worklet_ScatterPermutation_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/worklet/internal/ScatterBase.h>

namespace vtkm
{
namespace worklet
{

/// \brief A scatter that maps input to output based on a permutation array.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterPermutation is similar to
/// \c ScatterCounting but can have lesser memory usage for some cases.
/// The constructor takes an array of ids, where each entry maps the
/// corresponding output to an input. The ids can be in any order and there
/// can be duplicates. Note that even with duplicates the VistIndex is always 0.
///
template <typename PermutationStorage = VTKM_DEFAULT_STORAGE_TAG>
class ScatterPermutation : public internal::ScatterBase
{
private:
  using PermutationArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id, PermutationStorage>;

public:
  using OutputToInputMapType = PermutationArrayHandle;
  using VisitArrayType = vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>;

  ScatterPermutation(const PermutationArrayHandle& permutation)
    : Permutation(permutation)
  {
  }

  VTKM_CONT
  template <typename RangeType>
  vtkm::Id GetOutputRange(RangeType) const
  {
    return this->Permutation.GetNumberOfValues();
  }

  template <typename RangeType>
  VTKM_CONT OutputToInputMapType GetOutputToInputMap(RangeType) const
  {
    return this->Permutation;
  }

  VTKM_CONT OutputToInputMapType GetOutputToInputMap() const { return this->Permutation; }

  VTKM_CONT
  VisitArrayType GetVisitArray(vtkm::Id inputRange) const { return VisitArrayType(0, inputRange); }

  VTKM_CONT
  VisitArrayType GetVisitArray(vtkm::Id3 inputRange) const
  {
    return this->GetVisitArray(inputRange[0] * inputRange[1] * inputRange[2]);
  }

private:
  PermutationArrayHandle Permutation;
};
}
} // vtkm::worklet

#endif // vtk_m_worklet_ScatterPermutation_h
