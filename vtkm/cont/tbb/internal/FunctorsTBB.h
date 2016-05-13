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

#include <vtkm/Types.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/Error.h>
#include <vtkm/cont/internal/FunctorsGeneral.h>
#include <vtkm/exec/internal/ErrorMessageBuffer.h>


VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/type_traits/remove_reference.hpp>

// gcc || clang
#if  defined(_WIN32)
// TBB includes windows.h, which clobbers min and max functions so we
// define NOMINMAX to fix that problem. We also include WIN32_LEAN_AND_MEAN
// to reduce the number of macros and objects windows.h imports as those also
// can cause conflicts
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#include <tbb/tbb_stddef.h>
#if (TBB_VERSION_MAJOR == 4) && (TBB_VERSION_MINOR == 2)
//we provide an patched implementation of tbb parallel_sort
//that fixes ADL for std::swap. This patch has been submitted to Intel
//and is fixed in TBB 4.2 update 2.
#include <vtkm/cont/tbb/internal/parallel_sort.h>
#else
#include <tbb/parallel_sort.h>
#endif

#include <tbb/blocked_range.h>
#include <tbb/blocked_range3d.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_scan.h>
#include <tbb/partitioner.h>
#include <tbb/tick_count.h>

#if defined(_WIN32)
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif

VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace cont {
namespace tbb {

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

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_EXPORT
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
  VTKM_EXEC_CONT_EXPORT
  void reverse_join(const ScanInclusiveBody &left)
  {
    this->Sum = this->BinaryOperation(left.Sum, this->Sum);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  void assign(const ScanInclusiveBody &src)
  {
    this->Sum = src.Sum;
  }
};


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

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_EXPORT
  void operator()(const ::tbb::blocked_range<vtkm::Id> &range, ::tbb::pre_scan_tag)
  {
    typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
      InputIteratorsType;
    InputIteratorsType inputIterators(this->InputPortal);

    //move the iterator to the first item
    typename InputIteratorsType::IteratorType iter =
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());
    ValueType temp = this->Sum;
    for (vtkm::Id index = range.begin(); index != range.end(); ++index, ++iter)
      {
      temp = this->BinaryOperation(temp, *iter);
      }
    this->Sum = temp;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
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
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  void reverse_join(const ScanExclusiveBody &left)
  {
    this->Sum = this->BinaryOperation(left.Sum, this->Sum);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  void assign(const ScanExclusiveBody &src)
  {
    this->Sum = src.Sum;
  }
};

template<class InputPortalType, class OutputPortalType,
    class BinaryOperationType>
VTKM_SUPPRESS_EXEC_WARNINGS
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
VTKM_SUPPRESS_EXEC_WARNINGS
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

  VTKM_CONT_EXPORT
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

  VTKM_CONT_EXPORT
  void operator()(const ::tbb::blocked_range3d<vtkm::Id> &range) const {
    try
      {
      for( vtkm::Id k=range.pages().begin(); k!=range.pages().end(); ++k)
        {
        for( vtkm::Id j=range.rows().begin(); j!=range.rows().end(); ++j)
          {
          const vtkm::Id start =range.cols().begin();
          const vtkm::Id end = range.cols().end();
VTKM_VECTORIZATION_PRE_LOOP
          for( vtkm::Id i=start; i != end; ++i)
            {
VTKM_VECTORIZATION_IN_LOOP
            this->Functor(vtkm::Id3(i, j, k));
            }
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

  VTKM_CONT_EXPORT
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
VTKM_SUPPRESS_EXEC_WARNINGS
VTKM_CONT_EXPORT static void ScatterPortal(InputPortalType  inputPortal,
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
