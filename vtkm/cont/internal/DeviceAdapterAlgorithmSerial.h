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
#ifndef vtk_m_cont_internal_DeviceAdapterAlgorithmSerial_h
#define vtk_m_cont_internal_DeviceAdapterAlgorithmSerial_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <vtkm/cont/internal/DeviceAdapterTagSerial.h>

#include <vtkm/BinaryOperators.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/iterator/counting_iterator.hpp>
#include <boost/utility/enable_if.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

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

 template<typename T, class CIn>
  VTKM_CONT_EXPORT static T Reduce(
      const vtkm::cont::ArrayHandle<T,CIn> &input, T initialValue)
  {
    return Reduce(input, initialValue, vtkm::internal::Add());
  }

 template<typename T, class CIn, class BinaryFunctor>
  VTKM_CONT_EXPORT static T Reduce(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      T initialValue,
      BinaryFunctor binary_functor)
  {
    typedef typename vtkm::cont::ArrayHandle<T,CIn>
        ::template ExecutionTypes<Device>::PortalConst PortalIn;

    internal::WrappedBinaryOperator<T, BinaryFunctor> wrappedOp( binary_functor );
    PortalIn inputPortal = input.PrepareForInput(Device());
    return std::accumulate(vtkm::cont::ArrayPortalToIteratorBegin(inputPortal),
                           vtkm::cont::ArrayPortalToIteratorEnd(inputPortal),
                           initialValue,
                           wrappedOp);
  }

  template<typename T, typename U, class KIn, class VIn, class KOut, class VOut,
          class BinaryFunctor>
  VTKM_CONT_EXPORT static void ReduceByKey(
      const vtkm::cont::ArrayHandle<T,KIn> &keys,
      const vtkm::cont::ArrayHandle<U,VIn> &values,
      vtkm::cont::ArrayHandle<T,KOut> &keys_output,
      vtkm::cont::ArrayHandle<U,VOut> &values_output,
      BinaryFunctor binary_functor)
  {
    typedef typename vtkm::cont::ArrayHandle<T,KIn>
        ::template ExecutionTypes<Device>::PortalConst PortalKIn;
    typedef typename vtkm::cont::ArrayHandle<U,VIn>
        ::template ExecutionTypes<Device>::PortalConst PortalVIn;

    typedef typename vtkm::cont::ArrayHandle<T,KOut>
        ::template ExecutionTypes<Device>::Portal PortalKOut;
    typedef typename vtkm::cont::ArrayHandle<U,VOut>
        ::template ExecutionTypes<Device>::Portal PortalVOut;

    PortalKIn keysPortalIn = keys.PrepareForInput(Device());
    PortalVIn valuesPortalIn = values.PrepareForInput(Device());

    const vtkm::Id numberOfKeys = keys.GetNumberOfValues();
    PortalKOut keysPortalOut = keys_output.PrepareForOutput(numberOfKeys, Device());
    PortalVOut valuesPortalOut = values_output.PrepareForOutput(numberOfKeys, Device());

    vtkm::Id writePos = 0;
    vtkm::Id readPos = 0;

    T currentKey = keysPortalIn.Get(readPos);
    U currentValue = valuesPortalIn.Get(readPos);

    for(++readPos; readPos < numberOfKeys; ++readPos)
      {
      while(readPos < numberOfKeys &&
            currentKey == keysPortalIn.Get(readPos) )
        {
        currentValue = binary_functor(currentValue, valuesPortalIn.Get(readPos));
        ++readPos;
        }

      if(readPos < numberOfKeys)
        {
        keysPortalOut.Set(writePos, currentKey);
        valuesPortalOut.Set(writePos, currentValue);
        ++writePos;

        currentKey = keysPortalIn.Get(readPos);
        currentValue = valuesPortalIn.Get(readPos);
        }
      }

    //now write out the last set of values
    keysPortalOut.Set(writePos, currentKey);
    valuesPortalOut.Set(writePos, currentValue);

    //now we need to shrink to the correct number of keys/values
    //writePos is zero-based so add 1 to get correct length
    keys_output.Shrink( writePos + 1  );
    values_output.Shrink( writePos + 1 );
  }

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

    if (numberOfValues <= 0) { return vtkm::TypeTraits<T>::ZeroInitialization(); }

    std::partial_sum(vtkm::cont::ArrayPortalToIteratorBegin(inputPortal),
                     vtkm::cont::ArrayPortalToIteratorEnd(inputPortal),
                     vtkm::cont::ArrayPortalToIteratorBegin(outputPortal));

    // Return the value at the last index in the array, which is the full sum.
    return outputPortal.Get(numberOfValues - 1);
  }

  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output,
      BinaryFunctor binary_functor)
  {
    typedef typename vtkm::cont::ArrayHandle<T,COut>
        ::template ExecutionTypes<Device>::Portal PortalOut;
    typedef typename vtkm::cont::ArrayHandle<T,CIn>
        ::template ExecutionTypes<Device>::PortalConst PortalIn;

    internal::WrappedBinaryOperator<T,BinaryFunctor> wrappedBinaryOp(
                                                               binary_functor);

    vtkm::Id numberOfValues = input.GetNumberOfValues();

    PortalIn inputPortal = input.PrepareForInput(Device());
    PortalOut outputPortal = output.PrepareForOutput(numberOfValues, Device());

    if (numberOfValues <= 0) { return vtkm::TypeTraits<T>::ZeroInitialization(); }

    std::partial_sum(vtkm::cont::ArrayPortalToIteratorBegin(inputPortal),
                     vtkm::cont::ArrayPortalToIteratorEnd(inputPortal),
                     vtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
                     wrappedBinaryOp);

    // Return the value at the last index in the array, which is the full sum.
    return outputPortal.Get(numberOfValues - 1);
  }

  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output,
      BinaryFunctor binaryFunctor,
      const T& initialValue)
  {
    typedef typename vtkm::cont::ArrayHandle<T,COut>
        ::template ExecutionTypes<Device>::Portal PortalOut;
    typedef typename vtkm::cont::ArrayHandle<T,CIn>
        ::template ExecutionTypes<Device>::PortalConst PortalIn;

    internal::WrappedBinaryOperator<T, BinaryFunctor>
        wrappedBinaryOp(binaryFunctor);

    vtkm::Id numberOfValues = input.GetNumberOfValues();

    PortalIn inputPortal = input.PrepareForInput(Device());
    PortalOut outputPortal = output.PrepareForOutput(numberOfValues, Device());

    if (numberOfValues <= 0) { return initialValue; }

    // Shift right by one, by iterating backwards. We are required to iterate
    //backwards so that the algorithm works correctly when the input and output
    //are the same array, otherwise you just propagate the first element
    //to all elements
    //Note: We explicitly do not use std::copy_backwards for good reason.
    //The ICC compiler has been found to improperly optimize the copy_backwards
    //into a standard copy, causing the above issue.
    T lastValue = inputPortal.Get(numberOfValues - 1);
    for(vtkm::Id i=(numberOfValues-1); i >= 1; --i)
      {
      outputPortal.Set(i, inputPortal.Get(i-1));
      }
    outputPortal.Set(0, initialValue);

    std::partial_sum(vtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
                     vtkm::cont::ArrayPortalToIteratorEnd(outputPortal),
                     vtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
                     wrappedBinaryOp);

    return wrappedBinaryOp(outputPortal.Get(numberOfValues - 1), lastValue);
  }

  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    return ScanExclusive(input, output, vtkm::Sum(),
                         vtkm::TypeTraits<T>::ZeroInitialization());
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
    template<typename T>
    VTKM_EXEC_EXPORT void operator()(const T& index) const
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

    const vtkm::Id size = numInstances;
    for(vtkm::Id i=0; i < size; ++i)
      {
      kernel(i);
      }

    if (errorMessage.IsErrorRaised())
    {
      throw vtkm::cont::ErrorExecution(errorString);
    }
  }

