//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_internal_FunctorsGeneral_h
#define vtk_m_cont_internal_FunctorsGeneral_h

#include <vtkm/BinaryOperators.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/UnaryPredicates.h>
#include <vtkm/cont/ArrayPortalToIterators.h>

#include <vtkm/exec/FunctorBase.h>

#include <algorithm>

namespace vtkm
{
namespace cont
{
namespace internal
{

// Binary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// IteratorFromArrayPortalValue which happen when passed an input array that
// is implicit.
template <typename ResultType, typename Function>
struct WrappedBinaryOperator
{
  Function m_f;

  VTKM_CONT
  WrappedBinaryOperator(const Function& f)
    : m_f(f)
  {
  }

  template <typename Argument1, typename Argument2>
  VTKM_CONT ResultType operator()(const Argument1& x, const Argument2& y) const
  {
    return m_f(x, y);
  }

  template <typename Argument1, typename Argument2>
  VTKM_CONT ResultType
  operator()(const vtkm::internal::ArrayPortalValueReference<Argument1>& x,
             const vtkm::internal::ArrayPortalValueReference<Argument2>& y) const
  {
    using ValueTypeX = typename vtkm::internal::ArrayPortalValueReference<Argument1>::ValueType;
    using ValueTypeY = typename vtkm::internal::ArrayPortalValueReference<Argument2>::ValueType;
    return m_f((ValueTypeX)x, (ValueTypeY)y);
  }

  template <typename Argument1, typename Argument2>
  VTKM_CONT ResultType
  operator()(const Argument1& x,
             const vtkm::internal::ArrayPortalValueReference<Argument2>& y) const
  {
    using ValueTypeY = typename vtkm::internal::ArrayPortalValueReference<Argument2>::ValueType;
    return m_f(x, (ValueTypeY)y);
  }

  template <typename Argument1, typename Argument2>
  VTKM_CONT ResultType operator()(const vtkm::internal::ArrayPortalValueReference<Argument1>& x,
                                  const Argument2& y) const
  {
    using ValueTypeX = typename vtkm::internal::ArrayPortalValueReference<Argument1>::ValueType;
    return m_f((ValueTypeX)x, y);
  }
};

//needs to be in a location that TBB DeviceAdapterAlgorithm can reach
struct DefaultCompareFunctor
{

  template <typename T>
  VTKM_EXEC bool operator()(const T& first, const T& second) const
  {
    return first < second;
  }
};

//needs to be in a location that TBB DeviceAdapterAlgorithm can reach
template <typename T, typename U, class BinaryCompare = DefaultCompareFunctor>
struct KeyCompare
{
  KeyCompare()
    : CompareFunctor()
  {
  }
  explicit KeyCompare(BinaryCompare c)
    : CompareFunctor(c)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  bool operator()(const vtkm::Pair<T, U>& a, const vtkm::Pair<T, U>& b) const
  {
    return CompareFunctor(a.first, b.first);
  }

private:
  BinaryCompare CompareFunctor;
};

template <typename PortalConstType, typename T, typename BinaryFunctor>
struct ReduceKernel : vtkm::exec::FunctorBase
{
  PortalConstType Portal;
  T InitialValue;
  BinaryFunctor BinaryOperator;
  vtkm::Id PortalLength;

  VTKM_CONT
  ReduceKernel()
    : Portal()
    , InitialValue()
    , BinaryOperator()
    , PortalLength(0)
  {
  }

