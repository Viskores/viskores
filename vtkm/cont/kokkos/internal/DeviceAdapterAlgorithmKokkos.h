//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_kokkos_internal_DeviceAdapterAlgorithmKokkos_h
#define vtk_m_cont_kokkos_internal_DeviceAdapterAlgorithmKokkos_h

#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/kokkos/internal/DeviceAdapterTagKokkos.h>
#include <vtkm/cont/kokkos/internal/KokkosTypes.h>

#include <vtkm/exec/kokkos/internal/TaskBasic.h>

#include <vtkmstd/void_t.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <Kokkos_Core.hpp>
#include <Kokkos_DualView.hpp>
#include <Kokkos_Parallel_Reduce.hpp>
#include <Kokkos_Sort.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <type_traits>

namespace vtkm
{
namespace internal
{

template <typename, typename = void>
struct is_type_complete : public std::false_type
{
};

template <typename T>
struct is_type_complete<T, vtkmstd::void_t<decltype(sizeof(T))>> : public std::true_type
{
};
} // internal

namespace cont
{

namespace kokkos
{
namespace internal
{

//----------------------------------------------------------------------------
template <typename BitsPortal>
struct BitFieldToBoolField : public vtkm::exec::FunctorBase
{
  VTKM_EXEC_CONT BitFieldToBoolField() {}

  VTKM_CONT
  explicit BitFieldToBoolField(const BitsPortal& bp)
    : Bits(bp)
  {
  }

  VTKM_EXEC bool operator()(vtkm::Id bitIdx) const { return this->Bits.GetBit(bitIdx); }

private:
  BitsPortal Bits;
};

template <typename BitsPortal>
struct BitFieldCountSetBitsWord : public vtkm::exec::FunctorBase
{
  VTKM_EXEC_CONT BitFieldCountSetBitsWord() {}

  VTKM_CONT
  explicit BitFieldCountSetBitsWord(const BitsPortal& bp)
    : Bits(bp)
  {
  }

