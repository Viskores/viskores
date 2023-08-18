//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_NDimsHistogram_h
#define vtk_m_worklet_NDimsHistogram_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/filter/density_estimate/worklet/histogram/ComputeNDHistogram.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace worklet
{

class NDimsHistogram
{
public:
  void SetNumOfDataPoints(vtkm::Id _numDataPoints)
  {
    NumDataPoints = _numDataPoints;

    // Initialize bin1DIndex array
    vtkm::cont::ArrayHandleConstant<vtkm::Id> constant0Array(0, NumDataPoints);
    vtkm::cont::ArrayCopy(constant0Array, Bin1DIndex);
  }

  // Add a field and the bin number for this field along with specific range of the data
  // Return: binDelta is delta of a bin
  template <typename HandleType>
  void AddField(const HandleType& fieldArray,
                vtkm::Id numberOfBins,
                vtkm::Range& rangeOfValues,
                vtkm::Float64& binDelta,
                bool rangeProvided = false)
  {
    NumberOfBins.push_back(numberOfBins);

    if (fieldArray.GetNumberOfValues() != NumDataPoints)
    {
      throw vtkm::cont::ErrorBadValue("Array lengths does not match");
    }
    else
    {
      auto computeBins = [&](auto resolvedField) {
        // CastAndCallWithExtractedArray should give us an ArrayHandleRecombineVec
        using T = typename std::decay_t<decltype(resolvedField)>::ValueType::ComponentType;
        vtkm::cont::ArrayHandleRecombineVec<T> recombineField{ resolvedField };
        if (recombineField.GetNumberOfComponents() != 1)
        {
          VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                     "NDHistogram expects scalar fields, but was given field with "
                       << recombineField.GetNumberOfComponents()
                       << " components. Extracting first component.");
        }
        vtkm::cont::ArrayHandleStride<T> field =
          vtkm::cont::ArrayExtractComponent(recombineField, 0);
        if (!rangeProvided)
        {
          const vtkm::Vec<T, 2> initValue(vtkm::cont::ArrayGetValue(0, field));
          vtkm::Vec<T, 2> minMax =
            vtkm::cont::Algorithm::Reduce(field, initValue, vtkm::MinAndMax<T>());
          rangeOfValues.Min = static_cast<vtkm::Float64>(minMax[0]);
          rangeOfValues.Max = static_cast<vtkm::Float64>(minMax[1]);
        }
        binDelta = vtkm::worklet::histogram::compute_delta(
          rangeOfValues.Min, rangeOfValues.Max, numberOfBins);

        vtkm::worklet::histogram::SetHistogramBin<T> binWorklet(
          numberOfBins, rangeOfValues.Min, binDelta);
        vtkm::cont::Invoker{}(binWorklet, field, this->Bin1DIndex, this->Bin1DIndex);
      };
      fieldArray.CastAndCallWithExtractedArray(computeBins);
    }
  }

  // Execute N-Dim histogram worklet to get N-Dims histogram from input fields
  // Input arguments:
  //   binId: returned bin id of NDims-histogram, binId has n arrays, if length of fieldName is n
  //   freqs: returned frequency(count) array
  //     Note: the ND-histogram is returned in the fashion of sparse representation.
  //           (no zero frequency in freqs array)
  //           the length of all arrays in binId and freqs array must be the same
  //           if the length of fieldNames is n (compute a n-dimensional hisotgram)
  //           freqs[i] is the frequency of the bin with bin Ids{ binId[0][i], binId[1][i], ... binId[n-1][i] }
  void Run(std::vector<vtkm::cont::ArrayHandle<vtkm::Id>>& binId,
           vtkm::cont::ArrayHandle<vtkm::Id>& freqs)
  {
    binId.resize(NumberOfBins.size());

    // Sort the resulting bin(1D) array for counting
    vtkm::cont::Algorithm::Sort(Bin1DIndex);

    // Count frequency of each bin
    vtkm::cont::ArrayHandleConstant<vtkm::Id> constArray(1, NumDataPoints);
    vtkm::cont::Algorithm::ReduceByKey(Bin1DIndex, constArray, Bin1DIndex, freqs, vtkm::Add());

    //convert back to multi variate binId
    for (vtkm::Id i = static_cast<vtkm::Id>(NumberOfBins.size()) - 1; i >= 0; i--)
    {
      const vtkm::Id nFieldBins = NumberOfBins[static_cast<size_t>(i)];
      vtkm::worklet::histogram::ConvertHistBinToND binWorklet(nFieldBins);
      vtkm::worklet::DispatcherMapField<vtkm::worklet::histogram::ConvertHistBinToND>
        convertHistBinToNDDispatcher(binWorklet);
      size_t vectorId = static_cast<size_t>(i);
      convertHistBinToNDDispatcher.Invoke(Bin1DIndex, Bin1DIndex, binId[vectorId]);
    }
  }

private:
  std::vector<vtkm::Id> NumberOfBins;
  vtkm::cont::ArrayHandle<vtkm::Id> Bin1DIndex;
  vtkm::Id NumDataPoints;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_NDimsHistogram_h