  VTKM_CONT
  ReduceKernel(const PortalConstType& portal, T initialValue, BinaryFunctor binary_functor)
    : Portal(portal)
    , InitialValue(initialValue)
    , BinaryOperator(binary_functor)
    , PortalLength(portal.GetNumberOfValues())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  T operator()(vtkm::Id index) const
  {
    const vtkm::Id reduceWidth = 16;
    const vtkm::Id offset = index * reduceWidth;

    if (offset + reduceWidth >= this->PortalLength)
    {
      //This will only occur for a single index value, so this is the case
      //that needs to handle the initialValue
      T partialSum = BinaryOperator(this->InitialValue, this->Portal.Get(offset));
      vtkm::Id currentIndex = offset + 1;
      while (currentIndex < this->PortalLength)
      {
        partialSum = BinaryOperator(partialSum, this->Portal.Get(currentIndex));
        ++currentIndex;
      }
      return partialSum;
    }
    else
    {
      //optimize the usecase where all values are valid and we don't
      //need to check that we might go out of bounds
      T partialSum = BinaryOperator(this->Portal.Get(offset), this->Portal.Get(offset + 1));
      for (int i = 2; i < reduceWidth; ++i)
      {
        partialSum = BinaryOperator(partialSum, this->Portal.Get(offset + i));
      }
      return partialSum;
    }
  }
};

struct ReduceKeySeriesStates
{
  bool fStart; // START of a segment
  bool fEnd;   // END of a segment

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ReduceKeySeriesStates(bool start = false, bool end = false)
    : fStart(start)
    , fEnd(end)
  {
  }
};

template <typename InputPortalType, typename KeyStatePortalType>
struct ReduceStencilGeneration : vtkm::exec::FunctorBase
{
  InputPortalType Input;
  KeyStatePortalType KeyState;

  VTKM_CONT
  ReduceStencilGeneration(const InputPortalType& input, const KeyStatePortalType& kstate)
    : Input(input)
    , KeyState(kstate)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id centerIndex) const
  {
    using ValueType = typename InputPortalType::ValueType;
    using KeyStateType = typename KeyStatePortalType::ValueType;

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

    if (centerIndex == 0)
    {
      //this means we are at the start of the array
      //means we are automatically START
      //just need to check if we are END
      const ValueType centerValue = this->Input.Get(centerIndex);
      const ValueType rightValue = this->Input.Get(rightIndex);
      const KeyStateType state = ReduceKeySeriesStates(true, rightValue != centerValue);
      this->KeyState.Set(centerIndex, state);
    }
    else if (rightIndex == this->Input.GetNumberOfValues())
    {
      //this means we are at the end, so we are at least END
      //just need to check if we are START
      const ValueType centerValue = this->Input.Get(centerIndex);
      const ValueType leftValue = this->Input.Get(leftIndex);
      const KeyStateType state = ReduceKeySeriesStates(leftValue != centerValue, true);
      this->KeyState.Set(centerIndex, state);
    }
    else
    {
      const ValueType centerValue = this->Input.Get(centerIndex);
      const bool leftMatches(this->Input.Get(leftIndex) == centerValue);
      const bool rightMatches(this->Input.Get(rightIndex) == centerValue);

      //assume it is the middle, and check for the other use-case
      KeyStateType state = ReduceKeySeriesStates(!leftMatches, !rightMatches);
      this->KeyState.Set(centerIndex, state);
    }
  }
};

template <typename BinaryFunctor>
struct ReduceByKeyAdd
{
  BinaryFunctor BinaryOperator;

  ReduceByKeyAdd(BinaryFunctor binary_functor)
    : BinaryOperator(binary_functor)
  {
  }

  template <typename T>
  vtkm::Pair<T, ReduceKeySeriesStates> operator()(
    const vtkm::Pair<T, ReduceKeySeriesStates>& a,
    const vtkm::Pair<T, ReduceKeySeriesStates>& b) const
  {
    using ReturnType = vtkm::Pair<T, ReduceKeySeriesStates>;
    //need too handle how we are going to add two numbers together
    //based on the keyStates that they have

    // Make it work for parallel inclusive scan.  Will end up with all start bits = 1
    // the following logic should change if you use a different parallel scan algorithm.
    if (!b.second.fStart)
    {
      // if b is not START, then it's safe to sum a & b.
      // Propagate a's start flag to b
      // so that later when b's START bit is set, it means there must exists a START between a and b
      return ReturnType(this->BinaryOperator(a.first, b.first),
                        ReduceKeySeriesStates(a.second.fStart, b.second.fEnd));
    }
    return b;
  }
};

struct ReduceByKeyUnaryStencilOp
{
  bool operator()(ReduceKeySeriesStates keySeriesState) const { return keySeriesState.fEnd; }
};

template <typename T,
          typename InputPortalType,
          typename KeyStatePortalType,
          typename OutputPortalType>
struct ShiftCopyAndInit : vtkm::exec::FunctorBase
{
  InputPortalType Input;
  KeyStatePortalType KeyState;
  OutputPortalType Output;
  T initValue;

