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

#include <vtkm/cont/ArrayHandleStreaming.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/internal/FunctorsGeneral.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>

#include <vtkm/TypeTraits.h>

#include <vtkm/internal/Windows.h>

namespace vtkm {
namespace cont {
namespace internal {

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
///   VTKM_CONT static void Schedule(Functor functor,
///                                        vtkm::Id numInstances)
///   {
///     ...
///   }
///
///   template<class Functor>
///   VTKM_CONT static void Schedule(Functor functor,
///                                        vtkm::Id3 maxRange)
///   {
///     ...
///   }
///
///   VTKM_CONT static void Synchronize()
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
  VTKM_CONT
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


public:
  //--------------------------------------------------------------------------
  // Copy
  template<typename T, typename U, class CIn, class COut>
  VTKM_CONT static void Copy(const vtkm::cont::ArrayHandle<T, CIn> &input,
                                    vtkm::cont::ArrayHandle<U, COut> &output)
  {
    typedef  CopyKernel<
          typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
          typename vtkm::cont::ArrayHandle<U,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal>
            CopyKernelType;
    const vtkm::Id inSize = input.GetNumberOfValues();

    CopyKernelType kernel(input.PrepareForInput(DeviceAdapterTag()),
                          output.PrepareForOutput(inSize, DeviceAdapterTag()));
    DerivedAlgorithm::Schedule(kernel, inSize);
  }

  //--------------------------------------------------------------------------
  // CopyIf
  template<typename T, typename U, class CIn, class CStencil,
  class COut, class UnaryPredicate>
  VTKM_CONT static void CopyIf(
    const vtkm::cont::ArrayHandle<T,CIn>& input,
    const vtkm::cont::ArrayHandle<U,CStencil>& stencil,
    vtkm::cont::ArrayHandle<T,COut>& output,
    UnaryPredicate unary_predicate)
  {
    VTKM_ASSERT(input.GetNumberOfValues() == stencil.GetNumberOfValues());
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
    UnaryPredicate> indexKernel(stencilPortal,
                                indexPortal,
                                unary_predicate);

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
    UnaryPredicate> copyKernel(inputPortal,
                               stencilPortal,
                               indexPortal,
                               outputPortal,
                               unary_predicate);
    DerivedAlgorithm::Schedule(copyKernel, arrayLength);
  }

  template<typename T, typename U, class CIn, class CStencil, class COut>
  VTKM_CONT static void CopyIf(
    const vtkm::cont::ArrayHandle<T,CIn>& input,
    const vtkm::cont::ArrayHandle<U,CStencil>& stencil,
    vtkm::cont::ArrayHandle<T,COut>& output)
  {
    ::vtkm::NotZeroInitialized unary_predicate;
    DerivedAlgorithm::CopyIf(input, stencil, output, unary_predicate);
  }

  //--------------------------------------------------------------------------
  // CopySubRange
  template<typename T, typename U, class CIn, class COut>
  VTKM_CONT static bool CopySubRange(const vtkm::cont::ArrayHandle<T, CIn> &input,
                                            vtkm::Id inputStartIndex,
                                            vtkm::Id numberOfElementsToCopy,
                                            vtkm::cont::ArrayHandle<U, COut> &output,
                                            vtkm::Id outputIndex=0)
  {
    typedef  CopyKernel<
          typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
          typename vtkm::cont::ArrayHandle<U,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal>
            CopyKernel;

    const vtkm::Id inSize = input.GetNumberOfValues();
    if(inputStartIndex < 0 ||
       numberOfElementsToCopy < 0 ||
       outputIndex < 0 ||
       inputStartIndex >= inSize)
    {  //invalid parameters
      return false;
    }

    //determine if the numberOfElementsToCopy needs to be reduced
    if(inSize < (inputStartIndex + numberOfElementsToCopy))
      { //adjust the size
      numberOfElementsToCopy = (inSize - inputStartIndex);
      }

    const vtkm::Id outSize = output.GetNumberOfValues();
    const vtkm::Id copyOutEnd = outputIndex + numberOfElementsToCopy;
    if(outSize < copyOutEnd)
    { //output is not large enough
      if(outSize == 0)
      { //since output has nothing, just need to allocate to correct length
        output.Allocate(copyOutEnd);
      }
      else
      { //we currently have data in this array, so preserve it in the new
        //resized array
        vtkm::cont::ArrayHandle<U, COut> temp;
        temp.Allocate(copyOutEnd);
        DerivedAlgorithm::CopySubRange(output, 0, outSize, temp);
        output = temp;
      }
    }

    CopyKernel kernel(input.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForInPlace(DeviceAdapterTag()),
                      inputStartIndex,
                      outputIndex);
    DerivedAlgorithm::Schedule(kernel, numberOfElementsToCopy);
    return true;
  }

