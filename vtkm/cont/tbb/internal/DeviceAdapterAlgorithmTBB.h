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
#ifndef vtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
#define vtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h


#include <vtkm/cont/internal/IteratorFromArrayPortal.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>
#include <vtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>
#include <vtkm/exec/internal/ErrorMessageBuffer.h>
#include <vtkm/Extent.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>

#include <boost/type_traits/remove_reference.hpp>

// Disable warnings we check vtkm for but TBB does not.
#if defined(VTKM_GCC) || defined(VTKM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
// gcc || clang
#elif _WIN32
// TBB includes windows.h, which clobbers min and max functions so we
// define NOMINMAX to fix that problem. We also include WIN32_LEAN_AND_MEAN
// to reduce the number of macros and objects windows.h imports as those also
// can cause conflicts
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

//we provide an patched implementation of tbb parallel_sort
//that fixes ADL for std::swap. This patch has been submitted to Intel
//and should be included in future version of TBB.
#include <vtkm/cont/tbb/internal/parallel_sort.h>

#include <tbb/blocked_range.h>
#include <tbb/blocked_range3d.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_scan.h>
#include <tbb/partitioner.h>
#include <tbb/tick_count.h>


#if defined(VTKM_GCC) || defined(VTKM_CLANG)
#pragma GCC diagnostic pop
// gcc || clang
#elif _WIN32
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif

namespace vtkm {
namespace cont {

template<>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB> :
    vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
        DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>,
        vtkm::cont::DeviceAdapterTagTBB>
{
private:
  // The "grain size" of scheduling with TBB.  Not a lot of thought has gone
  // into picking this size.
  static const vtkm::Id TBB_GRAIN_SIZE = 4096;

  template<class InputPortalType, class OutputPortalType,
      class BinaryOperationType>
  struct ScanInclusiveBody
  {
    typedef typename boost::remove_reference<
        typename OutputPortalType::ValueType>::type ValueType;
    ValueType Sum;
    bool FirstCall;
    InputPortalType InputPortal;
    OutputPortalType OutputPortal;
    BinaryOperationType BinaryOperation;

    VTKM_CONT_EXPORT
    ScanInclusiveBody(const InputPortalType &inputPortal,
                      const OutputPortalType &outputPortal,
                      BinaryOperationType binaryOperation)
      : Sum( vtkm::TypeTraits<ValueType>::ZeroInitialization() ),
        FirstCall(true),
        InputPortal(inputPortal),
        OutputPortal(outputPortal),
        BinaryOperation(binaryOperation)
    {  }

    VTKM_EXEC_CONT_EXPORT
    ScanInclusiveBody(const ScanInclusiveBody &body, ::tbb::split)
      : Sum( vtkm::TypeTraits<ValueType>::ZeroInitialization() ),
        FirstCall(true),
        InputPortal(body.InputPortal),
        OutputPortal(body.OutputPortal),
        BinaryOperation(body.BinaryOperation) {  }

    VTKM_EXEC_EXPORT
    void operator()(const ::tbb::blocked_range<vtkm::Id> &range, ::tbb::pre_scan_tag)
    {
      typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
        InputIteratorsType;
      InputIteratorsType inputIterators(this->InputPortal);

      //use temp, and iterators instead of member variable to reduce false sharing
      typename InputIteratorsType::IteratorType inIter =
        inputIterators.GetBegin() + range.begin();
      ValueType temp = this->FirstCall ? *inIter++ :
                       this->BinaryOperation(this->Sum, *inIter++);
      this->FirstCall = false;
      for (vtkm::Id index = range.begin() + 1; index != range.end();
           ++index, ++inIter)
        {
        temp = this->BinaryOperation(temp, *inIter);
        }
      this->Sum = temp;
    }

    VTKM_EXEC_EXPORT
    void operator()(const ::tbb::blocked_range<vtkm::Id> &range, ::tbb::final_scan_tag)
    {
      typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
        InputIteratorsType;
      typedef vtkm::cont::ArrayPortalToIterators<OutputPortalType>
        OutputIteratorsType;

      InputIteratorsType inputIterators(this->InputPortal);
      OutputIteratorsType outputIterators(this->OutputPortal);

      //use temp, and iterators instead of member variable to reduce false sharing
      typename InputIteratorsType::IteratorType inIter =
        inputIterators.GetBegin() + range.begin();
      typename OutputIteratorsType::IteratorType outIter =
        outputIterators.GetBegin() + range.begin();
      ValueType temp = this->FirstCall ? *inIter++ :
                       this->BinaryOperation(this->Sum, *inIter++);
      this->FirstCall = false;
      *outIter++ = temp;
      for (vtkm::Id index = range.begin() + 1; index != range.end();
           ++index, ++inIter, ++outIter)
        {
        *outIter = temp = this->BinaryOperation(temp, *inIter);
        }
      this->Sum = temp;
    }

    VTKM_EXEC_CONT_EXPORT
    void reverse_join(const ScanInclusiveBody &left)
    {
      this->Sum = this->BinaryOperation(left.Sum, this->Sum);
    }

    VTKM_EXEC_CONT_EXPORT
    void assign(const ScanInclusiveBody &src)
    {
      this->Sum = src.Sum;
    }
  };

  template<class InputPortalType, class OutputPortalType,
      class BinaryOperationType>
  VTKM_CONT_EXPORT static
  typename boost::remove_reference<typename OutputPortalType::ValueType>::type
  ScanInclusivePortals(InputPortalType inputPortal,
                       OutputPortalType outputPortal,
                       BinaryOperationType binaryOperation)
  {
    typedef typename
        boost::remove_reference<typename OutputPortalType::ValueType>::type
        ValueType;
    typedef internal::WrappedBinaryOperator<ValueType, BinaryOperationType>
        WrappedBinaryOp;

    WrappedBinaryOp wrappedBinaryOp(binaryOperation);
    ScanInclusiveBody<InputPortalType, OutputPortalType, WrappedBinaryOp>
        body(inputPortal, outputPortal, wrappedBinaryOp);
    vtkm::Id arrayLength = inputPortal.GetNumberOfValues();

    ::tbb::blocked_range<vtkm::Id> range(0, arrayLength, TBB_GRAIN_SIZE);
    ::tbb::parallel_scan( range, body );
    return body.Sum;
  }

  template<class InputPortalType, class OutputPortalType,
      class BinaryOperationType>
  struct ScanExclusiveBody
  {
    typedef typename boost::remove_reference<
        typename OutputPortalType::ValueType>::type ValueType;
    ValueType Sum;
    ValueType InitialValue;
    InputPortalType InputPortal;
    OutputPortalType OutputPortal;
    BinaryOperationType BinaryOperation;

    VTKM_CONT_EXPORT
    ScanExclusiveBody(const InputPortalType &inputPortal,
                      const OutputPortalType &outputPortal,
                      BinaryOperationType binaryOperation,
                      const ValueType& initialValue)
      : Sum(initialValue),
        InitialValue(initialValue),
        InputPortal(inputPortal),
        OutputPortal(outputPortal),
        BinaryOperation(binaryOperation)
    {  }

    VTKM_EXEC_CONT_EXPORT
    ScanExclusiveBody(const ScanExclusiveBody &body, ::tbb::split)
      : Sum(body.InitialValue),
        InitialValue(body.InitialValue),
        InputPortal(body.InputPortal),
        OutputPortal(body.OutputPortal),
        BinaryOperation(body.BinaryOperation)
    {  }

    VTKM_EXEC_EXPORT
    void operator()(const ::tbb::blocked_range<vtkm::Id> &range, ::tbb::pre_scan_tag)
    {
      typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
        InputIteratorsType;
      InputIteratorsType inputIterators(this->InputPortal);

      //move the iterator to the first item
      typename InputIteratorsType::IteratorType iter =
        inputIterators.GetBegin() + range.begin();
      ValueType temp = this->Sum;
      for (vtkm::Id index = range.begin(); index != range.end(); ++index, ++iter)
        {
        temp = this->BinaryOperation(temp, *iter);
        }
      this->Sum = temp;
    }

    VTKM_EXEC_EXPORT
    void operator()(const ::tbb::blocked_range<vtkm::Id> &range, ::tbb::final_scan_tag)
    {
      typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
        InputIteratorsType;
      typedef vtkm::cont::ArrayPortalToIterators<OutputPortalType>
        OutputIteratorsType;

      InputIteratorsType inputIterators(this->InputPortal);
      OutputIteratorsType outputIterators(this->OutputPortal);

      //move the iterators to the first item
      typename InputIteratorsType::IteratorType inIter =
        inputIterators.GetBegin() + range.begin();
      typename OutputIteratorsType::IteratorType outIter =
        outputIterators.GetBegin() + range.begin();
      ValueType temp = this->Sum;
      for (vtkm::Id index = range.begin(); index != range.end();
           ++index, ++inIter, ++outIter)
        {
        //copy into a local reference since Input and Output portal
        //could point to the same memory location
        ValueType v = *inIter;
        *outIter = temp;
        temp = this->BinaryOperation(temp, v);
        }
      this->Sum = temp;
    }

    VTKM_EXEC_CONT_EXPORT
    void reverse_join(const ScanExclusiveBody &left)
    {
      this->Sum = this->BinaryOperation(left.Sum, this->Sum);
    }

    VTKM_EXEC_CONT_EXPORT
    void assign(const ScanExclusiveBody &src)
    {
      this->Sum = src.Sum;
    }
  };

  template<class InputPortalType, class OutputPortalType,
      class BinaryOperationType>
  VTKM_CONT_EXPORT static
  typename boost::remove_reference<typename OutputPortalType::ValueType>::type
  ScanExclusivePortals(InputPortalType inputPortal,
                       OutputPortalType outputPortal,
                       BinaryOperationType binaryOperation,
                       typename boost::remove_reference<
                           typename OutputPortalType::ValueType>::type initialValue)
  {
    typedef typename
        boost::remove_reference<typename OutputPortalType::ValueType>::type
        ValueType;
    typedef internal::WrappedBinaryOperator<ValueType, BinaryOperationType>
        WrappedBinaryOp;

    WrappedBinaryOp wrappedBinaryOp(binaryOperation);
    ScanExclusiveBody<InputPortalType, OutputPortalType, WrappedBinaryOp>
        body(inputPortal, outputPortal, wrappedBinaryOp, initialValue);
    vtkm::Id arrayLength = inputPortal.GetNumberOfValues();

    ::tbb::blocked_range<vtkm::Id> range(0, arrayLength, TBB_GRAIN_SIZE);
    ::tbb::parallel_scan( range, body );

    // Seems a little weird to me that we would return the last value in the
    // array rather than the sum, but that is how the function is specified.
    return body.Sum;
  }



public:
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut> &output)
  {
    return ScanInclusivePortals(
          input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
          output.PrepareForOutput(input.GetNumberOfValues(),
              vtkm::cont::DeviceAdapterTagTBB()), vtkm::internal::Add());
  }

  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut> &output,
      BinaryFunctor binary_functor)
  {
    return ScanInclusivePortals(
          input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
          output.PrepareForOutput(input.GetNumberOfValues(),
            vtkm::cont::DeviceAdapterTagTBB()), binary_functor);
  }

  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut> &output)
  {
    return ScanExclusivePortals(
          input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
          output.PrepareForOutput(input.GetNumberOfValues(),
            vtkm::cont::DeviceAdapterTagTBB()),
          vtkm::internal::Add(), vtkm::TypeTraits<T>::ZeroInitialization());
  }

  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut> &output,
      BinaryFunctor binary_functor,
      const T& initialValue)
  {
    return ScanExclusivePortals(
          input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
          output.PrepareForOutput(input.GetNumberOfValues(),
            vtkm::cont::DeviceAdapterTagTBB()), binary_functor, initialValue);
  }