  ShiftCopyAndInit(const InputPortalType& _input,
                   const KeyStatePortalType& kstate,
                   OutputPortalType& _output,
                   T _init)
    : Input(_input)
    , KeyState(kstate)
    , Output(_output)
    , initValue(_init)
  {
  }

  void operator()(vtkm::Id index) const
  {
    if (this->KeyState.Get(index).fStart)
    {
      Output.Set(index, initValue);
    }
    else
    {
      Output.Set(index, Input.Get(index - 1));
    }
  }
};

template <class InputPortalType, class OutputPortalType>
struct CopyKernel
{
  InputPortalType InputPortal;
  OutputPortalType OutputPortal;
  vtkm::Id InputOffset;
  vtkm::Id OutputOffset;

  VTKM_CONT
  CopyKernel(InputPortalType inputPortal,
             OutputPortalType outputPortal,
             vtkm::Id inputOffset = 0,
             vtkm::Id outputOffset = 0)
    : InputPortal(inputPortal)
    , OutputPortal(outputPortal)
    , InputOffset(inputOffset)
    , OutputOffset(outputOffset)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void operator()(vtkm::Id index) const
  {
    using ValueType = typename OutputPortalType::ValueType;
    this->OutputPortal.Set(
      index + this->OutputOffset,
      static_cast<ValueType>(this->InputPortal.Get(index + this->InputOffset)));
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType, class ValuesPortalType, class OutputPortalType>
struct LowerBoundsKernel
{
  InputPortalType InputPortal;
  ValuesPortalType ValuesPortal;
  OutputPortalType OutputPortal;

  VTKM_CONT
  LowerBoundsKernel(InputPortalType inputPortal,
                    ValuesPortalType valuesPortal,
                    OutputPortalType outputPortal)
    : InputPortal(inputPortal)
    , ValuesPortal(valuesPortal)
    , OutputPortal(outputPortal)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    // This method assumes that (1) InputPortalType can return working
    // iterators in the execution environment and that (2) methods not
    // specified with VTKM_EXEC (such as the STL algorithms) can be
    // called from the execution environment. Neither one of these is
    // necessarily true, but it is true for the current uses of this general
    // function and I don't want to compete with STL if I don't have to.

    using InputIteratorsType = vtkm::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);
    auto resultPos = std::lower_bound(
      inputIterators.GetBegin(), inputIterators.GetEnd(), this->ValuesPortal.Get(index));

    vtkm::Id resultIndex =
      static_cast<vtkm::Id>(std::distance(inputIterators.GetBegin(), resultPos));
    this->OutputPortal.Set(index, resultIndex);
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType,
          class ValuesPortalType,
          class OutputPortalType,
          class BinaryCompare>
struct LowerBoundsComparisonKernel
{
  InputPortalType InputPortal;
  ValuesPortalType ValuesPortal;
  OutputPortalType OutputPortal;
  BinaryCompare CompareFunctor;

  VTKM_CONT
  LowerBoundsComparisonKernel(InputPortalType inputPortal,
                              ValuesPortalType valuesPortal,
                              OutputPortalType outputPortal,
                              BinaryCompare binary_compare)
    : InputPortal(inputPortal)
    , ValuesPortal(valuesPortal)
    , OutputPortal(outputPortal)
    , CompareFunctor(binary_compare)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    // This method assumes that (1) InputPortalType can return working
    // iterators in the execution environment and that (2) methods not
    // specified with VTKM_EXEC (such as the STL algorithms) can be
    // called from the execution environment. Neither one of these is
    // necessarily true, but it is true for the current uses of this general
    // function and I don't want to compete with STL if I don't have to.

    using InputIteratorsType = vtkm::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);
    auto resultPos = std::lower_bound(inputIterators.GetBegin(),
                                      inputIterators.GetEnd(),
                                      this->ValuesPortal.Get(index),
                                      this->CompareFunctor);

    vtkm::Id resultIndex =
      static_cast<vtkm::Id>(std::distance(inputIterators.GetBegin(), resultPos));
    this->OutputPortal.Set(index, resultIndex);
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
};

template <typename PortalType>
struct SetConstantKernel
{
  using ValueType = typename PortalType::ValueType;
  PortalType Portal;
  ValueType Value;

