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
#ifndef vtk_m_cont_internal_DeviceAdapterAlgorithmGeneral_h
#define vtk_m_cont_internal_DeviceAdapterAlgorithmGeneral_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>

#include <algorithm>

namespace vtkm {
namespace cont {
namespace internal {

// Binary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// IteratorFromArrayPortalValue which happen when passed an input array that
// is implicit.
template<typename ResultType, typename Function>
  struct WrappedBinaryOperator
{
  Function m_f;

 VTKM_CONT_EXPORT
  WrappedBinaryOperator(const Function &f)
    : m_f(f)
  {}

  template<typename Argument1, typename Argument2>
   VTKM_CONT_EXPORT ResultType operator()(const Argument1 &x, const Argument2 &y) const
  {
    return m_f(x, y);
  }

  template<typename Argument1, typename Argument2>
   VTKM_CONT_EXPORT ResultType operator()(
    const detail::IteratorFromArrayPortalValue<Argument1> &x,
    const detail::IteratorFromArrayPortalValue<Argument2> &y) const
  {
    typedef typename detail::IteratorFromArrayPortalValue<Argument1>::ValueType
                            ValueTypeX;
    typedef typename detail::IteratorFromArrayPortalValue<Argument2>::ValueType
                            ValueTypeY;
    return m_f( (ValueTypeX)x, (ValueTypeY)y );
  }

  template<typename Argument1, typename Argument2>
   VTKM_CONT_EXPORT ResultType operator()(
    const Argument1 &x,
    const detail::IteratorFromArrayPortalValue<Argument2> &y) const
  {
    typedef typename detail::IteratorFromArrayPortalValue<Argument2>::ValueType
                            ValueTypeY;
    return m_f( x, (ValueTypeY)y );
  }

  template<typename Argument1, typename Argument2>
   VTKM_CONT_EXPORT ResultType operator()(
    const detail::IteratorFromArrayPortalValue<Argument1> &x,
    const Argument2 &y) const
  {
    typedef typename detail::IteratorFromArrayPortalValue<Argument1>::ValueType
                            ValueTypeX;
    return m_f( (ValueTypeX)x, y );
  }

};

/// \brief
///
/// This struct provides algorithms that implement "general" device adapter
/// algorithms. If a device adapter provides implementations for Schedule,
/// and Synchronize, the rest of the algorithms can be implemented by calling
/// these functions.
///
/// It should be noted that we recommend that you also implement Sort,
/// ScanInclusive, and ScanExclusive for improved performance.
///
/// An easy way to implement the DeviceAdapterAlgorithm specialization is to
/// subclass this and override the implementation of methods as necessary.
/// As an example, the code would look something like this.
///
/// \code{.cpp}
/// template<>
/// struct DeviceAdapterAlgorithm<DeviceAdapterTagFoo>
///    : DeviceAdapterAlgorithmGeneral<DeviceAdapterAlgorithm<DeviceAdapterTagFoo>,
///                                    DeviceAdapterTagFoo>
/// {
///   template<class Functor>
///   VTKM_CONT_EXPORT static void Schedule(Functor functor,
///                                        vtkm::Id numInstances)
///   {
///     ...
///   }
///
///   template<class Functor>
///   VTKM_CONT_EXPORT static void Schedule(Functor functor,
///                                        vtkm::Id3 maxRange)
///   {
///     ...
///   }
///
///   VTKM_CONT_EXPORT static void Synchronize()
///   {
///     ...
///   }
/// };
/// \endcode
///
/// You might note that DeviceAdapterAlgorithmGeneral has two template
/// parameters that are redundant. Although the first parameter, the class for
/// the actual DeviceAdapterAlgorithm class containing Schedule, and
/// Synchronize is the same as DeviceAdapterAlgorithm<DeviceAdapterTag>, it is
/// made a separate template parameter to avoid a recursive dependence between
/// DeviceAdapterAlgorithmGeneral.h and DeviceAdapterAlgorithm.h
///
template<class DerivedAlgorithm, class DeviceAdapterTag>
struct DeviceAdapterAlgorithmGeneral
{
  //--------------------------------------------------------------------------
  // Get Execution Value
  // This method is used internally to get a single element from the execution
  // array. Might want to expose this and/or allow actual device adapter
  // implementations to provide one.
private:
  template<typename T, class CIn>
  VTKM_CONT_EXPORT
  static T GetExecutionValue(const vtkm::cont::ArrayHandle<T, CIn> &input,
                             vtkm::Id index)
  {
    typedef vtkm::cont::ArrayHandle<T,CIn> InputArrayType;
    typedef vtkm::cont::ArrayHandle<T,vtkm::cont::StorageTagBasic>
        OutputArrayType;

    OutputArrayType output;

    CopyKernel<
        typename InputArrayType::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename OutputArrayType::template ExecutionTypes<DeviceAdapterTag>::Portal>
        kernel(input.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(1, DeviceAdapterTag()),
               index);

    DerivedAlgorithm::Schedule(kernel, 1);

    return output.GetPortalConstControl().Get(0);
  }

  //--------------------------------------------------------------------------
  // Copy
private:
  template<class InputPortalType, class OutputPortalType>
  struct CopyKernel
  {
    InputPortalType InputPortal;
    OutputPortalType OutputPortal;
    vtkm::Id InputOffset;
    vtkm::Id OutputOffset;

