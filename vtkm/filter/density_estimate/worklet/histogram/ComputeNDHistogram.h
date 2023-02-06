//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_ComputeNDHistogram_h
#define vtk_m_worklet_ComputeNDHistogram_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayGetValues.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace histogram
{
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if defined(VTKM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc
template <typename T>
T compute_delta(T fieldMinValue, T fieldMaxValue, vtkm::Id num)
{
  using VecType = vtkm::VecTraits<T>;
  const T fieldRange = fieldMaxValue - fieldMinValue;
  return fieldRange / static_cast<typename VecType::ComponentType>(num);
}
#if defined(VTKM_GCC)
#pragma GCC diagnostic pop
#endif // gcc

// For each value set the bin it should be in
template <typename FieldType>
class SetHistogramBin : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn value, FieldIn binIndexIn, FieldOut binIndexOut);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  vtkm::Id numberOfBins;
  vtkm::Float64 minValue;
  vtkm::Float64 delta;

  VTKM_CONT
  SetHistogramBin(vtkm::Id numberOfBins0, vtkm::Float64 minValue0, vtkm::Float64 delta0)
    : numberOfBins(numberOfBins0)
    , minValue(minValue0)
    , delta(delta0)
  {
  }

  VTKM_EXEC
  void operator()(const FieldType& value, const vtkm::Id& binIndexIn, vtkm::Id& binIndexOut) const
  {
    const vtkm::Float64 fvalue = static_cast<vtkm::Float64>(value);
    vtkm::Id localBinIdx = static_cast<vtkm::Id>((fvalue - minValue) / delta);
    if (localBinIdx < 0)
      localBinIdx = 0;
    else if (localBinIdx >= numberOfBins)
      localBinIdx = numberOfBins - 1;

    binIndexOut = binIndexIn * numberOfBins + localBinIdx;
  }
};

// Convert N-dims bin index into 1D index
class ConvertHistBinToND : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn bin1DIndexIn,
                                FieldOut bin1DIndexOut,
                                FieldOut oneVariableIndexOut);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  vtkm::Id numberOfBins;

  VTKM_CONT
  explicit ConvertHistBinToND(vtkm::Id numberOfBins0)
    : numberOfBins(numberOfBins0)
  {
  }

  VTKM_EXEC
  void operator()(const vtkm::Id& bin1DIndexIn,
                  vtkm::Id& bin1DIndexOut,
                  vtkm::Id& oneVariableIndexOut) const
  {
    oneVariableIndexOut = bin1DIndexIn % numberOfBins;
    bin1DIndexOut = (bin1DIndexIn - oneVariableIndexOut) / numberOfBins;
  }
};
}
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ComputeNDHistogram_h