private:
  template<class FunctorType>
  class ScheduleKernel
  {
  public:
    VTKM_CONT_EXPORT ScheduleKernel(const FunctorType &functor)
      : Functor(functor)
    {  }

    VTKM_CONT_EXPORT void SetErrorMessageBuffer(
        const vtkm::exec::internal::ErrorMessageBuffer &errorMessage)
    {
      this->ErrorMessage = errorMessage;
      this->Functor.SetErrorMessageBuffer(errorMessage);
    }

    VTKM_EXEC_EXPORT
    void operator()(const ::tbb::blocked_range<vtkm::Id> &range) const {
      // The TBB device adapter causes array classes to be shared between
      // control and execution environment. This means that it is possible for
      // an exception to be thrown even though this is typically not allowed.
      // Throwing an exception from here is bad because there are several
      // simultaneous threads running. Get around the problem by catching the
      // error and setting the message buffer as expected.
      try
        {
        for (vtkm::Id index = range.begin(); index < range.end(); index++)
          {
          this->Functor(index);
          }
        }
      catch (vtkm::cont::Error error)
        {
        this->ErrorMessage.RaiseError(error.GetMessage().c_str());
        }
      catch (...)
        {
        this->ErrorMessage.RaiseError(
            "Unexpected error in execution environment.");
        }
    }
  private:
    FunctorType Functor;
    vtkm::exec::internal::ErrorMessageBuffer ErrorMessage;
  };