    VTKM_CONT_EXPORT
    CopyKernel(InputPortalType inputPortal,
               OutputPortalType outputPortal,
               vtkm::Id inputOffset = 0,
               vtkm::Id outputOffset = 0)
      : InputPortal(inputPortal),
        OutputPortal(outputPortal),
        InputOffset(inputOffset),
        OutputOffset(outputOffset)
    {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      this->OutputPortal.Set(
        index + this->OutputOffset,
        this->InputPortal.Get(index + this->InputOffset));
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };

public:
  template<typename T, typename U, class CIn, class COut>
  VTKM_CONT_EXPORT static void Copy(const vtkm::cont::ArrayHandle<T, CIn> &input,
                                    vtkm::cont::ArrayHandle<U, COut> &output)
  {
    vtkm::Id arraySize = input.GetNumberOfValues();

    CopyKernel<
        typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<U,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal>
        kernel(input.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(arraySize, DeviceAdapterTag()));

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  //--------------------------------------------------------------------------
  // Lower Bounds
private:
  template<class InputPortalType,class ValuesPortalType,class OutputPortalType>
  struct LowerBoundsKernel
  {
    InputPortalType InputPortal;
    ValuesPortalType ValuesPortal;
    OutputPortalType OutputPortal;

    VTKM_CONT_EXPORT
    LowerBoundsKernel(InputPortalType inputPortal,
                      ValuesPortalType valuesPortal,
                      OutputPortalType outputPortal)
      : InputPortal(inputPortal),
        ValuesPortal(valuesPortal),
        OutputPortal(outputPortal) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      // This method assumes that (1) InputPortalType can return working
      // iterators in the execution environment and that (2) methods not
      // specified with VTKM_EXEC_EXPORT (such as the STL algorithms) can be
      // called from the execution environment. Neither one of these is
      // necessarily true, but it is true for the current uses of this general
      // function and I don't want to compete with STL if I don't have to.

      typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
          InputIteratorsType;
      InputIteratorsType inputIterators(this->InputPortal);
      typename InputIteratorsType::IteratorType resultPos =
          std::lower_bound(inputIterators.GetBegin(),
                           inputIterators.GetEnd(),
                           this->ValuesPortal.Get(index));

      vtkm::Id resultIndex =
          static_cast<vtkm::Id>(
            std::distance(inputIterators.GetBegin(), resultPos));
      this->OutputPortal.Set(index, resultIndex);
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };

  template<class InputPortalType,class ValuesPortalType,class OutputPortalType,class Compare>
  struct LowerBoundsComparisonKernel
  {
    InputPortalType InputPortal;
    ValuesPortalType ValuesPortal;
    OutputPortalType OutputPortal;
    Compare CompareFunctor;

    VTKM_CONT_EXPORT
    LowerBoundsComparisonKernel(InputPortalType inputPortal,
                                ValuesPortalType valuesPortal,
                                OutputPortalType outputPortal,
                                Compare comp)
      : InputPortal(inputPortal),
        ValuesPortal(valuesPortal),
        OutputPortal(outputPortal),
        CompareFunctor(comp) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      // This method assumes that (1) InputPortalType can return working
      // iterators in the execution environment and that (2) methods not
      // specified with VTKM_EXEC_EXPORT (such as the STL algorithms) can be
      // called from the execution environment. Neither one of these is
      // necessarily true, but it is true for the current uses of this general
      // function and I don't want to compete with STL if I don't have to.

      typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
          InputIteratorsType;
      InputIteratorsType inputIterators(this->InputPortal);
      typename InputIteratorsType::IteratorType resultPos =
          std::lower_bound(inputIterators.GetBegin(),
                           inputIterators.GetEnd(),
                           this->ValuesPortal.Get(index),
                           this->CompareFunctor);

      vtkm::Id resultIndex =
          static_cast<vtkm::Id>(
            std::distance(inputIterators.GetBegin(), resultPos));
      this->OutputPortal.Set(index, resultIndex);
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };


public:
  template<typename T, class CIn, class CVal, class COut>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      const vtkm::cont::ArrayHandle<T,CVal> &values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &output)
  {
    vtkm::Id arraySize = values.GetNumberOfValues();

    LowerBoundsKernel<
        typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<T,CVal>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<vtkm::Id,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal>
        kernel(input.PrepareForInput(DeviceAdapterTag()),
               values.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(arraySize, DeviceAdapterTag()));

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template<typename T, class CIn, class CVal, class COut, class Compare>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      const vtkm::cont::ArrayHandle<T,CVal> &values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &output,
      Compare comp)
  {
    vtkm::Id arraySize = values.GetNumberOfValues();

    LowerBoundsComparisonKernel<
        typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<T,CVal>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<vtkm::Id,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal,
        Compare>
        kernel(input.PrepareForInput(DeviceAdapterTag()),
               values.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(arraySize, DeviceAdapterTag()),
               comp);

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template<class CIn, class COut>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<vtkm::Id,CIn> &input,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &values_output)
  {
    DeviceAdapterAlgorithmGeneral<
        DerivedAlgorithm,DeviceAdapterTag>::LowerBounds(input,
                                                        values_output,
                                                        values_output);
  }

  //--------------------------------------------------------------------------
  // Reduce
private:
  template<int ReduceWidth, typename T, typename ArrayType, typename BinaryOperation >
  struct ReduceKernel : vtkm::exec::FunctorBase
  {
    typedef typename ArrayType::template ExecutionTypes<
                            DeviceAdapterTag> ExecutionTypes;
    typedef typename ExecutionTypes::PortalConst PortalConst;

    PortalConst Portal;
    BinaryOperation BinaryOperator;
    vtkm::Id ArrayLength;

    VTKM_CONT_EXPORT
    ReduceKernel()
    : Portal(),
      BinaryOperator(),
      ArrayLength(0)
    {
    }