  VTKM_EXEC vtkm::Id operator()(vtkm::Id wordIdx) const
  {
    auto word = this->Bits.GetWord(wordIdx);
    if (wordIdx == (this->Bits.GetNumberOfWords() - 1))
    {
      word &= this->Bits.GetFinalWordMask();
    }

    return vtkm::CountSetBits(word);
  }

private:
  BitsPortal Bits;
};

//----------------------------------------------------------------------------
template <typename Operator, typename ResultType>
struct ReductionIdentity;

template <typename ResultType>
struct ReductionIdentity<vtkm::Sum, ResultType>
{
  static constexpr ResultType value = Kokkos::reduction_identity<ResultType>::sum();
};

template <typename ResultType>
struct ReductionIdentity<vtkm::Add, ResultType>
{
  static constexpr ResultType value = Kokkos::reduction_identity<ResultType>::sum();
};

template <typename ResultType>
struct ReductionIdentity<vtkm::Product, ResultType>
{
  static constexpr ResultType value = Kokkos::reduction_identity<ResultType>::prod();
};

template <typename ResultType>
struct ReductionIdentity<vtkm::Multiply, ResultType>
{
  static constexpr ResultType value = Kokkos::reduction_identity<ResultType>::prod();
};

template <typename ResultType>
struct ReductionIdentity<vtkm::Minimum, ResultType>
{
  static constexpr ResultType value = Kokkos::reduction_identity<ResultType>::min();
};

template <typename ResultType>
struct ReductionIdentity<vtkm::Maximum, ResultType>
{
  static constexpr ResultType value = Kokkos::reduction_identity<ResultType>::max();
};

template <typename ResultType>
struct ReductionIdentity<vtkm::MinAndMax<ResultType>, vtkm::Vec<ResultType, 2>>
{
  static constexpr vtkm::Vec<ResultType, 2> value =
    vtkm::Vec<ResultType, 2>(Kokkos::reduction_identity<ResultType>::min(),
                             Kokkos::reduction_identity<ResultType>::max());
};

template <typename ResultType>
struct ReductionIdentity<vtkm::BitwiseAnd, ResultType>
{
  static constexpr ResultType value = Kokkos::reduction_identity<ResultType>::band();
};

template <typename ResultType>
struct ReductionIdentity<vtkm::BitwiseOr, ResultType>
{
  static constexpr ResultType value = Kokkos::reduction_identity<ResultType>::bor();
};
}
} // kokkos::internal

template <>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagKokkos>
  : vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagKokkos>,
      vtkm::cont::DeviceAdapterTagKokkos>
{
private:
  using Superclass = vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
    DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagKokkos>,
    vtkm::cont::DeviceAdapterTagKokkos>;

  VTKM_CONT_EXPORT static vtkm::exec::internal::ErrorMessageBuffer GetErrorMessageBufferInstance();
  VTKM_CONT_EXPORT static void CheckForErrors();

public:
  template <typename IndicesStorage>
  VTKM_CONT static vtkm::Id BitFieldToUnorderedSet(
    const vtkm::cont::BitField& bits,
    vtkm::cont::ArrayHandle<Id, IndicesStorage>& indices)
  {
    vtkm::cont::Token token;
    auto bitsPortal = bits.PrepareForInput(DeviceAdapterTagKokkos{}, token);
    auto bits2bools = kokkos::internal::BitFieldToBoolField<decltype(bitsPortal)>(bitsPortal);

    DeviceAdapterAlgorithm::CopyIf(
      vtkm::cont::ArrayHandleIndex(bits.GetNumberOfBits()),
      vtkm::cont::make_ArrayHandleImplicit(bits2bools, bits.GetNumberOfBits()),
      indices);

    return indices.GetNumberOfValues();
  }

  VTKM_CONT static vtkm::Id CountSetBits(const vtkm::cont::BitField& bits)
  {
    vtkm::cont::Token token;
    auto bitsPortal = bits.PrepareForInput(DeviceAdapterTagKokkos{}, token);
    auto countPerWord =
      kokkos::internal::BitFieldCountSetBitsWord<decltype(bitsPortal)>(bitsPortal);

    return DeviceAdapterAlgorithm::Reduce(
      vtkm::cont::make_ArrayHandleImplicit(countPerWord, bitsPortal.GetNumberOfWords()),
      vtkm::Id{ 0 });
  }

  using Superclass::Copy;

  template <typename T>
  VTKM_CONT static void Copy(const vtkm::cont::ArrayHandle<T>& input,
                             vtkm::cont::ArrayHandle<T>& output)
  {
    const vtkm::Id inSize = input.GetNumberOfValues();

    vtkm::cont::Token token;

    auto portalIn = input.PrepareForInput(vtkm::cont::DeviceAdapterTagKokkos{}, token);
    auto portalOut = output.PrepareForOutput(inSize, vtkm::cont::DeviceAdapterTagKokkos{}, token);


    kokkos::internal::KokkosViewConstExec<T> viewIn(portalIn.GetArray(), inSize);
    kokkos::internal::KokkosViewExec<T> viewOut(portalOut.GetArray(), inSize);
    Kokkos::deep_copy(vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), viewOut, viewIn);
  }

private:
  template <typename ArrayHandle, typename BinaryFunctor, typename ResultType>
  VTKM_CONT static ResultType ReduceImpl(const ArrayHandle& input,
                                         BinaryFunctor binary_functor,
                                         ResultType initialValue,
                                         std::false_type)
  {
    return Superclass::Reduce(input, initialValue, binary_functor);
  }

  template <typename ArrayPortal, typename BinaryFunctor, typename ResultType>
  class ReduceFunctor
  {
  public:
    using size_type = vtkm::Id;
    using value_type = ResultType;

    KOKKOS_INLINE_FUNCTION
    ReduceFunctor() {}