  //--------------------------------------------------------------------------
  // Lower Bounds
  template<typename T, class CIn, class CVal, class COut>
  VTKM_CONT static void LowerBounds(
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

  template<typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT static void LowerBounds(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      const vtkm::cont::ArrayHandle<T,CVal> &values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &output,
      BinaryCompare binary_compare)
  {
    vtkm::Id arraySize = values.GetNumberOfValues();

    LowerBoundsComparisonKernel<
        typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<T,CVal>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<vtkm::Id,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal,
        BinaryCompare>
        kernel(input.PrepareForInput(DeviceAdapterTag()),
               values.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(arraySize, DeviceAdapterTag()),
               binary_compare);

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template<class CIn, class COut>
  VTKM_CONT static void LowerBounds(
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
 template<typename T, typename U, class CIn>
  VTKM_CONT static U Reduce(
      const vtkm::cont::ArrayHandle<T,CIn> &input, U initialValue)
  {
    return DerivedAlgorithm::Reduce(input, initialValue,vtkm::Add());
  }

 template<typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      U initialValue,
      BinaryFunctor binary_functor)
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
    typedef typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>
        ::PortalConst InputPortalType;

    typedef ReduceKernel<
            InputPortalType,
            U,
            BinaryFunctor
            > ReduceKernelType;

    typedef vtkm::cont::ArrayHandleImplicit<
                                            U,
                                            ReduceKernelType > ReduceHandleType;
    typedef vtkm::cont::ArrayHandle<
                                    U,
                                    vtkm::cont::StorageTagBasic> TempArrayType;

    ReduceKernelType kernel(input.PrepareForInput( DeviceAdapterTag() ),
                            initialValue,
                            binary_functor);

    vtkm::Id length = (input.GetNumberOfValues() / 16);
    length += (input.GetNumberOfValues() % 16 == 0) ? 0 : 1;
    ReduceHandleType reduced = vtkm::cont::make_ArrayHandleImplicit<U>(kernel,
                                                                       length);

    TempArrayType inclusiveScanStorage;
    const U scanResult = DerivedAlgorithm::ScanInclusive(reduced,
                                                   inclusiveScanStorage,
                                                   binary_functor);
    return scanResult;
  }

  //--------------------------------------------------------------------------
  // Streaming Reduce
  template<typename T, typename U, class CIn>
  VTKM_CONT static U StreamingReduce(
      const vtkm::Id numBlocks,
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      U initialValue)
  {
    return DerivedAlgorithm::StreamingReduce(numBlocks, input, initialValue, vtkm::Add());
  }

  template<typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static U StreamingReduce(
      const vtkm::Id numBlocks,
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      U initialValue,
      BinaryFunctor binary_functor)
  {
    vtkm::Id fullSize = input.GetNumberOfValues();
    vtkm::Id blockSize = fullSize / numBlocks;
    if (fullSize % numBlocks != 0) blockSize += 1;

    U lastResult;
    for (vtkm::Id block=0; block<numBlocks; block++)
    {
      vtkm::Id numberOfInstances = blockSize;
      if (block == numBlocks-1)
        numberOfInstances = fullSize - blockSize*block;

      vtkm::cont::ArrayHandleStreaming<vtkm::cont::ArrayHandle<T,CIn> > streamIn =
          vtkm::cont::ArrayHandleStreaming<vtkm::cont::ArrayHandle<T,CIn> >(
          input, block, blockSize, numberOfInstances);

      if (block == 0)
        lastResult = DerivedAlgorithm::Reduce(streamIn, initialValue, binary_functor);
      else
        lastResult = DerivedAlgorithm::Reduce(streamIn, lastResult, binary_functor);
    }
    return lastResult;
  }

  //--------------------------------------------------------------------------
  // Reduce By Key
  template<typename T, typename U, class KIn, class VIn, class KOut, class VOut,
          class BinaryFunctor>
  VTKM_CONT static void ReduceByKey(
      const vtkm::cont::ArrayHandle<T,KIn> &keys,
      const vtkm::cont::ArrayHandle<U,VIn> &values,
      vtkm::cont::ArrayHandle<T,KOut> &keys_output,
      vtkm::cont::ArrayHandle<U,VOut> &values_output,
      BinaryFunctor binary_functor)
  {
    VTKM_ASSERT(keys.GetNumberOfValues() == values.GetNumberOfValues());
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
    vtkm::cont::ArrayHandle< ReduceKeySeriesStates > keystate;

    {
    typedef typename vtkm::cont::ArrayHandle<T,KIn>::template ExecutionTypes<DeviceAdapterTag>
        ::PortalConst InputPortalType;

    typedef typename vtkm::cont::ArrayHandle< ReduceKeySeriesStates >::template ExecutionTypes<DeviceAdapterTag>
        ::Portal KeyStatePortalType;

    InputPortalType inputPortal = keys.PrepareForInput(DeviceAdapterTag());
    KeyStatePortalType keyStatePortal = keystate.PrepareForOutput(numberOfKeys,
                                                                 DeviceAdapterTag());
    ReduceStencilGeneration<InputPortalType, KeyStatePortalType> kernel(inputPortal, keyStatePortal);
    DerivedAlgorithm::Schedule(kernel, numberOfKeys);
    }

    //next step is we need to reduce the values for each key. This is done
    //by running an inclusive scan over the values array using the stencil.
    //
    // this inclusive scan will write out two values, the first being
    // the value summed currently, the second being 0 or 1, with 1 being used
    // when this is a value of a key we need to write ( END or START_AND_END)
    {
    typedef vtkm::cont::ArrayHandle<U,VIn> ValueInHandleType;
    typedef vtkm::cont::ArrayHandle<U,VOut> ValueOutHandleType;
    typedef vtkm::cont::ArrayHandle< ReduceKeySeriesStates> StencilHandleType;
    typedef vtkm::cont::ArrayHandleZip<ValueInHandleType,
                                      StencilHandleType> ZipInHandleType;
        typedef vtkm::cont::ArrayHandleZip<ValueOutHandleType,
                                          StencilHandleType> ZipOutHandleType;

    StencilHandleType stencil;
    ValueOutHandleType reducedValues;

    ZipInHandleType scanInput( values, keystate);
    ZipOutHandleType scanOutput( reducedValues, stencil);

    DerivedAlgorithm::ScanInclusive(scanInput,
                                    scanOutput,
                                    ReduceByKeyAdd<BinaryFunctor>(binary_functor) );

    //at this point we are done with keystate, so free the memory
    keystate.ReleaseResources();

    // all we need know is an efficient way of doing the write back to the
    // reduced global memory. this is done by using CopyIf with the
    // stencil and values we just created with the inclusive scan
    DerivedAlgorithm::CopyIf( reducedValues,
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
  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      vtkm::cont::ArrayHandle<T,COut>& output,
      BinaryFunctor binaryFunctor,
      const T& initialValue)
  {
    typedef vtkm::cont::ArrayHandle<T,vtkm::cont::StorageTagBasic>
        TempArrayType;
    typedef vtkm::cont::ArrayHandle<T,COut> OutputArrayType;

    typedef typename TempArrayType::template ExecutionTypes<DeviceAdapterTag>
        ::PortalConst SrcPortalType;
    typedef typename OutputArrayType::template ExecutionTypes<DeviceAdapterTag>
        ::Portal DestPortalType;

    vtkm::Id numValues = input.GetNumberOfValues();
    if (numValues <= 0)
    {
      return initialValue;
    }

    TempArrayType inclusiveScan;
    T result = DerivedAlgorithm::ScanInclusive(input, inclusiveScan, binaryFunctor);

    InclusiveToExclusiveKernel<SrcPortalType, DestPortalType, BinaryFunctor>
      inclusiveToExclusive(inclusiveScan.PrepareForInput(DeviceAdapterTag()),
                           output.PrepareForOutput(numValues, DeviceAdapterTag()),
                           binaryFunctor,
                           initialValue);

    DerivedAlgorithm::Schedule(inclusiveToExclusive, numValues);

    return binaryFunctor(initialValue, result);
  }

  template<typename T, class CIn, class COut>
  VTKM_CONT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    // TODO: add DerivedAlgorithm?
    return DerivedAlgorithm::ScanExclusive(input, output, vtkm::Sum(),
                         vtkm::TypeTraits<T>::ZeroInitialization());
  }

  //--------------------------------------------------------------------------
  // TODO: what is the return type/value?
  // Scan Exclusive By Key
  template<typename T, typename U, typename KIn, typename VIn, typename VOut,
    class BinaryFunctor>
  VTKM_CONT static void ScanExclusiveByKey(
    const vtkm::cont::ArrayHandle<T, KIn>& keys,
    const vtkm::cont::ArrayHandle<U, VIn>& values,
    vtkm::cont::ArrayHandle<U ,VOut>& output,
    BinaryFunctor binaryFunctor,
    const U& initialValue)
  {
    // 0. TODO: special case for 1 element input?
    vtkm::Id numberOfKeys = keys.GetNumberOfValues();

    // 1. Create head flags
    //we need to determine based on the keys what is the keystate for
    //each key. The states are start, middle, end of a series and the special
    //state start and end of a series
    vtkm::cont::ArrayHandle< ReduceKeySeriesStates > keystate;

    {
      typedef typename vtkm::cont::ArrayHandle<T,KIn>::template ExecutionTypes<DeviceAdapterTag>
      ::PortalConst InputPortalType;

      typedef typename vtkm::cont::ArrayHandle< ReduceKeySeriesStates >::template ExecutionTypes<DeviceAdapterTag>
      ::Portal KeyStatePortalType;

      InputPortalType inputPortal = keys.PrepareForInput(DeviceAdapterTag());
      KeyStatePortalType keyStatePortal = keystate.PrepareForOutput(numberOfKeys,
                                                                    DeviceAdapterTag());
      ReduceStencilGeneration<InputPortalType, KeyStatePortalType> kernel(inputPortal, keyStatePortal);
      DerivedAlgorithm::Schedule(kernel, numberOfKeys);
    }

    // 2. Shift input and initialize elements at head flags position to initValue
    typedef typename vtkm::cont::ArrayHandle<T,vtkm::cont::StorageTagBasic> TempArrayType;
    typedef typename vtkm::cont::ArrayHandle<T,vtkm::cont::StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::Portal TempPortalType;
    TempArrayType temp;
    {
      typedef typename vtkm::cont::ArrayHandle<T,KIn>::template ExecutionTypes<DeviceAdapterTag>
      ::PortalConst InputPortalType;

      typedef typename vtkm::cont::ArrayHandle< ReduceKeySeriesStates >::template ExecutionTypes<DeviceAdapterTag>
      ::PortalConst KeyStatePortalType;

      InputPortalType inputPortal = values.PrepareForInput(DeviceAdapterTag());
      KeyStatePortalType keyStatePortal = keystate.PrepareForInput(DeviceAdapterTag());
      TempPortalType tempPortal = temp.PrepareForOutput(numberOfKeys,
                                                              DeviceAdapterTag());

      ShiftCopyAndInit<U, InputPortalType, KeyStatePortalType, TempPortalType>
        kernel(inputPortal, keyStatePortal, tempPortal, initialValue);
      DerivedAlgorithm::Schedule(kernel, numberOfKeys);
    }
    // 3. Perform an ScanInclusiveByKey
    DerivedAlgorithm::ScanInclusiveByKey(keys, temp, output, binaryFunctor);
  }

  template<typename T, typename U, class KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanExclusiveByKey(
    const vtkm::cont::ArrayHandle<T, KIn>& keys,
    const vtkm::cont::ArrayHandle<U, VIn>& values,
    vtkm::cont::ArrayHandle<U, VOut>& output)
  {
    // TODO: add DerivedAlgorithm?
    ScanExclusiveByKey(keys, values, output, vtkm::Sum(),
                       vtkm::TypeTraits<U>::ZeroInitialization());
  }

  //--------------------------------------------------------------------------
  // Streaming exclusive scan
  template<typename T, class CIn, class COut>
  VTKM_CONT static T StreamingScanExclusive(
      const vtkm::Id numBlocks,
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    return DerivedAlgorithm::StreamingScanExclusive(numBlocks,
                                                    input,
                                                    output,
                                                    vtkm::Sum(),
                                                    vtkm::TypeTraits<T>::ZeroInitialization());
  }

  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T StreamingScanExclusive(
      const vtkm::Id numBlocks,
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      vtkm::cont::ArrayHandle<T,COut>& output,
      BinaryFunctor binary_functor,
      const T& initialValue)
  {
    vtkm::Id fullSize = input.GetNumberOfValues();
    vtkm::Id blockSize = fullSize / numBlocks;
    if (fullSize % numBlocks != 0) blockSize += 1;

    T lastResult;
    for (vtkm::Id block=0; block<numBlocks; block++)
    {
      vtkm::Id numberOfInstances = blockSize;
      if (block == numBlocks-1)
        numberOfInstances = fullSize - blockSize*block;

      vtkm::cont::ArrayHandleStreaming<vtkm::cont::ArrayHandle<T,CIn> > streamIn =
          vtkm::cont::ArrayHandleStreaming<vtkm::cont::ArrayHandle<T,CIn> >(
          input, block, blockSize, numberOfInstances);

      vtkm::cont::ArrayHandleStreaming<vtkm::cont::ArrayHandle<T,COut> > streamOut =
          vtkm::cont::ArrayHandleStreaming<vtkm::cont::ArrayHandle<T,COut> >(
          output, block, blockSize, numberOfInstances);

      if (block == 0)
      {
        streamOut.AllocateFullArray(fullSize);
        lastResult = DerivedAlgorithm::ScanExclusive(streamIn, streamOut, binary_functor, initialValue);
      }
      else
      {
        lastResult = DerivedAlgorithm::ScanExclusive(streamIn, streamOut, binary_functor, lastResult);
      }

      streamOut.SyncControlArray();
    }
    return lastResult;
  }

  //--------------------------------------------------------------------------
  // Scan Inclusive
  template<typename T, class CIn, class COut>
  VTKM_CONT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    return DerivedAlgorithm::ScanInclusive(input,
                                            output,
                                           vtkm::Add());
  }

  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output,
      BinaryFunctor binary_functor)
  {
    typedef typename
        vtkm::cont::ArrayHandle<T,COut>
            ::template ExecutionTypes<DeviceAdapterTag>::Portal PortalType;

    typedef ScanKernel<PortalType,BinaryFunctor> ScanKernelType;

    DerivedAlgorithm::Copy(input, output);

    vtkm::Id numValues = output.GetNumberOfValues();
    if (numValues < 1)
    {
      return vtkm::TypeTraits<T>::ZeroInitialization();
    }

    PortalType portal = output.PrepareForInPlace(DeviceAdapterTag());

    vtkm::Id stride;
    for (stride = 2; stride-1 < numValues; stride *= 2)
    {
      ScanKernelType kernel(portal, binary_functor, stride, stride/2 - 1);
      DerivedAlgorithm::Schedule(kernel, numValues/stride);
    }

    // Do reverse operation on odd indices. Start at stride we were just at.
    for (stride /= 2; stride > 1; stride /= 2)
    {
      ScanKernelType kernel(portal, binary_functor, stride, stride - 1);
      DerivedAlgorithm::Schedule(kernel, numValues/stride);
    }

    return GetExecutionValue(output, numValues-1);
  }