    VTKM_CONT_EXPORT
    ReduceKernel(const ArrayType &array, BinaryOperation op)
      : Portal(array.PrepareForInput( DeviceAdapterTag() ) ),
        BinaryOperator(op),
        ArrayLength( array.GetNumberOfValues() )
    {  }

    VTKM_EXEC_EXPORT
    T operator()(vtkm::Id index) const
    {
      const vtkm::Id offset = index * ReduceWidth;

      //at least the first value access to the portal will be valid
      //only the rest could be invalid
      T partialSum = this->Portal.Get( offset );

      if( offset + ReduceWidth >= this->ArrayLength )
        {
        vtkm::Id currentIndex = offset + 1;
        while( currentIndex < this->ArrayLength)
          {
          partialSum = BinaryOperator(partialSum, this->Portal.Get(currentIndex));
          ++currentIndex;
          }
        }
      else
        {
        //optimize the usecase where all values are valid and we don't
        //need to check that we might go out of bounds
        for(int i=1; i < ReduceWidth; ++i)
          {
          partialSum = BinaryOperator(partialSum,
                                      this->Portal.Get( offset + i )
                                      );
          }
        }
      return partialSum;
    }
  };

  //--------------------------------------------------------------------------
  // Reduce
public:
 template<typename T, class CIn>
  VTKM_CONT_EXPORT static T Reduce(
      const vtkm::cont::ArrayHandle<T,CIn> &input, T initialValue)
  {
    return DerivedAlgorithm::Reduce(input, initialValue, vtkm::internal::Add());
  }

 template<typename T, class CIn, class BinaryOperator>
  VTKM_CONT_EXPORT static T Reduce(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      T initialValue,
      BinaryOperator binaryOp)
  {
    //Crazy Idea:
    //We create a implicit array handle that wraps the input
    //array handle. The implicit functor is passed the input array handle, and
    //the number of elements it needs to sum. This way the implicit handle
    //acts as the first level reduction. Say for example reducing 16 values
    //at a time.
    //
    //Now that we have an implicit array that is 1/16 the length of full array
    //we can use scan inclusive to compute the final sum
    typedef ReduceKernel<
            16,
            T,
            vtkm::cont::ArrayHandle<T,CIn>,
            BinaryOperator
            > ReduceKernelType;

    typedef vtkm::cont::ArrayHandleImplicit<
                                            T,
                                            ReduceKernelType > ReduceHandleType;
    typedef vtkm::cont::ArrayHandle<
                                    T,
                                    vtkm::cont::StorageTagBasic> TempArrayType;

    ReduceKernelType kernel(input, binaryOp);
    vtkm::Id length = (input.GetNumberOfValues() / 16);
    length += (input.GetNumberOfValues() % 16 == 0) ? 0 : 1;
    ReduceHandleType reduced = vtkm::cont::make_ArrayHandleImplicit<T>(kernel,
                                                                       length);

    TempArrayType inclusiveScanStorage;
    T scanResult = DerivedAlgorithm::ScanInclusive(reduced,
                                                   inclusiveScanStorage,
                                                   binaryOp);
    return binaryOp(initialValue, scanResult);
  }

  //--------------------------------------------------------------------------
  // Reduce By Key
private:

  struct ReduceKeySeriesStates
  {
    //It is needed that END and START_AND_END are both odd numbers
    //so that the first bit of both are 1
    enum { MIDDLE=0, END=1, START=2, START_AND_END=3};
  };

  template<typename InputPortalType>
  struct ReduceStencilGeneration : vtkm::exec::FunctorBase
  {
    typedef typename vtkm::cont::ArrayHandle< vtkm::UInt8 >::template ExecutionTypes<DeviceAdapterTag>
        ::Portal KeyStatePortalType;

    InputPortalType Input;
    KeyStatePortalType KeyState;

    VTKM_CONT_EXPORT
    ReduceStencilGeneration(const InputPortalType &input,
                            const KeyStatePortalType &kstate)
      : Input(input),
        KeyState(kstate)
    {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id centerIndex) const
    {
      typedef ReduceKeySeriesStates States;

      typedef typename InputPortalType::ValueType ValueType;
      typedef typename KeyStatePortalType::ValueType KeyStateType;

      const vtkm::Id leftIndex = centerIndex - 1;
      const vtkm::Id rightIndex = centerIndex + 1;

      //we need to determine which of three states this
      //index is. It can be:
      // 1. Middle of a set of equivalent keys.
      // 2. Start of a set of equivalent keys.
      // 3. End of a set of equivalent keys.
      // 4. Both the start and end of a set of keys

      //we don't have to worry about an array of length 1, as
      //the calling code handles that use case

      if(centerIndex == 0)
        {
        //this means we are at the start of the array
        //means we are automatically START
        //just need to check if we are END
        const ValueType centerValue = this->Input.Get(centerIndex);
        const ValueType rightValue = this->Input.Get(rightIndex);
        const KeyStateType state = (rightValue == centerValue) ? States::START :
                                                                 States::START_AND_END;
        this->KeyState.Set(centerIndex, state);
        }
      else if(rightIndex == this->Input.GetNumberOfValues())
        {
        //this means we are at the end, so we are at least END
        //just need to check if we are START
        const ValueType centerValue = this->Input.Get(centerIndex);
        const ValueType leftValue = this->Input.Get(leftIndex);
        const KeyStateType state = (leftValue == centerValue) ? States::END :
                                                               States::START_AND_END;
        this->KeyState.Set(centerIndex, state);
        }
      else
        {
        const ValueType centerValue = this->Input.Get(centerIndex);
        const bool leftMatches(this->Input.Get(leftIndex) == centerValue);
        const bool rightMatches(this->Input.Get(rightIndex) == centerValue);

        //assume it is the middle, and check for the other use-case
        KeyStateType state = States::MIDDLE;
        if(!leftMatches && rightMatches)
          {
          state = States::START;
          }
        else if(leftMatches && !rightMatches)
          {
          state = States::END;
          }
        else if(!leftMatches && !rightMatches)
          {
          state = States::START_AND_END;
          }
        this->KeyState.Set(centerIndex, state);
        }
    }
  };

