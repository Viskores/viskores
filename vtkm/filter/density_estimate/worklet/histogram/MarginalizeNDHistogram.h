//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_MarginalizeNDHistogram_h
#define vtk_m_worklet_MarginalizeNDHistogram_h

#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace worklet
{
namespace histogram
{
// Set freq of the entity, which does not meet the condition, to 0
template <class BinaryCompare>
class ConditionalFreq : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  ConditionalFreq(BinaryCompare _bop)
    : bop(_bop)
  {
  }

  VTKM_CONT
  void setVar(vtkm::Id _var) { var = _var; }

  BinaryCompare bop;
  vtkm::Id var;

  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  VTKM_EXEC
  void operator()(const vtkm::Id& binIdIn, const vtkm::Id& freqIn, vtkm::Id& freqOut) const
  {
    if (bop(var, binIdIn))
      freqOut = freqIn;
    else
      freqOut = 0;
  }
};

// Convert multiple indices to 1D index
class To1DIndex : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn bin, FieldIn binIndexIn, FieldOut binIndexOut);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  vtkm::Id numberOfBins;

  VTKM_CONT
  To1DIndex(vtkm::Id numberOfBins0)
    : numberOfBins(numberOfBins0)
  {
  }

  VTKM_EXEC
  void operator()(const vtkm::Id& bin, const vtkm::Id& binIndexIn, vtkm::Id& binIndexOut) const
  {
    binIndexOut = binIndexIn * numberOfBins + bin;
  }
};
}
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_MarginalizeNDHistogram_h