public:
  template<class FunctorType>
  VTKM_CONT_EXPORT
  static void Schedule(FunctorType functor, vtkm::Id numInstances)
  {
    const vtkm::Id MESSAGE_SIZE = 1024;
    char errorString[MESSAGE_SIZE];
    errorString[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer
        errorMessage(errorString, MESSAGE_SIZE);

    ScheduleKernel<FunctorType> kernel(functor);
    kernel.SetErrorMessageBuffer(errorMessage);

    ::tbb::blocked_range<vtkm::Id> range(0, numInstances, TBB_GRAIN_SIZE);

    ::tbb::parallel_for(range, kernel);

    if (errorMessage.IsErrorRaised())
      {
      throw vtkm::cont::ErrorExecution(errorString);
      }
  }

private:
  template<class FunctorType>
  class ScheduleKernelId3
  {
  public:
    VTKM_CONT_EXPORT ScheduleKernelId3(const FunctorType &functor,
                                      const vtkm::Id3& dims)
      : Functor(functor),
        Dims(dims)
      {  }

    VTKM_CONT_EXPORT void SetErrorMessageBuffer(
        const vtkm::exec::internal::ErrorMessageBuffer &errorMessage)
    {
      this->ErrorMessage = errorMessage;
      this->Functor.SetErrorMessageBuffer(errorMessage);
    }

    VTKM_EXEC_EXPORT
    void operator()(const ::tbb::blocked_range3d<vtkm::Id> &range) const {
      try
        {
        for( vtkm::Id k=range.pages().begin(); k!=range.pages().end(); ++k)
          {
          vtkm::Id index = k * this->Dims[1] * this->Dims[0];
          index += range.rows().begin() * this->Dims[0];
          for( vtkm::Id j=range.rows().begin(); j!=range.rows().end(); ++j)
            {
            for( vtkm::Id i=range.cols().begin(); i!=range.cols().end(); ++i)
              {
              this->Functor(index + i);
              }
            index += this->Dims[0];
            }
          }
        }
      catch (vtkm::cont::Error error)
        {
        this->ErrorMessage.RaiseError(error.GetMessage().c_str());
        }
      catch (...)
        {
        this->ErrorMessage.RaiseError(
            "Unexpected error in execution environment.");
        }
    }
  private:
    FunctorType Functor;
    vtkm::Id3 Dims;
    vtkm::exec::internal::ErrorMessageBuffer ErrorMessage;
  };

public:
  template<class FunctorType>
  VTKM_CONT_EXPORT
  static void Schedule(FunctorType functor,
                       vtkm::Id3 rangeMax)
  {
    //we need to extract from the functor that uniform grid information
    const vtkm::Id MESSAGE_SIZE = 1024;
    char errorString[MESSAGE_SIZE];
    errorString[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer
        errorMessage(errorString, MESSAGE_SIZE);

    //memory is generally setup in a way that iterating the first range
    //in the tightest loop has the best cache coherence.
    ::tbb::blocked_range3d<vtkm::Id> range(0, rangeMax[2],
                                           0, rangeMax[1],
                                           0, rangeMax[0]);

    ScheduleKernelId3<FunctorType> kernel(functor,rangeMax);
    kernel.SetErrorMessageBuffer(errorMessage);

    ::tbb::parallel_for(range, kernel);

    if (errorMessage.IsErrorRaised())
      {
      throw vtkm::cont::ErrorExecution(errorString);
      }
  }

  template<typename T, class Container>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Container> &values)
  {
    //this is required to get sort to work with zip handles
    std::less< T > lessOp;
    Sort(values, lessOp );
  }

  template<typename T, class Container, class BinaryCompare>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Container> &values, BinaryCompare binary_compare)
  {
    typedef typename vtkm::cont::ArrayHandle<T,Container>::template
      ExecutionTypes<vtkm::cont::DeviceAdapterTagTBB>::Portal PortalType;
    PortalType arrayPortal = values.PrepareForInPlace(
      vtkm::cont::DeviceAdapterTagTBB());

    typedef vtkm::cont::ArrayPortalToIterators<PortalType> IteratorsType;
    IteratorsType iterators(arrayPortal);

    internal::WrappedBinaryOperator<bool,BinaryCompare> wrappedCompare(binary_compare);
    ::tbb::parallel_sort(iterators.GetBegin(),
                         iterators.GetEnd(),
                         wrappedCompare);
  }