  struct ReduceByKeyAdd
  {
    template<typename T>
    vtkm::Pair<T, vtkm::UInt8> operator()(const vtkm::Pair<T, vtkm::UInt8>& a,
                                          const vtkm::Pair<T, vtkm::UInt8>& b) const
    {
    typedef vtkm::Pair<T, vtkm::UInt8> ReturnType;
    typedef ReduceKeySeriesStates States;
    //need too handle how we are going to add two numbers together
    //based on the keyStates that they have

    //need to optimize this logic, we can use a bit mask to determine
    //the secondary value.
    if(a.second == States::START && b.second == States::END)
      {
      return ReturnType(a.first + b.first, States::START_AND_END); //with second type as START_AND_END
      }
    else if((a.second == States::START  || a.second == States::MIDDLE) &&
            (b.second == States::MIDDLE || b.second == States::END))
      {
      //note that we cant have START + END as that is handled above
      //as a special use case
      return ReturnType(a.first + b.first, b.second); //with second type as b.second
      }
    else
      {
      return b;
      }
    }

  };

  struct ReduceByKeyUnaryStencilOp
  {
    bool operator()(vtkm::UInt8 keySeriesState) const
    {
    typedef ReduceKeySeriesStates States;
    return (keySeriesState == States::END ||
            keySeriesState == States::START_AND_END);
    }

  };

public:
  template<typename T, typename U, class KIn, class VIn, class KOut, class VOut,
          class BinaryOperation>
  VTKM_CONT_EXPORT static void ReduceByKey(
      const vtkm::cont::ArrayHandle<T,KIn> &keys,
      const vtkm::cont::ArrayHandle<U,VIn> &values,
      vtkm::cont::ArrayHandle<T,KOut> &keys_output,
      vtkm::cont::ArrayHandle<U,VOut> &values_output,
      BinaryOperation binaryOp)
  {
    (void) binaryOp;

    VTKM_ASSERT_CONT(keys.GetNumberOfValues() == values.GetNumberOfValues());
    const vtkm::Id numberOfKeys = keys.GetNumberOfValues();

    if(numberOfKeys <= 1)
      { //we only have a single key/value so that is our output
      DerivedAlgorithm::Copy(keys, keys_output);
      DerivedAlgorithm::Copy(values, values_output);
      return;
      }

    //we need to determine based on the keys what is the keystate for
    //each key. The states are start, middle, end of a series and the special
    //state start and end of a series
    vtkm::cont::ArrayHandle< vtkm::UInt8 > keystate;

    {
    typedef typename vtkm::cont::ArrayHandle<T,KIn>::template ExecutionTypes<DeviceAdapterTag>
        ::PortalConst InputPortalType;

    typedef typename vtkm::cont::ArrayHandle< vtkm::UInt8 >::template ExecutionTypes<DeviceAdapterTag>
        ::Portal KeyStatePortalType;

    InputPortalType inputPortal = keys.PrepareForInput(DeviceAdapterTag());
    KeyStatePortalType keyStatePortal = keystate.PrepareForOutput(numberOfKeys,
                                                                 DeviceAdapterTag());
    ReduceStencilGeneration<InputPortalType> kernel(inputPortal, keyStatePortal);
    DerivedAlgorithm::Schedule(kernel, numberOfKeys);
    }

    //next step is we need to reduce the values for each key. This is done
    //by running an inclusive scan over the values array using the stencil.
    //
    // this inclusive scan will write out two values, the first being
    // the value summed currently, the second being 0 or 1, with 1 being used
    // when this is a value of a key we need to write ( END or START_AND_END)
    {
    typedef vtkm::cont::ArrayHandle<U,VIn> ValueHandleType;
    typedef vtkm::cont::ArrayHandle< vtkm::UInt8> StencilHandleType;
    typedef vtkm::cont::ArrayHandleZip<ValueHandleType,
                                      StencilHandleType> ZipHandleType;

    vtkm::cont::ArrayHandle< vtkm::UInt8 > stencil;
    vtkm::cont::ArrayHandle< U > reducedValues;

    ZipHandleType scanInput( values, keystate);
    ZipHandleType scanOutput( reducedValues, stencil);
    DerivedAlgorithm::ScanInclusive(scanInput, scanOutput, ReduceByKeyAdd() );

    //at this point we are done with keystate, so free the memory
    keystate.ReleaseResources();

    // all we need know is an efficient way of doing the write back to the
    // reduced global memory. this is done by using StreamCompact with the
    // stencil and values we just created with the inclusive scan
    DerivedAlgorithm::StreamCompact( reducedValues,
                                     stencil,
                                     values_output,
                                     ReduceByKeyUnaryStencilOp());

    } //release all temporary memory


    //find all the unique keys
    DerivedAlgorithm::Copy(keys,keys_output);
    DerivedAlgorithm::Unique(keys_output);
  }

  //--------------------------------------------------------------------------
  // Scan Exclusive
private:
  template<typename PortalType>
  struct SetConstantKernel
  {
    typedef typename PortalType::ValueType ValueType;
    PortalType Portal;
    ValueType Value;

    VTKM_CONT_EXPORT
    SetConstantKernel(const PortalType &portal, ValueType value)
      : Portal(portal), Value(value) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      this->Portal.Set(index, this->Value);
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };

public:
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    typedef vtkm::cont::ArrayHandle<T,vtkm::cont::StorageTagBasic>
        TempArrayType;
    typedef vtkm::cont::ArrayHandle<T,COut> OutputArrayType;

