//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_NDimsEntropy_h
#define vtk_m_worklet_NDimsEntropy_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/NDimsHistogram.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/histogram/ComputeNDEntropy.h>

#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace worklet
{

class NDimsEntropy
{
public:
  void SetNumOfDataPoints(vtkm::Id _numDataPoints)
  {
    NumDataPoints = _numDataPoints;
    NdHistogram.SetNumOfDataPoints(_numDataPoints);
  }

  // Add a field and the bin for this field
  // Return: rangeOfRange is min max value of this array
  //         binDelta is delta of a bin
  template <typename HandleType>
  void AddField(const HandleType& fieldArray, vtkm::Id numberOfBins)
  {
    vtkm::Range range;
    vtkm::Float64 delta;

    NdHistogram.AddField(fieldArray, numberOfBins, range, delta);
  }

  // Execute the entropy computation filter given
  // fields and number of bins of each fields
  // Returns:
  // Entropy (log2) of the field of the data
  vtkm::Float64 Run()
  {
    std::vector<vtkm::cont::ArrayHandle<vtkm::Id>> binIds;
    vtkm::cont::ArrayHandle<vtkm::Id> freqs;
    NdHistogram.Run(binIds, freqs);

    ///// calculate sum of frequency of the histogram /////
    vtkm::Id initFreqSumValue = 0;
    vtkm::Id freqSum = vtkm::cont::Algorithm::Reduce(freqs, initFreqSumValue, vtkm::Sum());

    ///// calculate information content of each bin using self-define worklet /////
    vtkm::cont::ArrayHandle<vtkm::Float64> informationContent;
    vtkm::worklet::histogram::SetBinInformationContent binWorklet(
      static_cast<vtkm::Float64>(freqSum));
    vtkm::worklet::DispatcherMapField<vtkm::worklet::histogram::SetBinInformationContent>
      setBinInformationContentDispatcher(binWorklet);
    setBinInformationContentDispatcher.Invoke(freqs, informationContent);

    ///// calculate entropy by summing up information conetent of all bins /////
    vtkm::Float64 initEntropyValue = 0;
    vtkm::Float64 entropy =
      vtkm::cont::Algorithm::Reduce(informationContent, initEntropyValue, vtkm::Sum());

    return entropy;
  }

private:
  vtkm::worklet::NDimsHistogram NdHistogram;
  vtkm::Id NumDataPoints;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_NDimsEntropy_h