private:

  template<typename InputPortalType,
           typename IndexPortalType,
           typename OutputPortalType>
  class ScatterKernel
  {
  public:
    VTKM_CONT_EXPORT ScatterKernel(InputPortalType  inputPortal,
                                   IndexPortalType  indexPortal,
                                   OutputPortalType outputPortal)
      : ValuesPortal(inputPortal),
        IndexPortal(indexPortal),
        OutputPortal(outputPortal)
    {  }

    VTKM_EXEC_EXPORT
    void operator()(const ::tbb::blocked_range<vtkm::Id> &range) const
    {
      // The TBB device adapter causes array classes to be shared between
      // control and execution environment. This means that it is possible for
      // an exception to be thrown even though this is typically not allowed.
      // Throwing an exception from here is bad because there are several
      // simultaneous threads running. Get around the problem by catching the
      // error and setting the message buffer as expected.
      try
        {
        for (vtkm::Id i = range.begin(); i < range.end(); i++)
          {
          OutputPortal.Set( i, ValuesPortal.Get(IndexPortal.Get(i)) );
          }
        }
      catch (vtkm::cont::Error error)
        {
        this->ErrorMessage.RaiseError(error.GetMessage().c_str());
        }
      catch (...)
        {
        this->ErrorMessage.RaiseError(
            "Unexpected error in execution environment.");
        }
    }
  private:
    InputPortalType ValuesPortal;
    IndexPortalType IndexPortal;
    OutputPortalType OutputPortal;
    vtkm::exec::internal::ErrorMessageBuffer ErrorMessage;
  };

  template<typename InputPortalType,
           typename IndexPortalType,
           typename OutputPortalType>
  VTKM_CONT_EXPORT static void ScatterPortal(InputPortalType  inputPortal,
                                             IndexPortalType  indexPortal,
                                             OutputPortalType outputPortal)
  {
    const vtkm::Id size = inputPortal.GetNumberOfValues();
    VTKM_ASSERT_CONT(size == indexPortal.GetNumberOfValues() );

    ScatterKernel<InputPortalType,
                  IndexPortalType,
                  OutputPortalType> scatter(inputPortal,
                                            indexPortal,
                                            outputPortal);

    ::tbb::blocked_range<vtkm::Id> range(0, size, TBB_GRAIN_SIZE);
    ::tbb::parallel_for(range, scatter);
  }

