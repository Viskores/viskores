//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_serial_internal_DeviceAdapterAlgorithmSerial_h
#define vtk_m_cont_serial_internal_DeviceAdapterAlgorithmSerial_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

#include <vtkm/BinaryOperators.h>

#include <vtkm/exec/serial/internal/TaskTiling.h>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <type_traits>

namespace vtkm
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagSerial>
  : vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagSerial>,
      vtkm::cont::DeviceAdapterTagSerial>
{
private:
  using Device = vtkm::cont::DeviceAdapterTagSerial;

  // MSVC likes complain about narrowing type conversions in std::copy and
  // provides no reasonable way to disable the warning. As a work-around, this
  // template calls std::copy if and only if the types match, otherwise falls
  // back to a iterative casting approach. Since std::copy can only really
  // optimize same-type copies, this shouldn't affect performance.
  template <typename InIter, typename OutIter>
  static void DoCopy(InIter src, InIter srcEnd, OutIter dst, std::false_type)
  {
    using OutputType = typename std::iterator_traits<OutIter>::value_type;
    while (src != srcEnd)
    {
      *dst = static_cast<OutputType>(*src);
      ++src;
      ++dst;
    }
  }

  template <typename InIter, typename OutIter>
  static void DoCopy(InIter src, InIter srcEnd, OutIter dst, std::true_type)
  {
    std::copy(src, srcEnd, dst);
  }

public:
  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static void Copy(const vtkm::cont::ArrayHandle<T, CIn>& input,
                             vtkm::cont::ArrayHandle<U, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id inSize = input.GetNumberOfValues();
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagSerial());
    auto outputPortal = output.PrepareForOutput(inSize, DeviceAdapterTagSerial());

    if (inSize <= 0)
    {
      return;
    }

    using InputType = decltype(inputPortal.Get(0));
    using OutputType = decltype(outputPortal.Get(0));

    DoCopy(vtkm::cont::ArrayPortalToIteratorBegin(inputPortal),
           vtkm::cont::ArrayPortalToIteratorEnd(inputPortal),
           vtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
           std::is_same<InputType, OutputType>());
  }

  template <typename T, typename U, class CIn, class CStencil, class COut>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    ::vtkm::NotZeroInitialized unary_predicate;
    CopyIf(input, stencil, output, unary_predicate);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output,
                               UnaryPredicate predicate)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id inputSize = input.GetNumberOfValues();
    VTKM_ASSERT(inputSize == stencil.GetNumberOfValues());

    auto inputPortal = input.PrepareForInput(DeviceAdapterTagSerial());
    auto stencilPortal = stencil.PrepareForInput(DeviceAdapterTagSerial());
    auto outputPortal = output.PrepareForOutput(inputSize, DeviceAdapterTagSerial());

    vtkm::Id readPos = 0;
    vtkm::Id writePos = 0;

    for (; readPos < inputSize; ++readPos)
    {
      if (predicate(stencilPortal.Get(readPos)))
      {
        outputPortal.Set(writePos, inputPortal.Get(readPos));
        ++writePos;
      }
    }

    output.Shrink(writePos);
  }

  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static bool CopySubRange(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                     vtkm::Id inputStartIndex,
                                     vtkm::Id numberOfElementsToCopy,
                                     vtkm::cont::ArrayHandle<U, COut>& output,
                                     vtkm::Id outputIndex = 0)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id inSize = input.GetNumberOfValues();

    // Check if the ranges overlap and fail if they do.
    if (input == output && ((outputIndex >= inputStartIndex &&
                             outputIndex < inputStartIndex + numberOfElementsToCopy) ||
                            (inputStartIndex >= outputIndex &&
                             inputStartIndex < outputIndex + numberOfElementsToCopy)))
    {
      return false;
    }

    if (inputStartIndex < 0 || numberOfElementsToCopy < 0 || outputIndex < 0 ||
        inputStartIndex >= inSize)
    { //invalid parameters
      return false;
    }

    //determine if the numberOfElementsToCopy needs to be reduced
    if (inSize < (inputStartIndex + numberOfElementsToCopy))
    { //adjust the size
      numberOfElementsToCopy = (inSize - inputStartIndex);
    }

    const vtkm::Id outSize = output.GetNumberOfValues();
    const vtkm::Id copyOutEnd = outputIndex + numberOfElementsToCopy;
    if (outSize < copyOutEnd)
    { //output is not large enough
      if (outSize == 0)
      { //since output has nothing, just need to allocate to correct length
        output.Allocate(copyOutEnd);
      }
      else
      { //we currently have data in this array, so preserve it in the new
        //resized array
        vtkm::cont::ArrayHandle<U, COut> temp;
        temp.Allocate(copyOutEnd);
        CopySubRange(output, 0, outSize, temp);
        output = temp;
      }
    }

    auto inputPortal = input.PrepareForInput(DeviceAdapterTagSerial());
    auto outputPortal = output.PrepareForInPlace(DeviceAdapterTagSerial());
    auto inIter = vtkm::cont::ArrayPortalToIteratorBegin(inputPortal);
    auto outIter = vtkm::cont::ArrayPortalToIteratorBegin(outputPortal);

    using InputType = decltype(inputPortal.Get(0));
    using OutputType = decltype(outputPortal.Get(0));

    DoCopy(inIter + inputStartIndex,
           inIter + inputStartIndex + numberOfElementsToCopy,
           outIter + outputIndex,
           std::is_same<InputType, OutputType>());

    return true;
  }

  template <typename T, typename U, class CIn>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    return Reduce(input, initialValue, vtkm::Add());
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    internal::WrappedBinaryOperator<U, BinaryFunctor> wrappedOp(binary_functor);
    auto inputPortal = input.PrepareForInput(Device());
    return std::accumulate(vtkm::cont::ArrayPortalToIteratorBegin(inputPortal),
                           vtkm::cont::ArrayPortalToIteratorEnd(inputPortal),
                           initialValue,
                           wrappedOp);
  }

  template <typename T,
            typename U,
            class KIn,
            class VIn,
            class KOut,
            class VOut,
            class BinaryFunctor>
  VTKM_CONT static void ReduceByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                    const vtkm::cont::ArrayHandle<U, VIn>& values,
                                    vtkm::cont::ArrayHandle<T, KOut>& keys_output,
                                    vtkm::cont::ArrayHandle<U, VOut>& values_output,
                                    BinaryFunctor binary_functor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    auto keysPortalIn = keys.PrepareForInput(Device());
    auto valuesPortalIn = values.PrepareForInput(Device());
    const vtkm::Id numberOfKeys = keys.GetNumberOfValues();

    VTKM_ASSERT(numberOfKeys == values.GetNumberOfValues());
    if (numberOfKeys == 0)
    {
      keys_output.Shrink(0);
      values_output.Shrink(0);
      return;
    }

    auto keysPortalOut = keys_output.PrepareForOutput(numberOfKeys, Device());
    auto valuesPortalOut = values_output.PrepareForOutput(numberOfKeys, Device());

    vtkm::Id writePos = 0;
    vtkm::Id readPos = 0;

    T currentKey = keysPortalIn.Get(readPos);
    U currentValue = valuesPortalIn.Get(readPos);

    for (++readPos; readPos < numberOfKeys; ++readPos)
    {
      while (readPos < numberOfKeys && currentKey == keysPortalIn.Get(readPos))
      {
        currentValue = binary_functor(currentValue, valuesPortalIn.Get(readPos));
        ++readPos;
      }

      if (readPos < numberOfKeys)
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
    keys_output.Shrink(writePos + 1);
    values_output.Shrink(writePos + 1);
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    internal::WrappedBinaryOperator<T, BinaryFunctor> wrappedBinaryOp(binary_functor);

    vtkm::Id numberOfValues = input.GetNumberOfValues();

    auto inputPortal = input.PrepareForInput(Device());
    auto outputPortal = output.PrepareForOutput(numberOfValues, Device());

    if (numberOfValues <= 0)
    {
      return vtkm::TypeTraits<T>::ZeroInitialization();
    }

    std::partial_sum(vtkm::cont::ArrayPortalToIteratorBegin(inputPortal),
                     vtkm::cont::ArrayPortalToIteratorEnd(inputPortal),
                     vtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
                     wrappedBinaryOp);

    // Return the value at the last index in the array, which is the full sum.
    return outputPortal.Get(numberOfValues - 1);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    return ScanInclusive(input, output, vtkm::Sum());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor,
                                   const T& initialValue)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    internal::WrappedBinaryOperator<T, BinaryFunctor> wrappedBinaryOp(binaryFunctor);

    vtkm::Id numberOfValues = input.GetNumberOfValues();

    auto inputPortal = input.PrepareForInput(Device());
    auto outputPortal = output.PrepareForOutput(numberOfValues, Device());

    if (numberOfValues <= 0)
    {
      return initialValue;
    }

    // Shift right by one, by iterating backwards. We are required to iterate
    //backwards so that the algorithm works correctly when the input and output
    //are the same array, otherwise you just propagate the first element
    //to all elements
    //Note: We explicitly do not use std::copy_backwards for good reason.
    //The ICC compiler has been found to improperly optimize the copy_backwards
    //into a standard copy, causing the above issue.
    T lastValue = inputPortal.Get(numberOfValues - 1);
    for (vtkm::Id i = (numberOfValues - 1); i >= 1; --i)
    {
      outputPortal.Set(i, inputPortal.Get(i - 1));
    }
    outputPortal.Set(0, initialValue);

    std::partial_sum(vtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
                     vtkm::cont::ArrayPortalToIteratorEnd(outputPortal),
                     vtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
                     wrappedBinaryOp);

    return wrappedBinaryOp(outputPortal.Get(numberOfValues - 1), lastValue);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    return ScanExclusive(input, output, vtkm::Sum(), vtkm::TypeTraits<T>::ZeroInitialization());
  }

  VTKM_CONT_EXPORT static void ScheduleTask(vtkm::exec::serial::internal::TaskTiling1D& functor,
                                            vtkm::Id size);
  VTKM_CONT_EXPORT static void ScheduleTask(vtkm::exec::serial::internal::TaskTiling3D& functor,
                                            vtkm::Id3 size);

  template <class FunctorType>
  VTKM_CONT static inline void Schedule(FunctorType functor, vtkm::Id size)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::exec::serial::internal::TaskTiling1D kernel(functor);
    ScheduleTask(kernel, size);
  }

  template <class FunctorType>
  VTKM_CONT static inline void Schedule(FunctorType functor, vtkm::Id3 size)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::exec::serial::internal::TaskTiling3D kernel(functor);
    ScheduleTask(kernel, size);
  }