    TempArrayType inclusiveScan;
    T result = DerivedAlgorithm::ScanInclusive(input, inclusiveScan);

    vtkm::Id numValues = inclusiveScan.GetNumberOfValues();
    if (numValues < 1)
    {
      return result;
    }

    typedef typename TempArrayType::template ExecutionTypes<DeviceAdapterTag>
        ::PortalConst SrcPortalType;
    SrcPortalType srcPortal = inclusiveScan.PrepareForInput(DeviceAdapterTag());

    typedef typename OutputArrayType::template ExecutionTypes<DeviceAdapterTag>
        ::Portal DestPortalType;
    DestPortalType destPortal = output.PrepareForOutput(numValues,
                                                        DeviceAdapterTag());

    // Set first value in output (always 0).
    DerivedAlgorithm::Schedule(
          SetConstantKernel<DestPortalType>(destPortal,0), 1);
    // Shift remaining values over by one.
    DerivedAlgorithm::Schedule(
          CopyKernel<SrcPortalType,DestPortalType>(srcPortal,
                                                   destPortal,
                                                   0,
                                                   1),
          numValues - 1);

    return result;
  }

  //--------------------------------------------------------------------------
  // Scan Inclusive
private:
  template<typename PortalType, typename BinaryOperation>
  struct ScanKernel : vtkm::exec::FunctorBase
  {
    PortalType Portal;
    BinaryOperation BinaryOperator;
    vtkm::Id Stride;
    vtkm::Id Offset;
    vtkm::Id Distance;

    VTKM_CONT_EXPORT
    ScanKernel(const PortalType &portal, BinaryOperation binaryOp,
               vtkm::Id stride, vtkm::Id offset)
      : Portal(portal),
        BinaryOperator(binaryOp),
        Stride(stride),
        Offset(offset),
        Distance(stride/2)
    {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      typedef typename PortalType::ValueType ValueType;

      vtkm::Id leftIndex = this->Offset + index*this->Stride;
      vtkm::Id rightIndex = leftIndex + this->Distance;

      if (rightIndex < this->Portal.GetNumberOfValues())
      {
        ValueType leftValue = this->Portal.Get(leftIndex);
        ValueType rightValue = this->Portal.Get(rightIndex);
        this->Portal.Set(rightIndex, BinaryOperator(leftValue,rightValue) );
      }
    }
  };

public:
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    return DerivedAlgorithm::ScanInclusive(input,
                                            output,
                                            vtkm::internal::Add());
  }

  template<typename T, class CIn, class COut, class BinaryOperation>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output,
      BinaryOperation binaryOp)
  {
    typedef typename
        vtkm::cont::ArrayHandle<T,COut>
            ::template ExecutionTypes<DeviceAdapterTag>::Portal PortalType;

    typedef ScanKernel<PortalType,BinaryOperation> ScanKernelType;

    DerivedAlgorithm::Copy(input, output);

    vtkm::Id numValues = output.GetNumberOfValues();
    if (numValues < 1)
      {
      return output.GetPortalConstControl().Get(0);
      }

    PortalType portal = output.PrepareForInPlace(DeviceAdapterTag());

    vtkm::Id stride;
    for (stride = 2; stride-1 < numValues; stride *= 2)
    {
      ScanKernelType kernel(portal, binaryOp, stride, stride/2 - 1);
      DerivedAlgorithm::Schedule(kernel, numValues/stride);
    }

    // Do reverse operation on odd indices. Start at stride we were just at.
    for (stride /= 2; stride > 1; stride /= 2)
    {
      ScanKernelType kernel(portal, binaryOp, stride, stride - 1);
      DerivedAlgorithm::Schedule(kernel, numValues/stride);
    }

    return GetExecutionValue(output, numValues-1);
  }

  //--------------------------------------------------------------------------
  // Sort
private:
  template<typename PortalType, typename CompareType>
  struct BitonicSortMergeKernel : vtkm::exec::FunctorBase
  {
    PortalType Portal;
    CompareType Compare;
    vtkm::Id GroupSize;

    VTKM_CONT_EXPORT
    BitonicSortMergeKernel(const PortalType &portal,
                           const CompareType &compare,
                           vtkm::Id groupSize)
      : Portal(portal), Compare(compare), GroupSize(groupSize) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      typedef typename PortalType::ValueType ValueType;

      vtkm::Id groupIndex = index%this->GroupSize;
      vtkm::Id blockSize = 2*this->GroupSize;
      vtkm::Id blockIndex = index/this->GroupSize;

      vtkm::Id lowIndex = blockIndex * blockSize + groupIndex;
      vtkm::Id highIndex = lowIndex + this->GroupSize;

      if (highIndex < this->Portal.GetNumberOfValues())
      {
        ValueType lowValue = this->Portal.Get(lowIndex);
        ValueType highValue = this->Portal.Get(highIndex);
        if (this->Compare(highValue, lowValue))
        {
          this->Portal.Set(highIndex, lowValue);
          this->Portal.Set(lowIndex, highValue);
        }
      }
    }
  };

  template<typename PortalType, typename CompareType>
  struct BitonicSortCrossoverKernel : vtkm::exec::FunctorBase
  {
    PortalType Portal;
    CompareType Compare;
    vtkm::Id GroupSize;

    VTKM_CONT_EXPORT
    BitonicSortCrossoverKernel(const PortalType &portal,
                               const CompareType &compare,
                               vtkm::Id groupSize)
      : Portal(portal), Compare(compare), GroupSize(groupSize) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      typedef typename PortalType::ValueType ValueType;