  template<typename T, typename U, class KIn, class VIn, class VOut>
  VTKM_CONT static void ScanInclusiveByKey(
    const vtkm::cont::ArrayHandle<T, KIn> &keys,
    const vtkm::cont::ArrayHandle<U, VIn> &values,
    vtkm::cont::ArrayHandle<U, VOut> &values_output)
  {
    return DerivedAlgorithm::ScanInclusiveByKey(keys,
                                                values,
                                                values_output,
                                                vtkm::Add());
  }

  template<typename T, typename U, class KIn, class VIn, class VOut,
    class BinaryFunctor>
  VTKM_CONT static void ScanInclusiveByKey(
    const vtkm::cont::ArrayHandle<T, KIn> &keys,
    const vtkm::cont::ArrayHandle<U, VIn> &values,
    vtkm::cont::ArrayHandle<U, VOut> &values_output,
    BinaryFunctor binary_functor)
  {
    VTKM_ASSERT(keys.GetNumberOfValues() == values.GetNumberOfValues());
    const vtkm::Id numberOfKeys = keys.GetNumberOfValues();

    if(numberOfKeys <= 1)
    { //we only have a single key/value so that is our output
      DerivedAlgorithm::Copy(values, values_output);
      return;
    }

    //we need to determine based on the keys what is the keystate for
    //each key. The states are start, middle, end of a series and the special
    //state start and end of a series
    vtkm::cont::ArrayHandle< ReduceKeySeriesStates > keystate;

    {
      typedef typename vtkm::cont::ArrayHandle<T,KIn>::template ExecutionTypes<DeviceAdapterTag>
      ::PortalConst InputPortalType;

      typedef typename vtkm::cont::ArrayHandle< ReduceKeySeriesStates >::template ExecutionTypes<DeviceAdapterTag>
      ::Portal KeyStatePortalType;

      InputPortalType inputPortal = keys.PrepareForInput(DeviceAdapterTag());
      KeyStatePortalType keyStatePortal = keystate.PrepareForOutput(numberOfKeys,
                                                                    DeviceAdapterTag());
      ReduceStencilGeneration<InputPortalType, KeyStatePortalType> kernel(inputPortal, keyStatePortal);
      DerivedAlgorithm::Schedule(kernel, numberOfKeys);
    }

    //next step is we need to reduce the values for each key. This is done
    //by running an inclusive scan over the values array using the stencil.
    //
    // this inclusive scan will write out two values, the first being
    // the value summed currently, the second being 0 or 1, with 1 being used
    // when this is a value of a key we need to write ( END or START_AND_END)
    {
      typedef vtkm::cont::ArrayHandle<U, VIn> ValueInHandleType;
      typedef vtkm::cont::ArrayHandle<U, VOut> ValueOutHandleType;
      typedef vtkm::cont::ArrayHandle<ReduceKeySeriesStates> StencilHandleType;
      typedef vtkm::cont::ArrayHandleZip<ValueInHandleType,
        StencilHandleType> ZipInHandleType;
      typedef vtkm::cont::ArrayHandleZip<ValueOutHandleType,
        StencilHandleType> ZipOutHandleType;

      StencilHandleType stencil;
      ValueOutHandleType reducedValues;

      ZipInHandleType scanInput(values, keystate);
      ZipOutHandleType scanOutput(reducedValues, stencil);

      DerivedAlgorithm::ScanInclusive(scanInput,
                                      scanOutput,
                                      ReduceByKeyAdd<BinaryFunctor>(
                                        binary_functor));
      //at this point we are done with keystate, so free the memory
      keystate.ReleaseResources();
      DerivedAlgorithm::Copy(reducedValues, values_output);
    }
  }