public:
  template<typename T, typename U, class StorageT,  class StorageU>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT> &keys,
      vtkm::cont::ArrayHandle<U,StorageU> &values)
  {
    SortByKey(keys, values, std::less<T>());
  }

  template<typename T, typename U,
           class StorageT, class StorageU,
           class Compare>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT>& keys,
      vtkm::cont::ArrayHandle<U,StorageU>& values,
      Compare comp)
  {
    typedef vtkm::cont::ArrayHandle<T,StorageT> KeyType;
    if (sizeof(U) > sizeof(vtkm::Id))
    {
      /// More efficient sort:
      /// Move value indexes when sorting and reorder the value array at last

      typedef vtkm::cont::ArrayHandle<U,StorageU> ValueType;
      typedef vtkm::cont::ArrayHandle<vtkm::Id,StorageU> IndexType;
      typedef vtkm::cont::ArrayHandleZip<KeyType,IndexType> ZipHandleType;

      IndexType indexArray;
      ValueType valuesScattered;
      const vtkm::Id size = values.GetNumberOfValues();

      Copy( make_ArrayHandleCounting(0, keys.GetNumberOfValues()), indexArray);

      ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys,indexArray);
      Sort(zipHandle,KeyCompare<T,vtkm::Id,Compare>(comp));


      ScatterPortal(values.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
                    indexArray.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
                    valuesScattered.PrepareForOutput(size,vtkm::cont::DeviceAdapterTagTBB()));

      Copy( valuesScattered, values );
    }
    else
    {
      typedef vtkm::cont::ArrayHandle<U,StorageU> ValueType;
      typedef vtkm::cont::ArrayHandleZip<KeyType,ValueType> ZipHandleType;

      ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys,values);
      Sort(zipHandle,KeyCompare<T,U,Compare>(comp));
    }
  }

  VTKM_CONT_EXPORT static void Synchronize()
  {
    // Nothing to do. This device schedules all of its operations using a
    // split/join paradigm. This means that the if the control threaad is
    // calling this method, then nothing should be running in the execution
    // environment.
  }

};

/// TBB contains its own high resolution timer.
///
template<>
class DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagTBB>
{
public:
  VTKM_CONT_EXPORT DeviceAdapterTimerImplementation()
  {
    this->Reset();
  }
  VTKM_CONT_EXPORT void Reset()
  {
    vtkm::cont::DeviceAdapterAlgorithm<
        vtkm::cont::DeviceAdapterTagTBB>::Synchronize();
    this->StartTime = ::tbb::tick_count::now();
  }
  VTKM_CONT_EXPORT vtkm::Float64 GetElapsedTime()
  {
    vtkm::cont::DeviceAdapterAlgorithm<
        vtkm::cont::DeviceAdapterTagTBB>::Synchronize();
    ::tbb::tick_count currentTime = ::tbb::tick_count::now();
    ::tbb::tick_count::interval_t elapsedTime = currentTime - this->StartTime;
    return static_cast<vtkm::Float64>(elapsedTime.seconds());
  }

private:
  ::tbb::tick_count StartTime;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
