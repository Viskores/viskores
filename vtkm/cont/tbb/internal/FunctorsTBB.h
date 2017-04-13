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
#ifndef vtk_m_cont_tbb_internal_FunctorsTBB_h
#define vtk_m_cont_tbb_internal_FunctorsTBB_h

#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/Error.h>
#include <vtkm/cont/internal/FunctorsGeneral.h>
#include <vtkm/exec/internal/ErrorMessageBuffer.h>


VTKM_THIRDPARTY_PRE_INCLUDE

#if  defined(VTKM_MSVC)

// TBB's header include a #pragma comment(lib,"tbb.lib") line to make all
// consuming libraries link to tbb, this is bad behavior in a header
// based project
#pragma push_macro("__TBB_NO_IMPLICITLINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1

#endif // defined(VTKM_MSVC)

// TBB includes windows.h, so instead we want to include windows.h with the
// correct settings so that we don't clobber any existing function
#include <vtkm/internal/Windows.h>

#include <tbb/tbb_stddef.h>
#if (TBB_VERSION_MAJOR == 4) && (TBB_VERSION_MINOR == 2)
//we provide an patched implementation of tbb parallel_sort
//that fixes ADL for std::swap. This patch has been submitted to Intel
//and is fixed in TBB 4.2 update 2.
#include <vtkm/cont/tbb/internal/parallel_sort.h>
#else
#include <tbb/parallel_sort.h>
#endif

#include <numeric>
#include <tbb/blocked_range.h>
#include <tbb/blocked_range3d.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/parallel_scan.h>
#include <tbb/partitioner.h>
#include <tbb/tick_count.h>

#if defined(VTKM_MSVC)
#pragma pop_macro("__TBB_NO_IMPLICITLINKAGE")
#endif

VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace cont {
namespace tbb {

// The "grain size" of scheduling with TBB.  Not a lot of thought has gone
// into picking this size.
static const vtkm::Id TBB_GRAIN_SIZE = 1024;


template<class InputPortalType, class T, class BinaryOperationType>
struct ReduceBody
{
  T Sum;
  T InitialValue;
  bool FirstCall;
  InputPortalType InputPortal;
  BinaryOperationType BinaryOperation;

  VTKM_CONT
  ReduceBody(const InputPortalType &inputPortal,
             T initialValue,
             BinaryOperationType binaryOperation)
    : Sum(vtkm::TypeTraits<T>::ZeroInitialization()),
      InitialValue(initialValue),
      FirstCall(true),
      InputPortal(inputPortal),
      BinaryOperation(binaryOperation)
  {  }

  VTKM_EXEC_CONT
  ReduceBody(const ReduceBody &body, ::tbb::split)
    : Sum(vtkm::TypeTraits<T>::ZeroInitialization()),
      InitialValue(body.InitialValue),
      FirstCall(true),
      InputPortal(body.InputPortal),
      BinaryOperation(body.BinaryOperation) {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(const ::tbb::blocked_range<vtkm::Id> &range)
  {
    typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
      InputIteratorsType;
    InputIteratorsType inputIterators(this->InputPortal);

    //use temp, and iterators instead of member variable to reduce false sharing
    typename InputIteratorsType::IteratorType inIter =
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());

    T temp = this->BinaryOperation(*inIter, *(inIter+1));
    ++inIter; ++inIter;
    for (vtkm::Id index = range.begin()+2; index != range.end(); ++index, ++inIter)
      {
      temp = this->BinaryOperation(temp, *inIter);
      }

    //determine if we also have to add the initial value to temp
    if(range.begin() == 0)
    {
      temp = this->BinaryOperation(temp,this->InitialValue);
    }

    //Now we can save temp back to sum, taking into account if
    //this task has been called before, and the sum value needs
    //to also be reduced.
    if(this->FirstCall)
    {
      this->Sum = temp;
    }
    else
    {
      this->Sum = this->BinaryOperation(this->Sum,temp);
    }

    this->FirstCall = false;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void join(const ReduceBody &left)
  {
    // std::cout << "join" << std::endl;
    this->Sum = this->BinaryOperation(left.Sum, this->Sum);
  }
};

template<class InputPortalType, typename T, class BinaryOperationType>
VTKM_SUPPRESS_EXEC_WARNINGS
VTKM_CONT static
T ReducePortals(InputPortalType inputPortal,
                T initialValue,
                BinaryOperationType binaryOperation)
{
  typedef internal::WrappedBinaryOperator<T, BinaryOperationType>
      WrappedBinaryOp;

  WrappedBinaryOp wrappedBinaryOp(binaryOperation);
  ReduceBody<InputPortalType, T, WrappedBinaryOp>body(inputPortal,
                                                      initialValue,
                                                      wrappedBinaryOp);
  vtkm::Id arrayLength = inputPortal.GetNumberOfValues();

  if (arrayLength > 1)
  {
    ::tbb::blocked_range<vtkm::Id> range(0, arrayLength, TBB_GRAIN_SIZE);
    ::tbb::parallel_reduce( range, body );
    return body.Sum;
  }
  else if (arrayLength == 1)
  {
    //ReduceBody does not work with an array of size 1.
    return binaryOperation(initialValue, inputPortal.Get(0));
  }
  else // arrayLength == 0
  {
    // ReduceBody does not work with an array of size 0.
    return initialValue;
  }
}

template<class InputPortalType, class OutputPortalType,
    class BinaryOperationType>
struct ScanInclusiveBody
{
  using ValueType = typename std::remove_reference<
                        typename OutputPortalType::ValueType>::type;
  ValueType Sum;
  bool FirstCall;
  InputPortalType InputPortal;
  OutputPortalType OutputPortal;
  BinaryOperationType BinaryOperation;

  VTKM_CONT
  ScanInclusiveBody(const InputPortalType &inputPortal,
                    const OutputPortalType &outputPortal,
                    BinaryOperationType binaryOperation)
    : Sum( vtkm::TypeTraits<ValueType>::ZeroInitialization() ),
      FirstCall(true),
      InputPortal(inputPortal),
      OutputPortal(outputPortal),
      BinaryOperation(binaryOperation)
  {  }

  VTKM_EXEC_CONT
  ScanInclusiveBody(const ScanInclusiveBody &body, ::tbb::split)
    : Sum( vtkm::TypeTraits<ValueType>::ZeroInitialization() ),
      FirstCall(true),
      InputPortal(body.InputPortal),
      OutputPortal(body.OutputPortal),
      BinaryOperation(body.BinaryOperation) {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(const ::tbb::blocked_range<vtkm::Id> &range, ::tbb::pre_scan_tag)
  {
    typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
      InputIteratorsType;
    InputIteratorsType inputIterators(this->InputPortal);

    //use temp, and iterators instead of member variable to reduce false sharing
    typename InputIteratorsType::IteratorType inIter =
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());
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

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
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
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());
    typename OutputIteratorsType::IteratorType outIter =
      outputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());
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

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void reverse_join(const ScanInclusiveBody &left)
  {
    this->Sum = this->BinaryOperation(left.Sum, this->Sum);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void assign(const ScanInclusiveBody &src)
  {
    this->Sum = src.Sum;
  }
};


template<class InputPortalType, class OutputPortalType,
    class BinaryOperationType>
struct ScanExclusiveBody
{
  using ValueType = typename std::remove_reference<
                        typename OutputPortalType::ValueType>::type;

  ValueType Sum;
  bool FirstCall;
  InputPortalType InputPortal;
  OutputPortalType OutputPortal;
  BinaryOperationType BinaryOperation;

  VTKM_CONT
  ScanExclusiveBody(const InputPortalType &inputPortal,
                    const OutputPortalType &outputPortal,
                    BinaryOperationType binaryOperation,
                    const ValueType& initialValue)
    : Sum(initialValue),
      FirstCall(true),
      InputPortal(inputPortal),
      OutputPortal(outputPortal),
      BinaryOperation(binaryOperation)
  {  }

  VTKM_EXEC_CONT
  ScanExclusiveBody(const ScanExclusiveBody &body, ::tbb::split)
    : Sum(body.Sum),
      FirstCall(true),
      InputPortal(body.InputPortal),
      OutputPortal(body.OutputPortal),
      BinaryOperation(body.BinaryOperation)
  {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(const ::tbb::blocked_range<vtkm::Id> &range, ::tbb::pre_scan_tag)
  {
    typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
      InputIteratorsType;
    InputIteratorsType inputIterators(this->InputPortal);

    //move the iterator to the first item
    typename InputIteratorsType::IteratorType iter =
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());

    ValueType temp = *iter;
    ++iter;
    if(! (this->FirstCall && range.begin() > 0) )
      { temp = this->BinaryOperation(this->Sum, temp); }
    for (vtkm::Id index = range.begin()+1; index != range.end(); ++index, ++iter)
      {
      temp = this->BinaryOperation(temp, *iter);
      }
    this->Sum = temp;
    this->FirstCall = false;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
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
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());
    typename OutputIteratorsType::IteratorType outIter =
      outputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());

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
    this->FirstCall = false;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void reverse_join(const ScanExclusiveBody &left)
  {
    //The contract we have with TBB is that they will only join
    //two objects that have been scanned, or two objects which
    //haven't been scanned
    VTKM_ASSERT(left.FirstCall == this->FirstCall);
    if(!left.FirstCall && !this->FirstCall)
    {
      this->Sum = this->BinaryOperation(left.Sum, this->Sum);
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void assign(const ScanExclusiveBody &src)
  {
    this->Sum = src.Sum;
  }
};

template<class InputPortalType, class OutputPortalType,
    class BinaryOperationType>
VTKM_SUPPRESS_EXEC_WARNINGS
VTKM_CONT static
typename std::remove_reference<typename OutputPortalType::ValueType>::type
ScanInclusivePortals(InputPortalType inputPortal,
                     OutputPortalType outputPortal,
                     BinaryOperationType binaryOperation)
{
  using ValueType = typename std::remove_reference<
                        typename OutputPortalType::ValueType>::type;

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
VTKM_SUPPRESS_EXEC_WARNINGS
VTKM_CONT static
typename std::remove_reference<typename OutputPortalType::ValueType>::type
ScanExclusivePortals(InputPortalType inputPortal,
                     OutputPortalType outputPortal,
                     BinaryOperationType binaryOperation,
                     typename std::remove_reference<
                         typename OutputPortalType::ValueType>::type initialValue)
{
  using ValueType = typename std::remove_reference<
                        typename OutputPortalType::ValueType>::type;

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

template<class FunctorType>
class ScheduleKernel
{
public:
  VTKM_CONT ScheduleKernel(const FunctorType &functor)
    : Functor(functor)
  {  }

  VTKM_CONT void SetErrorMessageBuffer(
      const vtkm::exec::internal::ErrorMessageBuffer &errorMessage)
  {
    this->ErrorMessage = errorMessage;
    this->Functor.SetErrorMessageBuffer(errorMessage);
  }

  VTKM_CONT
  void operator()(const ::tbb::blocked_range<vtkm::Id> &range) const {
    // The TBB device adapter causes array classes to be shared between
    // control and execution environment. This means that it is possible for
    // an exception to be thrown even though this is typically not allowed.
    // Throwing an exception from here is bad because there are several
    // simultaneous threads running. Get around the problem by catching the
    // error and setting the message buffer as expected.
    try
      {
      const vtkm::Id start = range.begin();
      const vtkm::Id end = range.end();
VTKM_VECTORIZATION_PRE_LOOP
      for (vtkm::Id index = start; index != end; index++)
        {
VTKM_VECTORIZATION_IN_LOOP
        this->Functor(index);
        }
      }
    catch (vtkm::cont::Error &error)
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


template<class FunctorType>
class ScheduleKernelId3
{
public:
  VTKM_CONT ScheduleKernelId3(const FunctorType &functor)
    : Functor(functor)
    {  }

  VTKM_CONT void SetErrorMessageBuffer(
      const vtkm::exec::internal::ErrorMessageBuffer &errorMessage)
  {
    this->ErrorMessage = errorMessage;
    this->Functor.SetErrorMessageBuffer(errorMessage);
  }

  VTKM_CONT
  void operator()(const ::tbb::blocked_range3d<vtkm::Id> &range) const {
    try
      {
      const vtkm::Id kstart = range.pages().begin();
      const vtkm::Id kend = range.pages().end();
      const vtkm::Id jstart =range.rows().begin();
      const vtkm::Id jend = range.rows().end();
      const vtkm::Id istart =range.cols().begin();
      const vtkm::Id iend = range.cols().end();

      vtkm::Id3 index;
      for( vtkm::Id k=kstart; k!=kend; ++k)
        {
        index[2]=k;
        for( vtkm::Id j=jstart; j!=jend; ++j)
          {
          index[1]=j;
          for( vtkm::Id i=istart; i != iend; ++i)
            {
            index[0]=i;
            this->Functor(index);
            }
          }
        }
      }
    catch (vtkm::cont::Error &error)
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

template<typename InputPortalType,
         typename IndexPortalType,
         typename OutputPortalType>
class ScatterKernel
{
public:
  VTKM_CONT ScatterKernel(InputPortalType  inputPortal,
                                 IndexPortalType  indexPortal,
                                 OutputPortalType outputPortal)
    : ValuesPortal(inputPortal),
      IndexPortal(indexPortal),
      OutputPortal(outputPortal)
  {  }

  VTKM_CONT
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
VTKM_VECTORIZATION_PRE_LOOP
      for (vtkm::Id i = range.begin(); i < range.end(); i++)
        {
VTKM_VECTORIZATION_IN_LOOP
        OutputPortal.Set( i, ValuesPortal.Get(IndexPortal.Get(i)) );
        }
      }
    catch (vtkm::cont::Error &error)
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
VTKM_SUPPRESS_EXEC_WARNINGS
VTKM_CONT static void ScatterPortal(InputPortalType  inputPortal,
                                           IndexPortalType  indexPortal,
                                           OutputPortalType outputPortal)
{
  const vtkm::Id size = inputPortal.GetNumberOfValues();
  VTKM_ASSERT(size == indexPortal.GetNumberOfValues() );

  ScatterKernel<InputPortalType,
                IndexPortalType,
                OutputPortalType> scatter(inputPortal,
                                          indexPortal,
                                          outputPortal);

  ::tbb::blocked_range<vtkm::Id> range(0, size, TBB_GRAIN_SIZE);
  ::tbb::parallel_for(range, scatter);
}

}
}
}
#endif //vtk_m_cont_tbb_internal_FunctorsTBB_h
