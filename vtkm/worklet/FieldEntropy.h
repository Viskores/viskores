//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_FieldEntropy_h
#define vtk_m_worklet_FieldEntropy_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/FieldHistogram.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace worklet
{

//simple functor that returns basic statistics
class FieldEntropy
{
public:
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


  // Execute the entropy computation filter given data(a field) and number of bins
  // Returns:
  // Entropy (log2) of the field of the data
  template <typename FieldType, typename Storage>
  vtkm::Float64 Run(vtkm::cont::ArrayHandle<FieldType, Storage> fieldArray, vtkm::Id numberOfBins)
  {
    ///// calculate histogram using FieldHistogram worklet /////
    vtkm::Range range;
    FieldType delta;
    vtkm::cont::ArrayHandle<vtkm::Id> binArray;
    vtkm::worklet::FieldHistogram histogram;
    histogram.Run(fieldArray, numberOfBins, range, delta, binArray);

    ///// calculate sum of frequency of the histogram /////
    vtkm::Id initFreqSumValue = 0;
    vtkm::Id freqSum = vtkm::cont::Algorithm::Reduce(binArray, initFreqSumValue, vtkm::Sum());

    ///// calculate information content of each bin using self-define worklet /////
    vtkm::cont::ArrayHandle<vtkm::Float64> informationContent;
    SetBinInformationContent binWorklet(static_cast<vtkm::Float64>(freqSum));
    vtkm::worklet::DispatcherMapField<SetBinInformationContent> setBinInformationContentDispatcher(
      binWorklet);
    setBinInformationContentDispatcher.Invoke(binArray, informationContent);

    ///// calculate entropy by summing up information conetent of all bins /////
    vtkm::Float64 initEntropyValue = 0;
    vtkm::Float64 entropy =
      vtkm::cont::Algorithm::Reduce(informationContent, initEntropyValue, vtkm::Sum());

    return entropy;
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_FieldEntropy_h
