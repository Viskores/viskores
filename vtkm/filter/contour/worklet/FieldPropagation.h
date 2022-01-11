
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_contour_FieldPropagation_h
#define vtk_m_worklet_contour_FieldPropagation_h

#include <vtkm/VectorAnalysis.h>
#include <vtkm/worklet/WorkletMapField.h>
namespace vtkm
{
namespace worklet
{
namespace contour
{

// ---------------------------------------------------------------------------
class MapPointField : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn interpolation_ids,
                                FieldIn interpolation_weights,
                                WholeArrayIn inputField,
                                FieldOut output);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  VTKM_CONT
  MapPointField() {}

  template <typename WeightType, typename InFieldPortalType, typename OutFieldType>
  VTKM_EXEC void operator()(const vtkm::Id2& low_high,
                            const WeightType& weight,
                            const InFieldPortalType& inPortal,
                            OutFieldType& result) const
  {
    //fetch the low / high values from inPortal
    result = static_cast<OutFieldType>(
      vtkm::Lerp(inPortal.Get(low_high[0]), inPortal.Get(low_high[1]), weight));
  }
};
}
}
}


#endif