      vtkm::Id groupIndex = index%this->GroupSize;
      vtkm::Id blockSize = 2*this->GroupSize;
      vtkm::Id blockIndex = index/this->GroupSize;

      vtkm::Id lowIndex = blockIndex*blockSize + groupIndex;
      vtkm::Id highIndex = blockIndex*blockSize + (blockSize - groupIndex - 1);

      if (highIndex < this->Portal.GetNumberOfValues())
      {
        ValueType lowValue = this->Portal.Get(lowIndex);
        ValueType highValue = this->Portal.Get(highIndex);
        if (this->Compare(highValue, lowValue))
        {
          this->Portal.Set(highIndex, lowValue);
          this->Portal.Set(lowIndex, highValue);
        }
      }
    }
  };

  struct DefaultCompareFunctor
  {

    template<typename T>
    VTKM_EXEC_EXPORT
    bool operator()(const T& first, const T& second) const
    {
      return first < second;
    }
  };

public:
  template<typename T, class Storage, class CompareType>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Storage> &values,
      CompareType compare)
  {
    typedef typename vtkm::cont::ArrayHandle<T,Storage> ArrayType;
    typedef typename ArrayType::template ExecutionTypes<DeviceAdapterTag>
        ::Portal PortalType;

    vtkm::Id numValues = values.GetNumberOfValues();
    if (numValues < 2) { return; }

    PortalType portal = values.PrepareForInPlace(DeviceAdapterTag());

    vtkm::Id numThreads = 1;
    while (numThreads < numValues) { numThreads *= 2; }
    numThreads /= 2;

    typedef BitonicSortMergeKernel<PortalType,CompareType> MergeKernel;
    typedef BitonicSortCrossoverKernel<PortalType,CompareType> CrossoverKernel;

    for (vtkm::Id crossoverSize = 1;
         crossoverSize < numValues;
         crossoverSize *= 2)
    {
      DerivedAlgorithm::Schedule(CrossoverKernel(portal,compare,crossoverSize),
                                 numThreads);
      for (vtkm::Id mergeSize = crossoverSize/2; mergeSize > 0; mergeSize /= 2)
      {
        DerivedAlgorithm::Schedule(MergeKernel(portal,compare,mergeSize),
                                   numThreads);
      }
    }
  }

  template<typename T, class Storage>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Storage> &values)
  {
    DerivedAlgorithm::Sort(values, DefaultCompareFunctor());
  }

  //--------------------------------------------------------------------------
  // Sort by Key
protected:
  template<typename T, typename U, class Compare=DefaultCompareFunctor>
  struct KeyCompare
  {
    KeyCompare(): CompareFunctor() {}
    explicit KeyCompare(Compare c): CompareFunctor(c) {}

    VTKM_EXEC_EXPORT
    bool operator()(const vtkm::Pair<T,U>& a, const vtkm::Pair<T,U>& b) const
    {
      return CompareFunctor(a.first,b.first);
    }
  private:
    Compare CompareFunctor;
  };

public:

  template<typename T, typename U, class StorageT,  class StorageU>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT> &keys,
      vtkm::cont::ArrayHandle<U,StorageU> &values)
  {
    //combine the keys and values into a ZipArrayHandle
    //we than need to specify a custom compare function wrapper
    //that only checks for key side of the pair, using a custom compare functor.
    typedef vtkm::cont::ArrayHandle<T,StorageT> KeyType;
    typedef vtkm::cont::ArrayHandle<U,StorageU> ValueType;
    typedef vtkm::cont::ArrayHandleZip<KeyType,ValueType> ZipHandleType;

    ZipHandleType zipHandle =
                    vtkm::cont::make_ArrayHandleZip(keys,values);
    DerivedAlgorithm::Sort(zipHandle,KeyCompare<T,U>());
  }

  template<typename T, typename U, class StorageT,  class StorageU, class Compare>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT> &keys,
      vtkm::cont::ArrayHandle<U,StorageU> &values,
      Compare comp)
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
    DerivedAlgorithm::Sort(zipHandle,KeyCompare<T,U,Compare>(comp));
  }

  //--------------------------------------------------------------------------
  // Stream Compact
private:
  template<class StencilPortalType,
           class OutputPortalType,
           class PredicateOperator>
  struct StencilToIndexFlagKernel
  {
    typedef typename StencilPortalType::ValueType StencilValueType;
    StencilPortalType StencilPortal;
    OutputPortalType OutputPortal;
    PredicateOperator Predicate;

    VTKM_CONT_EXPORT
    StencilToIndexFlagKernel(StencilPortalType stencilPortal,
                             OutputPortalType outputPortal,
                             PredicateOperator predicate)
      : StencilPortal(stencilPortal),
        OutputPortal(outputPortal),
        Predicate(predicate) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      StencilValueType value = this->StencilPortal.Get(index);
      this->OutputPortal.Set(index, this->Predicate(value) ? 1 : 0);
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };

  template<class InputPortalType,
           class StencilPortalType,
           class IndexPortalType,
           class OutputPortalType,
           class PredicateOperator>
  struct CopyIfKernel
  {
    InputPortalType InputPortal;
    StencilPortalType StencilPortal;
    IndexPortalType IndexPortal;
    OutputPortalType OutputPortal;
    PredicateOperator Predicate;

    VTKM_CONT_EXPORT
    CopyIfKernel(InputPortalType inputPortal,
                 StencilPortalType stencilPortal,
                 IndexPortalType indexPortal,
                 OutputPortalType outputPortal,
                 PredicateOperator predicate)
      : InputPortal(inputPortal),
        StencilPortal(stencilPortal),
        IndexPortal(indexPortal),
        OutputPortal(outputPortal),
        Predicate(predicate) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      typedef typename StencilPortalType::ValueType StencilValueType;
      StencilValueType stencilValue = this->StencilPortal.Get(index);
      if (Predicate(stencilValue))
        {
        vtkm::Id outputIndex = this->IndexPortal.Get(index);

        typedef typename OutputPortalType::ValueType OutputValueType;
        OutputValueType value = this->InputPortal.Get(index);

        this->OutputPortal.Set(outputIndex, value);
        }
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };

