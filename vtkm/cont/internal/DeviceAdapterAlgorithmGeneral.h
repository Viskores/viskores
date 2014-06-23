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
#ifndef vtk_m_cont_internal_DeviceAdapterAlgorithmGeneral_h
#define vtk_m_cont_internal_DeviceAdapterAlgorithmGeneral_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>

#include <algorithm>

namespace {
/// Predicate that takes a single argument \c x, and returns
/// True if it isn't the identity of the Type \p T.
template<typename T>
struct not_default_constructor
{
  VTKM_EXEC_CONT_EXPORT bool operator()(const T &x)
  {
    return (x  != T());
  }
};

}

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
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static void Copy(const vtkm::cont::ArrayHandle<T, CIn> &input,
                                    vtkm::cont::ArrayHandle<T, COut> &output)
  {
    vtkm::Id arraySize = input.GetNumberOfValues();

    CopyKernel<
        typename vtkm::cont::ArrayHandle<T,CIn>::template ExecutionTypes<DeviceAdapterTag>::PortalConst,
        typename vtkm::cont::ArrayHandle<T,COut>::template ExecutionTypes<DeviceAdapterTag>::Portal>
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

      typename InputPortalType::IteratorType resultPos =
          std::lower_bound(this->InputPortal.GetIteratorBegin(),
                           this->InputPortal.GetIteratorEnd(),
                           this->ValuesPortal.Get(index));

      vtkm::Id resultIndex =
          static_cast<vtkm::Id>(
            std::distance(this->InputPortal.GetIteratorBegin(), resultPos));
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

      typename InputPortalType::IteratorType resultPos =
          std::lower_bound(this->InputPortal.GetIteratorBegin(),
                           this->InputPortal.GetIteratorEnd(),
                           this->ValuesPortal.Get(index),
                           this->CompareFunctor);

      vtkm::Id resultIndex =
          static_cast<vtkm::Id>(
            std::distance(this->InputPortal.GetIteratorBegin(), resultPos));
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
  template<typename PortalType>
  struct ScanKernel : vtkm::exec::FunctorBase
  {
    PortalType Portal;
    vtkm::Id Stride;
    vtkm::Id Offset;
    vtkm::Id Distance;

    VTKM_CONT_EXPORT
    ScanKernel(const PortalType &portal, vtkm::Id stride, vtkm::Id offset)
      : Portal(portal),
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
        this->Portal.Set(rightIndex, leftValue+rightValue);
      }
    }
  };

public:
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output)
  {
    typedef typename
        vtkm::cont::ArrayHandle<T,COut>
            ::template ExecutionTypes<DeviceAdapterTag>::Portal PortalType;

    DerivedAlgorithm::Copy(input, output);

    vtkm::Id numValues = output.GetNumberOfValues();
    if (numValues < 1)
    {
      return 0;
    }

    PortalType portal = output.PrepareForInPlace(DeviceAdapterTag());

    vtkm::Id stride;
    for (stride = 2; stride-1 < numValues; stride *= 2)
    {
      ScanKernel<PortalType> kernel(portal, stride, stride/2 - 1);
      DerivedAlgorithm::Schedule(kernel, numValues/stride);
    }

    // Do reverse operation on odd indices. Start at stride we were just at.
    for (stride /= 2; stride > 1; stride /= 2)
    {
      ScanKernel<PortalType> kernel(portal, stride, stride - 1);
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
  // Stream Compact
private:
  template<class StencilPortalType, class OutputPortalType>
  struct StencilToIndexFlagKernel
  {
    typedef typename StencilPortalType::ValueType StencilValueType;
    StencilPortalType StencilPortal;
    OutputPortalType OutputPortal;

    VTKM_CONT_EXPORT
    StencilToIndexFlagKernel(StencilPortalType stencilPortal,
                             OutputPortalType outputPortal)
      : StencilPortal(stencilPortal), OutputPortal(outputPortal) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      StencilValueType value = this->StencilPortal.Get(index);
      bool flag = not_default_constructor<StencilValueType>()(value);
      this->OutputPortal.Set(index, flag ? 1 : 0);
    }

    VTKM_CONT_EXPORT
    void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer &)
    {  }
  };

  template<class InputPortalType,
           class StencilPortalType,
           class IndexPortalType,
           class OutputPortalType>
  struct CopyIfKernel
  {
    InputPortalType InputPortal;
    StencilPortalType StencilPortal;
    IndexPortalType IndexPortal;
    OutputPortalType OutputPortal;

    VTKM_CONT_EXPORT
    CopyIfKernel(InputPortalType inputPortal,
                 StencilPortalType stencilPortal,
                 IndexPortalType indexPortal,
                 OutputPortalType outputPortal)
      : InputPortal(inputPortal),
        StencilPortal(stencilPortal),
        IndexPortal(indexPortal),
        OutputPortal(outputPortal) {  }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id index) const
    {
      typedef typename StencilPortalType::ValueType StencilValueType;
      StencilValueType stencilValue = this->StencilPortal.Get(index);
      if (not_default_constructor<StencilValueType>()(stencilValue))
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

  template<typename T, typename U, class CIn, class CStencil, class COut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      const vtkm::cont::ArrayHandle<U,CStencil>& stencil,
      vtkm::cont::ArrayHandle<T,COut>& output)
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

    StencilToIndexFlagKernel<
        StencilPortalType, IndexPortalType> indexKernel(stencilPortal,
                                                        indexPortal);

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
        OutputPortalType>copyKernel(inputPortal,
                                    stencilPortal,
                                    indexPortal,
                                    outputPortal);
    DerivedAlgorithm::Schedule(copyKernel, arrayLength);
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

      typename InputPortalType::IteratorType resultPos =
          std::upper_bound(this->InputPortal.GetIteratorBegin(),
                           this->InputPortal.GetIteratorEnd(),
                           this->ValuesPortal.Get(index));

      vtkm::Id resultIndex =
          static_cast<vtkm::Id>(
            std::distance(this->InputPortal.GetIteratorBegin(), resultPos));
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

      typename InputPortalType::IteratorType resultPos =
          std::upper_bound(this->InputPortal.GetIteratorBegin(),
                           this->InputPortal.GetIteratorEnd(),
                           this->ValuesPortal.Get(index),
                           this->CompareFunctor);

      vtkm::Id resultIndex =
          static_cast<vtkm::Id>(
            std::distance(this->InputPortal.GetIteratorBegin(), resultPos));
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