  VTKM_CONT
  SetConstantKernel(const PortalType& portal, ValueType value)
    : Portal(portal)
    , Value(value)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const { this->Portal.Set(index, this->Value); }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
};

template <typename PortalType, typename BinaryCompare>
struct BitonicSortMergeKernel : vtkm::exec::FunctorBase
{
  PortalType Portal;
  BinaryCompare Compare;
  vtkm::Id GroupSize;

  VTKM_CONT
  BitonicSortMergeKernel(const PortalType& portal, const BinaryCompare& compare, vtkm::Id groupSize)
    : Portal(portal)
    , Compare(compare)
    , GroupSize(groupSize)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using ValueType = typename PortalType::ValueType;

    vtkm::Id groupIndex = index % this->GroupSize;
    vtkm::Id blockSize = 2 * this->GroupSize;
    vtkm::Id blockIndex = index / this->GroupSize;

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

template <typename PortalType, typename BinaryCompare>
struct BitonicSortCrossoverKernel : vtkm::exec::FunctorBase
{
  PortalType Portal;
  BinaryCompare Compare;
  vtkm::Id GroupSize;

  VTKM_CONT
  BitonicSortCrossoverKernel(const PortalType& portal,
                             const BinaryCompare& compare,
                             vtkm::Id groupSize)
    : Portal(portal)
    , Compare(compare)
    , GroupSize(groupSize)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using ValueType = typename PortalType::ValueType;

    vtkm::Id groupIndex = index % this->GroupSize;
    vtkm::Id blockSize = 2 * this->GroupSize;
    vtkm::Id blockIndex = index / this->GroupSize;

    vtkm::Id lowIndex = blockIndex * blockSize + groupIndex;
    vtkm::Id highIndex = blockIndex * blockSize + (blockSize - groupIndex - 1);

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

template <class StencilPortalType, class OutputPortalType, class UnaryPredicate>
struct StencilToIndexFlagKernel
{
  using StencilValueType = typename StencilPortalType::ValueType;
  StencilPortalType StencilPortal;
  OutputPortalType OutputPortal;
  UnaryPredicate Predicate;

  VTKM_CONT
  StencilToIndexFlagKernel(StencilPortalType stencilPortal,
                           OutputPortalType outputPortal,
                           UnaryPredicate unary_predicate)
    : StencilPortal(stencilPortal)
    , OutputPortal(outputPortal)
    , Predicate(unary_predicate)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    StencilValueType value = this->StencilPortal.Get(index);
    this->OutputPortal.Set(index, this->Predicate(value) ? 1 : 0);
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType,
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