    KOKKOS_INLINE_FUNCTION
    explicit ReduceFunctor(const ArrayPortal& portal, const BinaryFunctor& op)
      : Portal(portal)
      , Operator(op)
    {
    }

    KOKKOS_INLINE_FUNCTION
    void operator()(const size_type i, value_type& update) const
    {
      update = this->Operator(update, this->Portal.Get(i));
    }

    KOKKOS_INLINE_FUNCTION
    void join(volatile value_type& dst, const volatile value_type& src) const
    {
      dst = this->Operator(dst, src);
    }

    KOKKOS_INLINE_FUNCTION void init(value_type& dst) const
    {
      dst = kokkos::internal::ReductionIdentity<BinaryFunctor, value_type>::value;
    }

  private:
    ArrayPortal Portal;
    BinaryFunctor Operator;
  };

  template <typename ArrayHandle, typename BinaryFunctor, typename ResultType>
  VTKM_CONT static ResultType ReduceImpl(const ArrayHandle& input,
                                         BinaryFunctor binary_functor,
                                         ResultType initialValue,
                                         std::true_type)
  {
    vtkm::cont::Token token;
    auto inputPortal = input.PrepareForInput(vtkm::cont::DeviceAdapterTagKokkos{}, token);

    ReduceFunctor<decltype(inputPortal), BinaryFunctor, ResultType> functor(inputPortal,
                                                                            binary_functor);

    ResultType result;

    Kokkos::RangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace, vtkm::Id> policy(
      vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), 0, input.GetNumberOfValues());
    Kokkos::parallel_reduce(policy, functor, result);

    return binary_functor(initialValue, result);
  }

  template <bool P1, typename BinaryFunctor, typename ResultType>
  struct UseKokkosReduceP1 : std::false_type
  {
  };

  template <typename BinaryFunctor, typename ResultType>
  struct UseKokkosReduceP1<true, BinaryFunctor, ResultType>
    : vtkm::internal::is_type_complete<
        kokkos::internal::ReductionIdentity<BinaryFunctor, ResultType>>
  {
  };

  template <typename BinaryFunctor, typename ResultType>
  struct UseKokkosReduce
    : UseKokkosReduceP1<
        vtkm::internal::is_type_complete<Kokkos::reduction_identity<ResultType>>::value,
        BinaryFunctor,
        ResultType>
  {
  };

public:
  template <typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
#if defined(VTKM_KOKKOS_CUDA)
    // Kokkos reduce is having some issues with the cuda backend. Please refer to issue #586.
    // Following is a work around where we use the Superclass reduce implementation when using
    // Cuda execution space.
    std::integral_constant<
      bool,
      !std::is_same<vtkm::cont::kokkos::internal::ExecutionSpace, Kokkos::Cuda>::value &&
        UseKokkosReduce<BinaryFunctor, U>::value>
      use_kokkos_reduce;
#else
    typename UseKokkosReduce<BinaryFunctor, U>::type use_kokkos_reduce;
#endif

