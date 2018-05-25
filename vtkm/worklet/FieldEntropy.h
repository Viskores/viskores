//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_FieldEntropy_h
#define vtk_m_worklet_FieldEntropy_h

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DeviceAdapter.h>
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
    using ControlSignature = void(FieldIn<> freq, FieldOut<> informationContent);
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
  template <typename FieldType, typename Storage, typename DeviceAdapter>
  vtkm::Float64 Run(vtkm::cont::ArrayHandle<FieldType, Storage> fieldArray,
                    vtkm::Id numberOfBins,
                    DeviceAdapter device)
  {
    using DeviceAlgorithms = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    ///// calculate histogram using FieldHistogram worklet /////
    vtkm::Range range;
    FieldType delta;
    vtkm::cont::ArrayHandle<vtkm::Id> binArray;
    vtkm::worklet::FieldHistogram histogram;
    histogram.Run(fieldArray, numberOfBins, range, delta, binArray, device);

    ///// calculate sum of frequency of the histogram /////
    vtkm::Id initFreqSumValue = 0;
    vtkm::Id freqSum = DeviceAlgorithms::Reduce(binArray, initFreqSumValue, vtkm::Sum());

    ///// calculate information content of each bin using self-define worklet /////
    vtkm::cont::ArrayHandle<vtkm::Float64> informationContent;
    SetBinInformationContent binWorklet(static_cast<vtkm::Float64>(freqSum));
    vtkm::worklet::DispatcherMapField<SetBinInformationContent, DeviceAdapter>
      setBinInformationContentDispatcher(binWorklet);
    setBinInformationContentDispatcher.Invoke(binArray, informationContent);

    ///// calculate entropy by summing up information conetent of all bins /////
    vtkm::Float64 initEntropyValue = 0;
    vtkm::Float64 entropy =
      DeviceAlgorithms::Reduce(informationContent, initEntropyValue, vtkm::Sum());

    return entropy;
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_FieldEntropy_h
