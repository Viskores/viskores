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

class ComputeBins
{
public:
  VTKM_CONT
  ComputeBins(vtkm::cont::ArrayHandle<vtkm::Id>& _bin1DIdx,
              vtkm::Id& _numOfBins,
              vtkm::Range& _minMax,
              vtkm::Float64& _binDelta)
    : Bin1DIdx(_bin1DIdx)
    , NumOfBins(_numOfBins)
    , MinMax(_minMax)
    , BinDelta(_binDelta)
  {
  }

  template <typename T, typename Storage>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, Storage>& field) const
  {
    const vtkm::Vec<T, 2> initValue(vtkm::cont::ArrayGetValue(0, field));
    vtkm::Vec<T, 2> minMax = vtkm::cont::Algorithm::Reduce(field, initValue, vtkm::MinAndMax<T>());
    MinMax.Min = static_cast<vtkm::Float64>(minMax[0]);
    MinMax.Max = static_cast<vtkm::Float64>(minMax[1]);
    BinDelta = compute_delta(MinMax.Min, MinMax.Max, NumOfBins);

    SetHistogramBin<T> binWorklet(NumOfBins, MinMax.Min, BinDelta);
    vtkm::worklet::DispatcherMapField<vtkm::worklet::histogram::SetHistogramBin<T>>
      setHistogramBinDispatcher(binWorklet);
    setHistogramBinDispatcher.Invoke(field, Bin1DIdx, Bin1DIdx);
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id>& Bin1DIdx;
  vtkm::Id& NumOfBins;
  vtkm::Range& MinMax;
  vtkm::Float64& BinDelta;
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
  ConvertHistBinToND(vtkm::Id numberOfBins0)
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