public:

  template<typename T, typename U, class CIn, class CStencil,
           class COut, class PredicateOperator>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      const vtkm::cont::ArrayHandle<U,CStencil>& stencil,
      vtkm::cont::ArrayHandle<T,COut>& output,
      PredicateOperator predicate)
  {
    VTKM_ASSERT_CONT(input.GetNumberOfValues() == stencil.GetNumberOfValues());
    vtkm::Id arrayLength = stencil.GetNumberOfValues();

    typedef vtkm::cont::ArrayHandle<
        vtkm::Id, vtkm::cont::StorageTagBasic> IndexArrayType;
    IndexArrayType indices;

    typedef typename vtkm::cont::ArrayHandle<U,CStencil>
        ::template ExecutionTypes<DeviceAdapterTag>::PortalConst
        StencilPortalType;
    StencilPortalType stencilPortal =
        stencil.PrepareForInput(DeviceAdapterTag());

    typedef typename IndexArrayType
        ::template ExecutionTypes<DeviceAdapterTag>::Portal IndexPortalType;
    IndexPortalType indexPortal =
        indices.PrepareForOutput(arrayLength, DeviceAdapterTag());

    StencilToIndexFlagKernel< StencilPortalType,
                              IndexPortalType,
                              PredicateOperator> indexKernel(stencilPortal,
                                                         indexPortal,
                                                         predicate);

    DerivedAlgorithm::Schedule(indexKernel, arrayLength);

    vtkm::Id outArrayLength = DerivedAlgorithm::ScanExclusive(indices, indices);

    typedef typename vtkm::cont::ArrayHandle<T,CIn>
        ::template ExecutionTypes<DeviceAdapterTag>::PortalConst
        InputPortalType;
    InputPortalType inputPortal = input.PrepareForInput(DeviceAdapterTag());

    typedef typename vtkm::cont::ArrayHandle<T,COut>
        ::template ExecutionTypes<DeviceAdapterTag>::Portal OutputPortalType;
    OutputPortalType outputPortal =
        output.PrepareForOutput(outArrayLength, DeviceAdapterTag());

    CopyIfKernel<
        InputPortalType,
        StencilPortalType,
        IndexPortalType,
        OutputPortalType,
        PredicateOperator> copyKernel(inputPortal,
                                    stencilPortal,
                                    indexPortal,
                                    outputPortal,
                                    predicate);
    DerivedAlgorithm::Schedule(copyKernel, arrayLength);
  }

template<typename T, typename U, class CIn, class CStencil, class COut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      const vtkm::cont::ArrayHandle<U,CStencil>& stencil,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    ::vtkm::not_default_constructor<U> predicate;
    DerivedAlgorithm::StreamCompact(input, stencil, output, predicate);
  }

  template<typename T, class CStencil, class COut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,CStencil> &stencil,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &output)
  {
    typedef vtkm::cont::ArrayHandleCounting<vtkm::Id> CountingHandleType;

    CountingHandleType input =
        vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0),
                                            stencil.GetNumberOfValues());
    DerivedAlgorithm::StreamCompact(input, stencil, output);
  }

  //--------------------------------------------------------------------------
  // Unique
private:
  template<class InputPortalType, class StencilPortalType>
  struct ClassifyUniqueKernel
  {
    InputPortalType InputPortal;
    StencilPortalType StencilPortal;

    VTKM_CONT_EXPORT
    ClassifyUniqueKernel(InputPortalType inputPortal,
                         StencilPortalType stencilPortal)
      : InputPortal(inputPortal), StencilPortal(stencilPortal) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      typedef typename StencilPortalType::ValueType ValueType;
      if (index == 0)
      {
        // Always copy first value.
        this->StencilPortal.Set(index, ValueType(1));
      }
      else
      {
        ValueType flag = ValueType(this->InputPortal.Get(index-1)
                                   != this->InputPortal.Get(index));
        this->StencilPortal.Set(index, flag);
      }
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };

  template<class InputPortalType, class StencilPortalType, class Compare>
  struct ClassifyUniqueComparisonKernel
  {
    InputPortalType InputPortal;
    StencilPortalType StencilPortal;
    Compare CompareFunctor;

    VTKM_CONT_EXPORT
    ClassifyUniqueComparisonKernel(InputPortalType inputPortal,
                                   StencilPortalType stencilPortal,
                                   Compare comp):
      InputPortal(inputPortal),
      StencilPortal(stencilPortal),
      CompareFunctor(comp) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      typedef typename StencilPortalType::ValueType ValueType;
      if (index == 0)
      {
        // Always copy first value.
        this->StencilPortal.Set(index, ValueType(1));
      }
      else
      {
        //comparison predicate returns true when they match
        const bool same = !(this->CompareFunctor(this->InputPortal.Get(index-1),
                                                 this->InputPortal.Get(index)));
        ValueType flag = ValueType(same);
        this->StencilPortal.Set(index, flag);
      }
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };

public:
  template<typename T, class Storage>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values)
  {
    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>
        stencilArray;
    vtkm::Id inputSize = values.GetNumberOfValues();

    ClassifyUniqueKernel<
        typename vtkm::cont::ArrayHandle<T,Storage>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<vtkm::Id,vtkm::cont::StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::Portal>
        classifyKernel(values.PrepareForInput(DeviceAdapterTag()),
                       stencilArray.PrepareForOutput(inputSize, DeviceAdapterTag()));
    DerivedAlgorithm::Schedule(classifyKernel, inputSize);

    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>
        outputArray;

    DerivedAlgorithm::StreamCompact(values, stencilArray, outputArray);

    DerivedAlgorithm::Copy(outputArray, values);
  }

  template<typename T, class Storage, class Compare>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values,
      Compare comp)
  {
    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>
        stencilArray;
    vtkm::Id inputSize = values.GetNumberOfValues();

    ClassifyUniqueComparisonKernel<
        typename vtkm::cont::ArrayHandle<T,Storage>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<vtkm::Id,vtkm::cont::StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::Portal,
        Compare>
        classifyKernel(values.PrepareForInput(DeviceAdapterTag()),
                       stencilArray.PrepareForOutput(inputSize, DeviceAdapterTag()),
                       comp);
    DerivedAlgorithm::Schedule(classifyKernel, inputSize);

    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>
        outputArray;

    DerivedAlgorithm::StreamCompact(values, stencilArray, outputArray);

    DerivedAlgorithm::Copy(outputArray, values);
  }

  //--------------------------------------------------------------------------
  // Upper bounds
private:
  template<class InputPortalType,class ValuesPortalType,class OutputPortalType>
  struct UpperBoundsKernel
  {
    InputPortalType InputPortal;
    ValuesPortalType ValuesPortal;
    OutputPortalType OutputPortal;

    VTKM_CONT_EXPORT
    UpperBoundsKernel(InputPortalType inputPortal,
                      ValuesPortalType valuesPortal,
                      OutputPortalType outputPortal)
      : InputPortal(inputPortal),
        ValuesPortal(valuesPortal),
        OutputPortal(outputPortal) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      // This method assumes that (1) InputPortalType can return working
      // iterators in the execution environment and that (2) methods not
      // specified with VTKM_EXEC_EXPORT (such as the STL algorithms) can be
      // called from the execution environment. Neither one of these is
      // necessarily true, but it is true for the current uses of this general
      // function and I don't want to compete with STL if I don't have to.

      typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
          InputIteratorsType;
      InputIteratorsType inputIterators(this->InputPortal);
      typename InputIteratorsType::IteratorType resultPos =
          std::upper_bound(inputIterators.GetBegin(),
                           inputIterators.GetEnd(),
                           this->ValuesPortal.Get(index));

      vtkm::Id resultIndex =
          static_cast<vtkm::Id>(
            std::distance(inputIterators.GetBegin(), resultPos));
      this->OutputPortal.Set(index, resultIndex);
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };


  template<class InputPortalType,class ValuesPortalType,class OutputPortalType,class Compare>
  struct UpperBoundsKernelComparisonKernel
  {
    InputPortalType InputPortal;
    ValuesPortalType ValuesPortal;
    OutputPortalType OutputPortal;
    Compare CompareFunctor;

    VTKM_CONT_EXPORT
    UpperBoundsKernelComparisonKernel(InputPortalType inputPortal,
                                      ValuesPortalType valuesPortal,
                                      OutputPortalType outputPortal,
                                      Compare comp)
      : InputPortal(inputPortal),
        ValuesPortal(valuesPortal),
        OutputPortal(outputPortal),
        CompareFunctor(comp) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      // This method assumes that (1) InputPortalType can return working
      // iterators in the execution environment and that (2) methods not
      // specified with VTKM_EXEC_EXPORT (such as the STL algorithms) can be
      // called from the execution environment. Neither one of these is
      // necessarily true, but it is true for the current uses of this general
      // function and I don't want to compete with STL if I don't have to.

      typedef vtkm::cont::ArrayPortalToIterators<InputPortalType>
          InputIteratorsType;
      InputIteratorsType inputIterators(this->InputPortal);
      typename InputIteratorsType::IteratorType resultPos =
          std::upper_bound(inputIterators.GetBegin(),
                           inputIterators.GetEnd(),
                           this->ValuesPortal.Get(index),
                           this->CompareFunctor);

      vtkm::Id resultIndex =
          static_cast<vtkm::Id>(
            std::distance(inputIterators.GetBegin(), resultPos));
      this->OutputPortal.Set(index, resultIndex);
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };

public:
  template<typename T, class CIn, class CVal, class COut>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      const vtkm::cont::ArrayHandle<T,CVal> &values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &output)
  {
    vtkm::Id arraySize = values.GetNumberOfValues();

    UpperBoundsKernel<
        typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<T,CVal>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<T,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal>
        kernel(input.PrepareForInput(DeviceAdapterTag()),
               values.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(arraySize, DeviceAdapterTag()));

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template<typename T, class CIn, class CVal, class COut, class Compare>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      const vtkm::cont::ArrayHandle<T,CVal> &values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &output,
      Compare comp)
  {
    vtkm::Id arraySize = values.GetNumberOfValues();

    UpperBoundsKernelComparisonKernel<
        typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<T,CVal>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<T,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal,
        Compare>
        kernel(input.PrepareForInput(DeviceAdapterTag()),
               values.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(arraySize, DeviceAdapterTag()),
               comp);

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template<class CIn, class COut>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<vtkm::Id,CIn> &input,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &values_output)
  {
    DeviceAdapterAlgorithmGeneral<DerivedAlgorithm,
      DeviceAdapterTag>::UpperBounds(input, values_output, values_output);
  }

};


}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_DeviceAdapterAlgorithmGeneral_h