public:
  template<class Functor>
  VTKM_CONT_EXPORT
  static void Schedule(Functor functor, vtkm::Id3 rangeMax)
  {
    const vtkm::Id MESSAGE_SIZE = 1024;
    char errorString[MESSAGE_SIZE];
    errorString[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer
        errorMessage(errorString, MESSAGE_SIZE);

    functor.SetErrorMessageBuffer(errorMessage);

    DeviceAdapterAlgorithm<Device>::ScheduleKernel<Functor> kernel(functor);

    vtkm::Id3 index;
    for(vtkm::Id k=0; k < rangeMax[2]; ++k)
      {
      index[2] = k;
      for(vtkm::Id j=0; j < rangeMax[1]; ++j)
        {
        index[1] = j;
        for(vtkm::Id i=0; i < rangeMax[0]; ++i)
          {
          index[0] = i;
          kernel( index );
          }
        }
      }

    if (errorMessage.IsErrorRaised())
    {
      throw vtkm::cont::ErrorExecution(errorString);
    }
  }

private:
  template<typename Vin, typename I, typename Vout, class StorageVin,  class StorageI, class StorageVout>
  VTKM_CONT_EXPORT static void Scatter(
      vtkm::cont::ArrayHandle<Vin,StorageVin> &values,
      vtkm::cont::ArrayHandle<I,StorageI> &index,
      vtkm::cont::ArrayHandle<Vout,StorageVout> &values_out
          )
  {
    typedef typename vtkm::cont::ArrayHandle<Vin,StorageVin>
        ::template ExecutionTypes<Device>::PortalConst PortalVIn;
    typedef typename vtkm::cont::ArrayHandle<I,StorageI>
        ::template ExecutionTypes<Device>::PortalConst PortalI;
    typedef typename vtkm::cont::ArrayHandle<Vout,StorageVout>
        ::template ExecutionTypes<Device>::Portal PortalVout;

    const vtkm::Id n = values.GetNumberOfValues();
    VTKM_ASSERT_CONT(n == index.GetNumberOfValues() );

    PortalVIn valuesPortal = values.PrepareForInput(Device());
    PortalI indexPortal = index.PrepareForInput(Device());
    PortalVout valuesOutPortal = values_out.PrepareForOutput(n, Device());

    for (vtkm::Id i=0; i<n; i++)
    {
       valuesOutPortal.Set( i, valuesPortal.Get(indexPortal.Get(i)) );
    }
  }

private:
  /// Reorder the value array along with the sorting algorithm
  template<typename T, typename U, class StorageT,  class StorageU, class BinaryCompare>
  VTKM_CONT_EXPORT static void SortByKeyDirect(
      vtkm::cont::ArrayHandle<T,StorageT> &keys,
      vtkm::cont::ArrayHandle<U,StorageU> &values,
      BinaryCompare binary_compare)
  {
    //combine the keys and values into a ZipArrayHandle
    //we than need to specify a custom compare function wrapper
    //that only checks for key side of the pair, using the custom compare
    //functor that the user passed in
    typedef vtkm::cont::ArrayHandle<T,StorageT> KeyType;
    typedef vtkm::cont::ArrayHandle<U,StorageU> ValueType;
    typedef vtkm::cont::ArrayHandleZip<KeyType,ValueType> ZipHandleType;

    ZipHandleType zipHandle =
                    vtkm::cont::make_ArrayHandleZip(keys,values);
    Sort(zipHandle,internal::KeyCompare<T,U,BinaryCompare>(binary_compare));
  }

public:
  template<typename T, typename U, class StorageT,  class StorageU>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT> &keys,
      vtkm::cont::ArrayHandle<U,StorageU> &values)
  {
    SortByKey(keys, values, std::less<T>());
  }

  template<typename T, typename U, class StorageT,  class StorageU, class BinaryCompare>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT> &keys,
      vtkm::cont::ArrayHandle<U,StorageU> &values,
      const BinaryCompare &binary_compare)
  {
    internal::WrappedBinaryOperator<bool, BinaryCompare > wrappedCompare( binary_compare );
    if (sizeof(U) > sizeof(vtkm::Id))
    {
      /// More efficient sort:
      /// Move value indexes when sorting and reorder the value array at last
      typedef vtkm::cont::ArrayHandle<U,StorageU> ValueType;
      typedef vtkm::cont::ArrayHandle<vtkm::Id,StorageU> IndexType;

      IndexType indexArray;
      ValueType valuesScattered;

      Copy( ArrayHandleIndex(keys.GetNumberOfValues()), indexArray);
      SortByKeyDirect(keys, indexArray, wrappedCompare);
      Scatter(values, indexArray, valuesScattered);
      Copy( valuesScattered, values );
    }
    else
    {
      SortByKeyDirect(keys, values, wrappedCompare);
    }
  }

  template<typename T, class Storage>
  VTKM_CONT_EXPORT static void Sort(vtkm::cont::ArrayHandle<T,Storage>& values)
  {
    Sort(values, std::less<T>());
  }

  template<typename T, class Storage, class BinaryCompare>
  VTKM_CONT_EXPORT static void Sort(vtkm::cont::ArrayHandle<T,Storage>& values,
                                    BinaryCompare binary_compare)
  {
    typedef typename vtkm::cont::ArrayHandle<T,Storage>
        ::template ExecutionTypes<Device>::Portal PortalType;

    PortalType arrayPortal = values.PrepareForInPlace(Device());
    vtkm::cont::ArrayPortalToIterators<PortalType> iterators(arrayPortal);


    internal::WrappedBinaryOperator<bool,BinaryCompare> wrappedCompare(binary_compare);
    std::sort(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
  }

  VTKM_CONT_EXPORT static void Synchronize()
  {
    // Nothing to do. This device is serial and has no asynchronous operations.
  }

};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_internal_DeviceAdapterAlgorithmSerial_h