    return ReduceImpl(input, binary_functor, initialValue, use_kokkos_reduce);
  }

  template <typename T, typename U, class CIn>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    return Reduce(input, initialValue, vtkm::Add());
  }

  template <typename WType, typename IType>
  VTKM_CONT static void ScheduleTask(
    vtkm::exec::kokkos::internal::TaskBasic1D<WType, IType>& functor,
    vtkm::Id numInstances)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    if (numInstances < 1)
    {
      // No instances means nothing to run. Just return.
      return;
    }

    functor.SetErrorMessageBuffer(GetErrorMessageBufferInstance());

    Kokkos::RangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace, vtkm::Id> policy(
      vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), 0, numInstances);
    Kokkos::parallel_for(policy, functor);
    CheckForErrors(); // synchronizes
  }

  template <typename WType, typename IType>
  VTKM_CONT static void ScheduleTask(
    vtkm::exec::kokkos::internal::TaskBasic3D<WType, IType>& functor,
    vtkm::Id3 rangeMax)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    if ((rangeMax[0] < 1) || (rangeMax[1] < 1) || (rangeMax[2] < 1))
    {
      // No instances means nothing to run. Just return.
      return;
    }

    functor.SetErrorMessageBuffer(GetErrorMessageBufferInstance());

    Kokkos::MDRangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace,
                          Kokkos::Rank<3>,
                          Kokkos::IndexType<vtkm::Id>>
      policy(vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(),
             { 0, 0, 0 },
             { rangeMax[0], rangeMax[1], rangeMax[2] });

    Kokkos::parallel_for(
      policy, KOKKOS_LAMBDA(vtkm::Id i, vtkm::Id j, vtkm::Id k) {
        auto flatIdx = i + (j * rangeMax[0]) + (k * rangeMax[0] * rangeMax[1]);
        functor(vtkm::Id3(i, j, k), flatIdx);
      });
    CheckForErrors(); // synchronizes
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id numInstances)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::exec::kokkos::internal::TaskBasic1D<Functor, vtkm::internal::NullType> kernel(functor);
    ScheduleTask(kernel, numInstances);
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, const vtkm::Id3& rangeMax)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::exec::kokkos::internal::TaskBasic3D<Functor, vtkm::internal::NullType> kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

private:
  template <typename T>
  VTKM_CONT static void SortImpl(vtkm::cont::ArrayHandle<T>& values, vtkm::SortLess, std::true_type)
  {
    vtkm::cont::Token token;
    auto portal = values.PrepareForInPlace(vtkm::cont::DeviceAdapterTagKokkos{}, token);
    kokkos::internal::KokkosViewExec<T> view(portal.GetArray(), portal.GetNumberOfValues());

    // We use per-thread execution spaces so that the threads can execute independently without
    // requiring global synchronizations.
    // Currently, there is no way to specify the execution space for sort and therefore it
    // executes in the default execution space.
    // Therefore, we need explicit syncs here.
    vtkm::cont::kokkos::internal::GetExecutionSpaceInstance().fence();
    Kokkos::sort(view);
    vtkm::cont::kokkos::internal::GetExecutionSpaceInstance().fence();
  }

  template <typename T>
  VTKM_CONT static void SortImpl(vtkm::cont::ArrayHandle<T>& values,
                                 vtkm::SortLess comp,
                                 std::false_type)
  {
    Superclass::Sort(values, comp);
  }

public:
  using Superclass::Sort;

  template <typename T>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T>& values, vtkm::SortLess comp)
  {
    SortImpl(values, comp, typename std::is_scalar<T>::type{});
  }

  VTKM_CONT static void Synchronize()
  {
    vtkm::cont::kokkos::internal::GetExecutionSpaceInstance().fence();
  }
};

template <>
class DeviceTaskTypes<vtkm::cont::DeviceAdapterTagKokkos>
{
public:
  template <typename WorkletType, typename InvocationType>
  VTKM_CONT static vtkm::exec::kokkos::internal::TaskBasic1D<WorkletType, InvocationType>
  MakeTask(WorkletType& worklet, InvocationType& invocation, vtkm::Id)
  {
    return vtkm::exec::kokkos::internal::TaskBasic1D<WorkletType, InvocationType>(worklet,
                                                                                  invocation);
  }

  template <typename WorkletType, typename InvocationType>
  VTKM_CONT static vtkm::exec::kokkos::internal::TaskBasic3D<WorkletType, InvocationType>
  MakeTask(WorkletType& worklet, InvocationType& invocation, vtkm::Id3)
  {
    return vtkm::exec::kokkos::internal::TaskBasic3D<WorkletType, InvocationType>(worklet,
                                                                                  invocation);
  }
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_kokkos_internal_DeviceAdapterAlgorithmKokkos_h
