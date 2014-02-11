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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_cont_internal_DeviceAdapterAlgorithmSerial_h
#define vtkm_cont_internal_DeviceAdapterAlgorithmSerial_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <vtkm/cont/internal/DeviceAdapterTagSerial.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>

#include <boost/iterator/counting_iterator.hpp>
#include <boost/utility/enable_if.hpp>

#include <algorithm>
#include <numeric>

namespace vtkm {
namespace cont {

template<>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagSerial> :
    vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
        DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagSerial>,
        vtkm::cont::DeviceAdapterTagSerial>
{
private:
  typedef vtkm::cont::DeviceAdapterTagSerial Device;

public:
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    typedef typename vtkm::cont::ArrayHandle<T,COut>
        ::template ExecutionTypes<Device>::Portal PortalOut;
    typedef typename vtkm::cont::ArrayHandle<T,CIn>
        ::template ExecutionTypes<Device>::PortalConst PortalIn;

    vtkm::Id numberOfValues = input.GetNumberOfValues();

    PortalIn inputPortal = input.PrepareForInput(Device());
    PortalOut outputPortal = output.PrepareForOutput(numberOfValues, Device());

    if (numberOfValues <= 0) { return 0; }

    std::partial_sum(inputPortal.GetIteratorBegin(),
                     inputPortal.GetIteratorEnd(),
                     outputPortal.GetIteratorBegin());

    // Return the value at the last index in the array, which is the full sum.
    return outputPortal.Get(numberOfValues - 1);
  }

  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    typedef typename vtkm::cont::ArrayHandle<T,COut>
        ::template ExecutionTypes<Device>::Portal PortalOut;
    typedef typename vtkm::cont::ArrayHandle<T,CIn>
        ::template ExecutionTypes<Device>::PortalConst PortalIn;

    vtkm::Id numberOfValues = input.GetNumberOfValues();

    PortalIn inputPortal = input.PrepareForInput(Device());
    PortalOut outputPortal = output.PrepareForOutput(numberOfValues, Device());

    if (numberOfValues <= 0) { return 0; }

    std::partial_sum(inputPortal.GetIteratorBegin(),
                     inputPortal.GetIteratorEnd(),
                     outputPortal.GetIteratorBegin());

    T fullSum = outputPortal.Get(numberOfValues - 1);

    // Shift right by one
    std::copy_backward(outputPortal.GetIteratorBegin(),
                       outputPortal.GetIteratorEnd()-1,
                       outputPortal.GetIteratorEnd());
    outputPortal.Set(0, 0);
    return fullSum;
  }

private:
  // This runs in the execution environment.
  template<class FunctorType>
  class ScheduleKernel
  {
  public:
    ScheduleKernel(const FunctorType &functor)
      : Functor(functor) {  }

    //needed for when calling from schedule on a range
    VTKM_EXEC_EXPORT void operator()(vtkm::Id index) const
    {
      this->Functor(index);
    }

  private:
    const FunctorType Functor;
  };

public:
  template<class Functor>
  VTKM_CONT_EXPORT static void Schedule(Functor functor,
                                        vtkm::Id numInstances)
  {
    const vtkm::Id MESSAGE_SIZE = 1024;
    char errorString[MESSAGE_SIZE];
    errorString[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer
        errorMessage(errorString, MESSAGE_SIZE);

    functor.SetErrorMessageBuffer(errorMessage);

    DeviceAdapterAlgorithm<Device>::ScheduleKernel<Functor> kernel(functor);

    std::for_each(
          ::boost::counting_iterator<vtkm::Id>(0),
          ::boost::counting_iterator<vtkm::Id>(numInstances),
          kernel);

    if (errorMessage.IsErrorRaised())
    {
      throw vtkm::cont::ErrorExecution(errorString);
    }
  }

  template<class FunctorType>
  VTKM_CONT_EXPORT
  static void Schedule(FunctorType functor, vtkm::Id3 rangeMax)
  {
    DeviceAdapterAlgorithm<Device>::Schedule(functor,
                                     rangeMax[0] * rangeMax[1] * rangeMax[2] );
  }

  template<typename T, class Container>
  VTKM_CONT_EXPORT static void Sort(vtkm::cont::ArrayHandle<T,Container>& values)
  {
    typedef typename vtkm::cont::ArrayHandle<T,Container>
        ::template ExecutionTypes<Device>::Portal PortalType;

    PortalType arrayPortal = values.PrepareForInPlace(Device());
    std::sort(arrayPortal.GetIteratorBegin(), arrayPortal.GetIteratorEnd());
  }

  template<typename T, class Container, class Compare>
  VTKM_CONT_EXPORT static void Sort(vtkm::cont::ArrayHandle<T,Container>& values,
                                    Compare comp)
  {
    typedef typename vtkm::cont::ArrayHandle<T,Container>
        ::template ExecutionTypes<Device>::Portal PortalType;

    PortalType arrayPortal = values.PrepareForInPlace(Device());
    std::sort(arrayPortal.GetIteratorBegin(), arrayPortal.GetIteratorEnd(),comp);
  }

  VTKM_CONT_EXPORT static void Synchronize()
  {
    // Nothing to do. This device is serial and has no asynchronous operations.
  }

};

}
} // namespace vtkm::cont

#endif //vtkm_cont_internal_DeviceAdapterAlgorithmSerial_h