  VTKM_CONT
  CopyIfKernel(InputPortalType inputPortal,
               StencilPortalType stencilPortal,
               IndexPortalType indexPortal,
               OutputPortalType outputPortal,
               PredicateOperator unary_predicate)
    : InputPortal(inputPortal)
    , StencilPortal(stencilPortal)
    , IndexPortal(indexPortal)
    , OutputPortal(outputPortal)
    , Predicate(unary_predicate)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using StencilValueType = typename StencilPortalType::ValueType;
    StencilValueType stencilValue = this->StencilPortal.Get(index);
    if (Predicate(stencilValue))
    {
      vtkm::Id outputIndex = this->IndexPortal.Get(index);

      using OutputValueType = typename OutputPortalType::ValueType;
      OutputValueType value = this->InputPortal.Get(index);

      this->OutputPortal.Set(outputIndex, value);
    }
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType, class StencilPortalType>
struct ClassifyUniqueKernel
{
  InputPortalType InputPortal;
  StencilPortalType StencilPortal;

  VTKM_CONT
  ClassifyUniqueKernel(InputPortalType inputPortal, StencilPortalType stencilPortal)
    : InputPortal(inputPortal)
    , StencilPortal(stencilPortal)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using ValueType = typename StencilPortalType::ValueType;
    if (index == 0)
    {
      // Always copy first value.
      this->StencilPortal.Set(index, ValueType(1));
    }
    else
    {
      ValueType flag = ValueType(this->InputPortal.Get(index - 1) != this->InputPortal.Get(index));
      this->StencilPortal.Set(index, flag);
    }
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType, class StencilPortalType, class BinaryCompare>
struct ClassifyUniqueComparisonKernel
{
  InputPortalType InputPortal;
  StencilPortalType StencilPortal;
  BinaryCompare CompareFunctor;

  VTKM_CONT
  ClassifyUniqueComparisonKernel(InputPortalType inputPortal,
                                 StencilPortalType stencilPortal,
                                 BinaryCompare binary_compare)
    : InputPortal(inputPortal)
    , StencilPortal(stencilPortal)
    , CompareFunctor(binary_compare)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using ValueType = typename StencilPortalType::ValueType;
    if (index == 0)
    {
      // Always copy first value.
      this->StencilPortal.Set(index, ValueType(1));
    }
    else
    {
      //comparison predicate returns true when they match
      const bool same =
        !(this->CompareFunctor(this->InputPortal.Get(index - 1), this->InputPortal.Get(index)));
      ValueType flag = ValueType(same);
      this->StencilPortal.Set(index, flag);
    }
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType, class ValuesPortalType, class OutputPortalType>
struct UpperBoundsKernel
{
  InputPortalType InputPortal;
  ValuesPortalType ValuesPortal;
  OutputPortalType OutputPortal;

  VTKM_CONT
  UpperBoundsKernel(InputPortalType inputPortal,
                    ValuesPortalType valuesPortal,
                    OutputPortalType outputPortal)
    : InputPortal(inputPortal)
    , ValuesPortal(valuesPortal)
    , OutputPortal(outputPortal)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    // This method assumes that (1) InputPortalType can return working
    // iterators in the execution environment and that (2) methods not
    // specified with VTKM_EXEC (such as the STL algorithms) can be
    // called from the execution environment. Neither one of these is
    // necessarily true, but it is true for the current uses of this general
    // function and I don't want to compete with STL if I don't have to.

    using InputIteratorsType = vtkm::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);
    auto resultPos = std::upper_bound(
      inputIterators.GetBegin(), inputIterators.GetEnd(), this->ValuesPortal.Get(index));

    vtkm::Id resultIndex =
      static_cast<vtkm::Id>(std::distance(inputIterators.GetBegin(), resultPos));
    this->OutputPortal.Set(index, resultIndex);
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType,
          class ValuesPortalType,
          class OutputPortalType,
          class BinaryCompare>
struct UpperBoundsKernelComparisonKernel
{
  InputPortalType InputPortal;
  ValuesPortalType ValuesPortal;
  OutputPortalType OutputPortal;
  BinaryCompare CompareFunctor;