  //--------------------------------------------------------------------------
  // Sort
  template<typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Sort(
      vtkm::cont::ArrayHandle<T,Storage> &values,
      BinaryCompare binary_compare)
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

    typedef BitonicSortMergeKernel<PortalType,BinaryCompare> MergeKernel;
    typedef BitonicSortCrossoverKernel<PortalType,BinaryCompare> CrossoverKernel;

    for (vtkm::Id crossoverSize = 1;
         crossoverSize < numValues;
         crossoverSize *= 2)
    {
      DerivedAlgorithm::Schedule(CrossoverKernel(portal,binary_compare,crossoverSize),
                                 numThreads);
      for (vtkm::Id mergeSize = crossoverSize/2; mergeSize > 0; mergeSize /= 2)
      {
        DerivedAlgorithm::Schedule(MergeKernel(portal,binary_compare,mergeSize),
                                   numThreads);
      }
    }
  }

  template<typename T, class Storage>
  VTKM_CONT static void Sort(
      vtkm::cont::ArrayHandle<T,Storage> &values)
  {
    DerivedAlgorithm::Sort(values, DefaultCompareFunctor());
  }

  //--------------------------------------------------------------------------
  // Sort by Key
public:

  template<typename T, typename U, class StorageT,  class StorageU>
  VTKM_CONT static void SortByKey(
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
    DerivedAlgorithm::Sort(zipHandle,internal::KeyCompare<T,U>());
  }

