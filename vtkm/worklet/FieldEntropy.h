//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/worklet/FieldHistogram.h>

#include <vtkm/cont/Field.h>

#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

namespace vtkm {
namespace worklet {

//simple functor that returns basic statistics
class FieldEntropy
{
public:
  // For each bin, calculate its information content (log2)
  template<typename FreqType>
  class SetBinInformationContent : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<> freq,
                                  FieldOut<> informationContent);
    typedef void ExecutionSignature(_1,_2);

    vtkm::Float64 freqSum;

    VTKM_CONT
    SetBinInformationContent(vtkm::Float64 _freqSum) : freqSum(_freqSum) {}

    VTKM_EXEC
    void operator()(const FreqType &freq, vtkm::Float64 &informationContent) const
    {
      vtkm::Float64 p = ( (vtkm::Float64)freq ) / freqSum;
      if( p > 0 )
        informationContent = -1 * p * log2(p);
      else
        informationContent = 0;
    }
  };


  // Execute the entropy computation filter given data(a field) and number of bins
  // Returns:
  // Entropy (log2) of the field of the data
  template<typename FieldType, typename Storage, typename DeviceAdapter>
  vtkm::Float64 Run(vtkm::cont::ArrayHandle<FieldType, Storage> fieldArray,
           vtkm::Id numberOfBins,
           DeviceAdapter device)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    ///// calculate histogram using FieldHistogram worklet /////
    vtkm::Range range;
    FieldType delta;
    vtkm::cont::ArrayHandle<vtkm::Id> binArray;
    vtkm::worklet::FieldHistogram histogram;
    histogram.Run(fieldArray, numberOfBins, range, delta, binArray, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());

    ///// calculate sum of frequency of the histogram /////
    vtkm::Id initFreqSumValue = 0;
    vtkm::Id freqSum = DeviceAlgorithms::Reduce(binArray, initFreqSumValue, vtkm::Sum());

    ///// calculate information content of each bin using self-define worklet /////
    vtkm::cont::ArrayHandle<vtkm::Float64> informationContent;
    informationContent.Allocate(numberOfBins);
    SetBinInformationContent<vtkm::Id> binWorklet( (vtkm::Float64)freqSum );
    vtkm::worklet::DispatcherMapField< SetBinInformationContent<vtkm::Id> > setBinInformationContentDispatcher(binWorklet);
    setBinInformationContentDispatcher.Invoke(binArray, informationContent);

    ///// calculate entropy by summing up information conetent of all bins /////
    vtkm::Float64 initEntropyValue = 0;
    vtkm::Float64 entropy = DeviceAlgorithms::Reduce(informationContent, initEntropyValue, vtkm::Sum());

    return entropy;
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_FieldHistogram_h
