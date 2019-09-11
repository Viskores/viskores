//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_NDimsHistMarginalization_h
#define vtk_m_worklet_NDimsHistMarginalization_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/histogram/ComputeNDHistogram.h>
#include <vtkm/worklet/histogram/ComputeNDHistogram.h>
#include <vtkm/worklet/histogram/ComputeNDHistogram.h>
#include <vtkm/worklet/histogram/MarginalizeNDHistogram.h>
#include <vtkm/worklet/histogram/MarginalizeNDHistogram.h>
#include <vtkm/worklet/histogram/MarginalizeNDHistogram.h>

#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace worklet
{


class NDimsHistMarginalization
{
public:
  // Execute the histogram (conditional) marginalization,
  //   given the multi-variable histogram(binId, freqIn)
  //   , marginalVariable and marginal condition
  // Input arguments:
  //   binId, freqsIn: input ND-histogram in the fashion of sparse representation
  //                   (definition of binId and frqIn please refer to NDimsHistogram.h),
  //                   (binId.size() is the number of variables)
  //   numberOfBins: number of bins of each variable (length of numberOfBins must be the same as binId.size() )
  //   marginalVariables: length is the same as number of variables.
  //                      1 indicates marginal variable, otherwise 0.
  //   conditionFunc: The Condition function for non-marginal variable.
  //                  This func takes two arguments (vtkm::Id var, vtkm::Id binId) and return bool
  //                  var is index of variable and binId is bin index in the variable var
  //                  return true indicates considering this bin into final marginal histogram
  //                  more details can refer to example in UnitTestNDimsHistMarginalization.cxx
  //   marginalBinId, marginalFreqs: return marginalized histogram in the fashion of sparse representation
  //                                 the definition is the same as (binId and freqsIn)
  template <typename BinaryCompare>
  void Run(const std::vector<vtkm::cont::ArrayHandle<vtkm::Id>>& binId,
           vtkm::cont::ArrayHandle<vtkm::Id>& freqsIn,
           vtkm::cont::ArrayHandle<vtkm::Id>& numberOfBins,
           vtkm::cont::ArrayHandle<bool>& marginalVariables,
           BinaryCompare conditionFunc,
           std::vector<vtkm::cont::ArrayHandle<vtkm::Id>>& marginalBinId,
           vtkm::cont::ArrayHandle<vtkm::Id>& marginalFreqs)
  {
    //total variables
    vtkm::Id numOfVariable = static_cast<vtkm::Id>(binId.size());

    const vtkm::Id numberOfValues = freqsIn.GetNumberOfValues();
    vtkm::cont::ArrayHandleConstant<vtkm::Id> constant0Array(0, numberOfValues);
    vtkm::cont::ArrayHandle<vtkm::Id> bin1DIndex;
    vtkm::cont::ArrayCopy(constant0Array, bin1DIndex);

    vtkm::cont::ArrayHandle<vtkm::Id> freqs;
    vtkm::cont::ArrayCopy(freqsIn, freqs);
    vtkm::Id numMarginalVariables = 0; //count num of marginal variables
    const auto marginalPortal = marginalVariables.GetPortalConstControl();
    const auto numBinsPortal = numberOfBins.GetPortalConstControl();
    for (vtkm::Id i = 0; i < numOfVariable; i++)
    {
      if (marginalPortal.Get(i) == true)
      {
        // Worklet to calculate 1D index for marginal variables
        numMarginalVariables++;
        const vtkm::Id nFieldBins = numBinsPortal.Get(i);
        vtkm::worklet::histogram::To1DIndex binWorklet(nFieldBins);
        vtkm::worklet::DispatcherMapField<vtkm::worklet::histogram::To1DIndex> to1DIndexDispatcher(
          binWorklet);
        size_t vecIndex = static_cast<size_t>(i);
        to1DIndexDispatcher.Invoke(binId[vecIndex], bin1DIndex, bin1DIndex);
      }
      else
      { //non-marginal variable
        // Worklet to set the frequency of entities which does not meet the condition
        // to 0 on non-marginal variables
        vtkm::worklet::histogram::ConditionalFreq<BinaryCompare> conditionalFreqWorklet{
          conditionFunc
        };
        conditionalFreqWorklet.setVar(i);
        vtkm::worklet::DispatcherMapField<vtkm::worklet::histogram::ConditionalFreq<BinaryCompare>>
          cfDispatcher(conditionalFreqWorklet);
        size_t vecIndex = static_cast<size_t>(i);
        cfDispatcher.Invoke(binId[vecIndex], freqs, freqs);
      }
    }


    // Sort the freq array for counting by key(1DIndex)
    vtkm::cont::Algorithm::SortByKey(bin1DIndex, freqs);

    // Add frequency within same 1d index bin (this get a nonSparse representation)
    vtkm::cont::ArrayHandle<vtkm::Id> nonSparseMarginalFreqs;
    vtkm::cont::Algorithm::ReduceByKey(
      bin1DIndex, freqs, bin1DIndex, nonSparseMarginalFreqs, vtkm::Add());

    // Convert to sparse representation(remove all zero freqncy entities)
    vtkm::cont::ArrayHandle<vtkm::Id> sparseMarginal1DBinId;
    vtkm::cont::Algorithm::CopyIf(bin1DIndex, nonSparseMarginalFreqs, sparseMarginal1DBinId);
    vtkm::cont::Algorithm::CopyIf(nonSparseMarginalFreqs, nonSparseMarginalFreqs, marginalFreqs);

    //convert back to multi variate binId
    marginalBinId.resize(static_cast<size_t>(numMarginalVariables));
    vtkm::Id marginalVarIdx = numMarginalVariables - 1;
    for (vtkm::Id i = numOfVariable - 1; i >= 0; i--)
    {
      if (marginalPortal.Get(i) == true)
      {
        const vtkm::Id nFieldBins = numBinsPortal.Get(i);
        vtkm::worklet::histogram::ConvertHistBinToND binWorklet(nFieldBins);
        vtkm::worklet::DispatcherMapField<vtkm::worklet::histogram::ConvertHistBinToND>
          convertHistBinToNDDispatcher(binWorklet);
        size_t vecIndex = static_cast<size_t>(marginalVarIdx);
        convertHistBinToNDDispatcher.Invoke(
          sparseMarginal1DBinId, sparseMarginal1DBinId, marginalBinId[vecIndex]);
        marginalVarIdx--;
      }
    }
  } //Run()

  // Execute the histogram marginalization WITHOUT CONDITION,
  // Please refer to the other Run() functions for the definition of input arguments.
  void Run(const std::vector<vtkm::cont::ArrayHandle<vtkm::Id>>& binId,
           vtkm::cont::ArrayHandle<vtkm::Id>& freqsIn,
           vtkm::cont::ArrayHandle<vtkm::Id>& numberOfBins,
           vtkm::cont::ArrayHandle<bool>& marginalVariables,
           std::vector<vtkm::cont::ArrayHandle<vtkm::Id>>& marginalBinId,
           vtkm::cont::ArrayHandle<vtkm::Id>& marginalFreqs)
  {
    //total variables
    vtkm::Id numOfVariable = static_cast<vtkm::Id>(binId.size());

    const vtkm::Id numberOfValues = freqsIn.GetNumberOfValues();
    vtkm::cont::ArrayHandleConstant<vtkm::Id> constant0Array(0, numberOfValues);
    vtkm::cont::ArrayHandle<vtkm::Id> bin1DIndex;
    vtkm::cont::ArrayCopy(constant0Array, bin1DIndex);

    vtkm::cont::ArrayHandle<vtkm::Id> freqs;
    vtkm::cont::ArrayCopy(freqsIn, freqs);
    vtkm::Id numMarginalVariables = 0; //count num of marginal variables
    const auto marginalPortal = marginalVariables.GetPortalConstControl();
    const auto numBinsPortal = numberOfBins.GetPortalConstControl();
    for (vtkm::Id i = 0; i < numOfVariable; i++)
    {
      if (marginalPortal.Get(i) == true)
      {
        // Worklet to calculate 1D index for marginal variables
        numMarginalVariables++;
        const vtkm::Id nFieldBins = numBinsPortal.Get(i);
        vtkm::worklet::histogram::To1DIndex binWorklet(nFieldBins);
        vtkm::worklet::DispatcherMapField<vtkm::worklet::histogram::To1DIndex> to1DIndexDispatcher(
          binWorklet);
        size_t vecIndex = static_cast<size_t>(i);
        to1DIndexDispatcher.Invoke(binId[vecIndex], bin1DIndex, bin1DIndex);
      }
    }

    // Sort the freq array for counting by key (1DIndex)
    vtkm::cont::Algorithm::SortByKey(bin1DIndex, freqs);

    // Add frequency within same 1d index bin
    vtkm::cont::Algorithm::ReduceByKey(bin1DIndex, freqs, bin1DIndex, marginalFreqs, vtkm::Add());

    //convert back to multi variate binId
    marginalBinId.resize(static_cast<size_t>(numMarginalVariables));
    vtkm::Id marginalVarIdx = numMarginalVariables - 1;
    for (vtkm::Id i = numOfVariable - 1; i >= 0; i--)
    {
      if (marginalPortal.Get(i) == true)
      {
        const vtkm::Id nFieldBins = numBinsPortal.Get(i);
        vtkm::worklet::histogram::ConvertHistBinToND binWorklet(nFieldBins);
        vtkm::worklet::DispatcherMapField<vtkm::worklet::histogram::ConvertHistBinToND>
          convertHistBinToNDDispatcher(binWorklet);
        size_t vecIndex = static_cast<size_t>(marginalVarIdx);
        convertHistBinToNDDispatcher.Invoke(bin1DIndex, bin1DIndex, marginalBinId[vecIndex]);
        marginalVarIdx--;
      }
    }
  } //Run()
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_NDimsHistMarginalization_h