  template<typename T, typename U, class StorageT,  class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKey(
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
    DerivedAlgorithm::Sort(zipHandle,internal::KeyCompare<T,U,BinaryCompare>(binary_compare));
  }

  //--------------------------------------------------------------------------
  // Unique
  template<typename T, class Storage>
  VTKM_CONT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values)
  {
    Unique(values, std::equal_to<T>());
  }

  template<typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values,
      BinaryCompare binary_compare)
  {
    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>
        stencilArray;
    vtkm::Id inputSize = values.GetNumberOfValues();

    typedef internal::WrappedBinaryOperator<bool,BinaryCompare> WrappedBOpType;
    WrappedBOpType wrappedCompare(binary_compare);

    ClassifyUniqueComparisonKernel<
        typename vtkm::cont::ArrayHandle<T,Storage>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<vtkm::Id,vtkm::cont::StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::Portal,
        WrappedBOpType>
        classifyKernel(values.PrepareForInput(DeviceAdapterTag()),
                       stencilArray.PrepareForOutput(inputSize, DeviceAdapterTag()),
                       wrappedCompare);

    DerivedAlgorithm::Schedule(classifyKernel, inputSize);

    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>
        outputArray;

    DerivedAlgorithm::CopyIf(values, stencilArray, outputArray);

    values.Allocate(outputArray.GetNumberOfValues());
    DerivedAlgorithm::Copy(outputArray, values);
  }

  //--------------------------------------------------------------------------
  // Upper bounds
  template<typename T, class CIn, class CVal, class COut>
  VTKM_CONT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      const vtkm::cont::ArrayHandle<T,CVal> &values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &output)
  {
    vtkm::Id arraySize = values.GetNumberOfValues();

    UpperBoundsKernel<
        typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<T,CVal>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<vtkm::Id,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal>
        kernel(input.PrepareForInput(DeviceAdapterTag()),
               values.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(arraySize, DeviceAdapterTag()));

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template<typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      const vtkm::cont::ArrayHandle<T,CVal> &values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &output,
      BinaryCompare binary_compare)
  {
    vtkm::Id arraySize = values.GetNumberOfValues();

    UpperBoundsKernelComparisonKernel<
        typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<T,CVal>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<vtkm::Id,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal,
        BinaryCompare>
        kernel(input.PrepareForInput(DeviceAdapterTag()),
               values.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(arraySize, DeviceAdapterTag()),
               binary_compare);

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template<class CIn, class COut>
  VTKM_CONT static void UpperBounds(
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

namespace vtkm {
namespace cont {
/// \brief Class providing a device-specific atomic interface.
///
/// The class provide the actual implementation used by vtkm::exec::AtomicArray.
/// A serial default implementation is provided. But each device will have a different
/// implementation.
///
/// Serial requires no form of atomicity
///
template<typename T, typename DeviceTag>
class DeviceAdapterAtomicArrayImplementation
{
public:
  VTKM_CONT
  DeviceAdapterAtomicArrayImplementation(
               vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> handle):
    Iterators( IteratorsType( handle.PrepareForInPlace(DeviceTag()) ) )
  {
  }

  VTKM_EXEC
  T Add(vtkm::Id index, const T& value) const
  {
    T* lockedValue;
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL > 0
    typedef typename vtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType IteratorType;
    typename IteratorType::pointer temp =
        &(*(Iterators.GetBegin() + static_cast<std::ptrdiff_t>(index)));
    lockedValue = temp;
    return vtkmAtomicAdd(lockedValue, value);
#else
    lockedValue = (Iterators.GetBegin()+index);
    return vtkmAtomicAdd(lockedValue, value);
#endif
  }

  VTKM_EXEC
  T CompareAndSwap(vtkm::Id index, const T& newValue, const T& oldValue) const
  {
    T* lockedValue;
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL > 0
    typedef typename vtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType IteratorType;
    typename IteratorType::pointer temp =
        &(*(Iterators.GetBegin()+static_cast<std::ptrdiff_t>(index)));
    lockedValue = temp;
    return vtkmCompareAndSwap(lockedValue, newValue, oldValue);
#else
    lockedValue = (Iterators.GetBegin()+index);
    return vtkmCompareAndSwap(lockedValue, newValue, oldValue);
#endif
  }

private:
  typedef typename vtkm::cont::ArrayHandle<T,vtkm::cont::StorageTagBasic>
        ::template ExecutionTypes<DeviceTag>::Portal PortalType;
  typedef vtkm::cont::ArrayPortalToIterators<PortalType> IteratorsType;
  IteratorsType Iterators;

#if defined(VTKM_MSVC) //MSVC atomics
  VTKM_EXEC
  vtkm::Int32 vtkmAtomicAdd(vtkm::Int32 *address, const vtkm::Int32 &value) const
  {
    return InterlockedExchangeAdd(reinterpret_cast<volatile long *>(address),value);
  }

  VTKM_EXEC
  vtkm::Int64 vtkmAtomicAdd(vtkm::Int64 *address, const vtkm::Int64 &value) const
  {
    return InterlockedExchangeAdd64(reinterpret_cast<volatile long long *>(address),value);
  }

  VTKM_EXEC
  vtkm::Int32 vtkmCompareAndSwap(vtkm::Int32 *address, const vtkm::Int32 &newValue, const vtkm::Int32 &oldValue) const
  {
    return InterlockedCompareExchange(reinterpret_cast<volatile long *>(address),newValue,oldValue);
  }

  VTKM_EXEC
  vtkm::Int64 vtkmCompareAndSwap(vtkm::Int64 *address,const vtkm::Int64 &newValue, const vtkm::Int64 &oldValue) const
  {
    return InterlockedCompareExchange64(reinterpret_cast<volatile long long *>(address),newValue, oldValue);
  }

#else //gcc built-in atomics

  VTKM_EXEC
  vtkm::Int32 vtkmAtomicAdd(vtkm::Int32 *address, const vtkm::Int32 &value) const
  {
    return __sync_fetch_and_add(address,value);
  }

  VTKM_EXEC
  vtkm::Int64 vtkmAtomicAdd(vtkm::Int64 *address, const vtkm::Int64 &value) const
  {
    return __sync_fetch_and_add(address,value);
  }

  VTKM_EXEC
  vtkm::Int32 vtkmCompareAndSwap(vtkm::Int32 *address, const vtkm::Int32 &newValue, const vtkm::Int32 &oldValue) const
  {
    return __sync_val_compare_and_swap(address,oldValue, newValue);
  }

  VTKM_EXEC
  vtkm::Int64 vtkmCompareAndSwap(vtkm::Int64 *address,const vtkm::Int64 &newValue, const vtkm::Int64 &oldValue) const
  {
    return __sync_val_compare_and_swap(address,oldValue,newValue);
  }

#endif
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_internal_DeviceAdapterAlgorithmGeneral_h