  VTKM_CONT
  UpperBoundsKernelComparisonKernel(InputPortalType inputPortal,
                                    ValuesPortalType valuesPortal,
                                    OutputPortalType outputPortal,
                                    BinaryCompare binary_compare)
    : InputPortal(inputPortal)
    , ValuesPortal(valuesPortal)
    , OutputPortal(outputPortal)
    , CompareFunctor(binary_compare)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    // This method assumes that (1) InputPortalType can return working
    // iterators in the execution environment and that (2) methods not
    // specified with VTKM_EXEC (such as the STL algorithms) can be
    // called from the execution environment. Neither one of these is
    // necessarily true, but it is true for the current uses of this general
    // function and I don't want to compete with STL if I don't have to.

    using InputIteratorsType = vtkm::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);
    auto resultPos = std::upper_bound(inputIterators.GetBegin(),
                                      inputIterators.GetEnd(),
                                      this->ValuesPortal.Get(index),
                                      this->CompareFunctor);

    vtkm::Id resultIndex =
      static_cast<vtkm::Id>(std::distance(inputIterators.GetBegin(), resultPos));
    this->OutputPortal.Set(index, resultIndex);
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
};

template <typename InPortalType, typename OutPortalType, typename BinaryFunctor>
struct InclusiveToExclusiveKernel : vtkm::exec::FunctorBase
{
  using ValueType = typename InPortalType::ValueType;

  InPortalType InPortal;
  OutPortalType OutPortal;
  BinaryFunctor BinaryOperator;
  ValueType InitialValue;

  VTKM_CONT
  InclusiveToExclusiveKernel(const InPortalType& inPortal,
                             const OutPortalType& outPortal,
                             BinaryFunctor& binaryOperator,
                             ValueType initialValue)
    : InPortal(inPortal)
    , OutPortal(outPortal)
    , BinaryOperator(binaryOperator)
    , InitialValue(initialValue)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    ValueType result = (index == 0)
      ? this->InitialValue
      : this->BinaryOperator(this->InitialValue, this->InPortal.Get(index - 1));
    this->OutPortal.Set(index, result);
  }
};

template <typename PortalType, typename BinaryFunctor>
struct ScanKernel : vtkm::exec::FunctorBase
{
  PortalType Portal;
  BinaryFunctor BinaryOperator;
  vtkm::Id Stride;
  vtkm::Id Offset;
  vtkm::Id Distance;

  VTKM_CONT
  ScanKernel(const PortalType& portal,
             BinaryFunctor binary_functor,
             vtkm::Id stride,
             vtkm::Id offset)
    : Portal(portal)
    , BinaryOperator(binary_functor)
    , Stride(stride)
    , Offset(offset)
    , Distance(stride / 2)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using ValueType = typename PortalType::ValueType;

    vtkm::Id leftIndex = this->Offset + index * this->Stride;
    vtkm::Id rightIndex = leftIndex + this->Distance;

    if (rightIndex < this->Portal.GetNumberOfValues())
    {
      ValueType leftValue = this->Portal.Get(leftIndex);
      ValueType rightValue = this->Portal.Get(rightIndex);
      this->Portal.Set(rightIndex, BinaryOperator(leftValue, rightValue));
    }
  }
};

template <typename InPortalType1,
          typename InPortalType2,
          typename OutPortalType,
          typename BinaryFunctor>
struct BinaryTransformKernel : vtkm::exec::FunctorBase
{
  InPortalType1 InPortal1;
  InPortalType2 InPortal2;
  OutPortalType OutPortal;
  BinaryFunctor BinaryOperator;

  VTKM_CONT
  BinaryTransformKernel(const InPortalType1& inPortal1,
                        const InPortalType2& inPortal2,
                        const OutPortalType& outPortal,
                        BinaryFunctor binaryOperator)
    : InPortal1(inPortal1)
    , InPortal2(inPortal2)
    , OutPortal(outPortal)
    , BinaryOperator(binaryOperator)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    this->OutPortal.Set(
      index, this->BinaryOperator(this->InPortal1.Get(index), this->InPortal2.Get(index)));
  }
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_FunctorsGeneral_h
