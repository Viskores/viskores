//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_ComputeNDEntropy_h
#define vtk_m_worklet_ComputeNDEntropy_h

#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace worklet
{
namespace histogram
{
// For each bin, calculate its information content (log2)
class SetBinInformationContent : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn freq, FieldOut informationContent);
  using ExecutionSignature = void(_1, _2);

  vtkm::Float64 FreqSum;

  VTKM_CONT
  SetBinInformationContent(vtkm::Float64 _freqSum)
    : FreqSum(_freqSum)
  {
  }

  template <typename FreqType>
  VTKM_EXEC void operator()(const FreqType& freq, vtkm::Float64& informationContent) const
  {
    vtkm::Float64 p = ((vtkm::Float64)freq) / FreqSum;
    if (p > 0)
      informationContent = -1 * p * vtkm::Log2(p);
    else
      informationContent = 0;
  }
};
}
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ComputeNDEntropy_h
