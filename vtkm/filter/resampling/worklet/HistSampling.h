//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_HistSampling_h
#define vtk_m_worklet_HistSampling_h

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
class AcceptanceProbsWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, _4);
  template <typename TypeOutPortal>
  VTKM_EXEC void operator()(const vtkm::FloatDefault& targetSampleNum,
                            const vtkm::Id& binIndex,
                            const vtkm::Id& binCount,
                            TypeOutPortal arrayOutPortal) const
  {
    if (binCount < 1 || targetSampleNum < 0.000001)
    {
      arrayOutPortal.Set(binIndex, 0.0);
    }
    else
    {
      arrayOutPortal.Set(binIndex, targetSampleNum / static_cast<vtkm::FloatDefault>(binCount));
    }
  }
};

class LookupWorklet : public vtkm::worklet::WorkletMapField
{
private:
  vtkm::Id m_num_bins;
  vtkm::Float64 m_min;
  vtkm::Float64 m_bin_delta;

public:
  LookupWorklet(vtkm::Id num_bins, vtkm::Float64 min_value, vtkm::Float64 bin_delta)
    : m_num_bins(num_bins)
    , m_min(min_value)
    , m_bin_delta(bin_delta)
  {
  }

  using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn, FieldIn);
  using ExecutionSignature = _2(_1, _3, _4);
  template <typename TablePortal, typename FieldType>
  VTKM_EXEC vtkm::FloatDefault operator()(const FieldType& field_value,
                                          TablePortal table,
                                          const vtkm::FloatDefault& random) const
  {
    vtkm::Id bin = static_cast<vtkm::Id>((field_value - m_min) / m_bin_delta);
    if (bin < 0)
    {
      bin = 0;
    }
    if (bin >= m_num_bins)
    {
      bin = m_num_bins - 1;
    }
    return random < table.Get(bin);
  }
};
};
} // vtkm::worklet


#endif // vtk_m_worklet_HistSampling_h