private:
  template <typename Vin,
            typename I,
            typename Vout,
            class StorageVin,
            class StorageI,
            class StorageVout>
  VTKM_CONT static void Scatter(vtkm::cont::ArrayHandle<Vin, StorageVin>& values,
                                vtkm::cont::ArrayHandle<I, StorageI>& index,
                                vtkm::cont::ArrayHandle<Vout, StorageVout>& values_out)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id n = values.GetNumberOfValues();
    VTKM_ASSERT(n == index.GetNumberOfValues());

    auto valuesPortal = values.PrepareForInput(Device());
    auto indexPortal = index.PrepareForInput(Device());
    auto valuesOutPortal = values_out.PrepareForOutput(n, Device());

    for (vtkm::Id i = 0; i < n; i++)
    {
      valuesOutPortal.Set(i, valuesPortal.Get(indexPortal.Get(i)));
    }
  }

private:
  /// Reorder the value array along with the sorting algorithm
  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKeyDirect(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                        vtkm::cont::ArrayHandle<U, StorageU>& values,
                                        BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    //combine the keys and values into a ZipArrayHandle
    //we than need to specify a custom compare function wrapper
    //that only checks for key side of the pair, using the custom compare
    //functor that the user passed in
    auto zipHandle = vtkm::cont::make_ArrayHandleZip(keys, values);
    Sort(zipHandle, internal::KeyCompare<T, U, BinaryCompare>(binary_compare));
  }

