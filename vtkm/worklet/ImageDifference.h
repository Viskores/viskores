//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_ImageDifference_h
#define vtk_m_worklet_ImageDifference_h

#include <vtkm/VecTraits.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

namespace vtkm
{
namespace worklet
{

class ImageDifferenceNeighborhood : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldIn, FieldOut, FieldOut);
  using ExecutionSignature = void(_2, _3, Boundary, _4, _5);
  using InputDomain = _1;

  ImageDifferenceNeighborhood(const vtkm::IdComponent& radius, const vtkm::FloatDefault& threshold)
    : ShiftRadius(radius)
    , Threshold(threshold)
  {
  }

  template <typename InputFieldPortalType>
  VTKM_EXEC void operator()(
    const vtkm::exec::FieldNeighborhood<InputFieldPortalType>& primaryNeighborhood,
    const typename InputFieldPortalType::ValueType& secondary,
    const vtkm::exec::BoundaryState& boundary,
    typename InputFieldPortalType::ValueType& diff,
    vtkm::FloatDefault& diffThreshold) const
  {
    using T = typename InputFieldPortalType::ValueType;

    auto minIndices = boundary.MinNeighborIndices(this->ShiftRadius);
    auto maxIndices = boundary.MaxNeighborIndices(this->ShiftRadius);

    T minPixelDiff;
    vtkm::FloatDefault minPixelDiffThreshold = 10000.0f;
    for (vtkm::IdComponent i = minIndices[0]; i <= maxIndices[0]; i++)
    {
      for (vtkm::IdComponent j = minIndices[1]; j <= maxIndices[1]; j++)
      {
        for (vtkm::IdComponent k = minIndices[2]; k <= maxIndices[2]; k++)
        {
          diff = vtkm::Abs(primaryNeighborhood.Get(i, j, k) - secondary);
          diffThreshold = static_cast<vtkm::FloatDefault>(vtkm::Magnitude(diff));
          if (diffThreshold < this->Threshold)
          {
            return;
          }
          if (diffThreshold < minPixelDiffThreshold)
          {
            minPixelDiffThreshold = diffThreshold;
            minPixelDiff = diff;
          }
        }
      }
    }
    diff = minPixelDiff;
    diffThreshold = minPixelDiffThreshold;
  }

private:
  vtkm::IdComponent ShiftRadius;
  vtkm::FloatDefault Threshold;
};

class ImageDifference : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  ImageDifference() = default;

  template <typename T, vtkm::IdComponent Size>
  VTKM_EXEC void operator()(const vtkm::Vec<T, Size>& primary,
                            const vtkm::Vec<T, Size>& secondary,
                            vtkm::Vec<T, Size>& diff,
                            vtkm::FloatDefault& diffThreshold) const
  {
    diff = vtkm::Abs(primary - secondary);
    diffThreshold = static_cast<vtkm::FloatDefault>(vtkm::Magnitude(diff));
  }
};


} // vtkm::worklet
} // vtkm

#endif // vtk_m_worklet_ImageDifference_h
