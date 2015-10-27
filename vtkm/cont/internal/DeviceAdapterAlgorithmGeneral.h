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
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/internal/FunctorsGeneral.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>

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


public:
  //--------------------------------------------------------------------------
  // Copy
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

  template<typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT_EXPORT static void LowerBounds(
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
 template<typename T, class CIn>
  VTKM_CONT_EXPORT static T Reduce(
      const vtkm::cont::ArrayHandle<T,CIn> &input, T initialValue)
  {
    return DerivedAlgorithm::Reduce(input, initialValue, vtkm::internal::Add());
  }

 template<typename T, class CIn, class BinaryFunctor>
  VTKM_CONT_EXPORT static T Reduce(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      T initialValue,
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
            BinaryFunctor
            > ReduceKernelType;

    typedef vtkm::cont::ArrayHandleImplicit<
                                            T,
                                            ReduceKernelType > ReduceHandleType;
    typedef vtkm::cont::ArrayHandle<
                                    T,
                                    vtkm::cont::StorageTagBasic> TempArrayType;

    ReduceKernelType kernel(input.PrepareForInput( DeviceAdapterTag() ),
                            binary_functor);

    vtkm::Id length = (input.GetNumberOfValues() / 16);
    length += (input.GetNumberOfValues() % 16 == 0) ? 0 : 1;
    ReduceHandleType reduced = vtkm::cont::make_ArrayHandleImplicit<T>(kernel,
                                                                       length);

    TempArrayType inclusiveScanStorage;
    T scanResult = DerivedAlgorithm::ScanInclusive(reduced,
                                                   inclusiveScanStorage,
                                                   binary_functor);
    return binary_functor(initialValue, scanResult);
  }

  //--------------------------------------------------------------------------
  // Reduce By Key
  template<typename T, typename U, class KIn, class VIn, class KOut, class VOut,
          class BinaryFunctor>
  VTKM_CONT_EXPORT static void ReduceByKey(
      const vtkm::cont::ArrayHandle<T,KIn> &keys,
      const vtkm::cont::ArrayHandle<U,VIn> &values,
      vtkm::cont::ArrayHandle<T,KOut> &keys_output,
      vtkm::cont::ArrayHandle<U,VOut> &values_output,
      BinaryFunctor binary_functor)
  {
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
  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
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
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    return ScanExclusive(input, output, vtkm::Sum(),
                         vtkm::TypeTraits<T>::ZeroInitialization());
  }

  //--------------------------------------------------------------------------
  // Scan Inclusive
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    return DerivedAlgorithm::ScanInclusive(input,
                                            output,
                                            vtkm::internal::Add());
  }

  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT_EXPORT static T ScanInclusive(
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
      return output.GetPortalConstControl().Get(0);
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

  //--------------------------------------------------------------------------
  // Sort
  template<typename T, class Storage, class BinaryCompare>
  VTKM_CONT_EXPORT static void Sort(
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
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Storage> &values)
  {
    DerivedAlgorithm::Sort(values, DefaultCompareFunctor());
  }

  //--------------------------------------------------------------------------
  // Sort by Key
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
    DerivedAlgorithm::Sort(zipHandle,internal::KeyCompare<T,U>());
  }

  template<typename T, typename U, class StorageT,  class StorageU, class BinaryCompare>
  VTKM_CONT_EXPORT static void SortByKey(
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
  // Stream Compact
  template<typename T, typename U, class CIn, class CStencil,
           class COut, class UnaryPredicate>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      const vtkm::cont::ArrayHandle<U,CStencil>& stencil,
      vtkm::cont::ArrayHandle<T,COut>& output,
      UnaryPredicate unary_predicate)
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
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      const vtkm::cont::ArrayHandle<U,CStencil>& stencil,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    ::vtkm::NotZeroInitialized unary_predicate;
    DerivedAlgorithm::StreamCompact(input, stencil, output, unary_predicate);
  }

  template<typename T, class CStencil, class COut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,CStencil> &stencil,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &output)
  {
    vtkm::cont::ArrayHandleIndex input(stencil.GetNumberOfValues());
    DerivedAlgorithm::StreamCompact(input, stencil, output);
  }

  //--------------------------------------------------------------------------
  // Unique
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

  template<typename T, class Storage, class BinaryCompare>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values,
      BinaryCompare binary_compare)
  {
    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>
        stencilArray;
    vtkm::Id inputSize = values.GetNumberOfValues();

    ClassifyUniqueComparisonKernel<
        typename vtkm::cont::ArrayHandle<T,Storage>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<vtkm::Id,vtkm::cont::StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::Portal,
        BinaryCompare>
        classifyKernel(values.PrepareForInput(DeviceAdapterTag()),
                       stencilArray.PrepareForOutput(inputSize, DeviceAdapterTag()),
                       binary_compare);
    DerivedAlgorithm::Schedule(classifyKernel, inputSize);

    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>
        outputArray;

    DerivedAlgorithm::StreamCompact(values, stencilArray, outputArray);

    DerivedAlgorithm::Copy(outputArray, values);
  }

  //--------------------------------------------------------------------------
  // Upper bounds
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
        typename vtkm::cont::ArrayHandle<vtkm::Id,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal>
        kernel(input.PrepareForInput(DeviceAdapterTag()),
               values.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(arraySize, DeviceAdapterTag()));

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template<typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT_EXPORT static void UpperBounds(
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