public:
  template <typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    SortByKey(keys, values, std::less<T>());
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values,
                                  const BinaryCompare& binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
    constexpr bool larger_than_64bits = sizeof(U) > sizeof(vtkm::Int64);
    if (larger_than_64bits)
    {
      /// More efficient sort:
      /// Move value indexes when sorting and reorder the value array at last
      vtkm::cont::ArrayHandle<vtkm::Id> indexArray;
      vtkm::cont::ArrayHandle<U, StorageU> valuesScattered;

      Copy(ArrayHandleIndex(keys.GetNumberOfValues()), indexArray);
      SortByKeyDirect(keys, indexArray, wrappedCompare);
      Scatter(values, indexArray, valuesScattered);
      Copy(valuesScattered, values);
    }
    else
    {
      SortByKeyDirect(keys, values, wrappedCompare);
    }
  }

  template <typename T, class Storage>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    Sort(values, std::less<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    auto arrayPortal = values.PrepareForInPlace(Device());
    vtkm::cont::ArrayPortalToIterators<decltype(arrayPortal)> iterators(arrayPortal);

    internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
    std::sort(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
  }

  template <typename T, class Storage>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    Unique(values, std::equal_to<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    auto arrayPortal = values.PrepareForInPlace(Device());
    vtkm::cont::ArrayPortalToIterators<decltype(arrayPortal)> iterators(arrayPortal);
    internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);

    auto end = std::unique(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
    values.Shrink(static_cast<vtkm::Id>(end - iterators.GetBegin()));
  }

  VTKM_CONT static void Synchronize()
  {
    // Nothing to do. This device is serial and has no asynchronous operations.
  }
};

template <>
class DeviceTaskTypes<vtkm::cont::DeviceAdapterTagSerial>
{
public:
  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::serial::internal::TaskTiling1D MakeTask(WorkletType& worklet,
                                                             InvocationType& invocation,
                                                             vtkm::Id,
                                                             vtkm::Id globalIndexOffset = 0)
  {
    return vtkm::exec::serial::internal::TaskTiling1D(worklet, invocation, globalIndexOffset);
  }

  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::serial::internal::TaskTiling3D MakeTask(WorkletType& worklet,
                                                             InvocationType& invocation,
                                                             vtkm::Id3,
                                                             vtkm::Id globalIndexOffset = 0)
  {
    return vtkm::exec::serial::internal::TaskTiling3D(worklet, invocation, globalIndexOffset);
  }
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_serial_internal_DeviceAdapterAlgorithmSerial_h
