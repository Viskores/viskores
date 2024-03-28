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

#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
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
#include <Kokkos_Sort.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <type_traits>

#if KOKKOS_VERSION_MAJOR > 3 || (KOKKOS_VERSION_MAJOR == 3 && KOKKOS_VERSION_MINOR >= 7)
#define VTKM_VOLATILE
#else
#define VTKM_VOLATILE volatile
#endif

#if defined(VTKM_ENABLE_KOKKOS_THRUST) && (defined(__HIP__) || defined(__CUDA__))
#define VTKM_USE_KOKKOS_THRUST
#endif

#if defined(VTKM_USE_KOKKOS_THRUST)
#include <thrust/device_ptr.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/sort.h>
#endif

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

//=============================================================================
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

  //----------------------------------------------------------------------------
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

  //----------------------------------------------------------------------------
#ifndef VTKM_CUDA
  // nvcc doesn't like the private class declaration so disable under CUDA
private:
#endif
  template <typename ArrayHandle, typename BinaryOperator, typename ResultType>
  VTKM_CONT static ResultType ReduceImpl(const ArrayHandle& input,
                                         BinaryOperator binaryOperator,
                                         ResultType initialValue,
                                         std::false_type)
  {
    return Superclass::Reduce(input, initialValue, binaryOperator);
  }

  template <typename BinaryOperator, typename FunctorOperator, typename ResultType>
  class KokkosReduceFunctor
  {
  public:
    using size_type = vtkm::Id;
    using value_type = ResultType;

    KOKKOS_INLINE_FUNCTION
    KokkosReduceFunctor() {}

    template <typename... Args>
    KOKKOS_INLINE_FUNCTION explicit KokkosReduceFunctor(const BinaryOperator& op, Args... args)
      : Operator(op)
      , Functor(std::forward<Args>(args)...)
    {
    }

    KOKKOS_INLINE_FUNCTION
    void join(VTKM_VOLATILE value_type& dst, const VTKM_VOLATILE value_type& src) const
    {
      dst = this->Operator(dst, src);
    }

    KOKKOS_INLINE_FUNCTION
    void init(value_type& dst) const
    {
      dst = kokkos::internal::ReductionIdentity<BinaryOperator, value_type>::value;
    }

    // Reduce operator
    KOKKOS_INLINE_FUNCTION
    void operator()(vtkm::Id i, ResultType& update) const
    {
      this->Functor(this->Operator, i, update);
    }

    // Scan operator
    KOKKOS_INLINE_FUNCTION
    void operator()(vtkm::Id i, ResultType& update, const bool final) const
    {
      this->Functor(this->Operator, i, update, final);
    }

  private:
    BinaryOperator Operator;
    FunctorOperator Functor;
  };

  template <typename ArrayPortal, typename BinaryOperator, typename ResultType>
  class ReduceOperator
  {
  public:
    KOKKOS_INLINE_FUNCTION
    ReduceOperator() {}

    KOKKOS_INLINE_FUNCTION
    explicit ReduceOperator(const ArrayPortal& portal)
      : Portal(portal)
    {
    }

    KOKKOS_INLINE_FUNCTION
    void operator()(const BinaryOperator& op, vtkm::Id i, ResultType& update) const
    {
      update = op(update, this->Portal.Get(i));
    }

  private:
    ArrayPortal Portal;
  };

  template <typename BinaryOperator, typename ArrayPortal, typename ResultType>
  using ReduceFunctor = KokkosReduceFunctor<BinaryOperator,
                                            ReduceOperator<ArrayPortal, BinaryOperator, ResultType>,
                                            ResultType>;

  template <typename ArrayHandle, typename BinaryOperator, typename ResultType>
  VTKM_CONT static ResultType ReduceImpl(const ArrayHandle& input,
                                         BinaryOperator binaryOperator,
                                         ResultType initialValue,
                                         std::true_type)
  {
    vtkm::cont::Token token;
    auto inputPortal = input.PrepareForInput(vtkm::cont::DeviceAdapterTagKokkos{}, token);

    ReduceFunctor<BinaryOperator, decltype(inputPortal), ResultType> functor(binaryOperator,
                                                                             inputPortal);

    ResultType result;

    Kokkos::RangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace, vtkm::Id> policy(
      vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), 0, input.GetNumberOfValues());
    Kokkos::parallel_reduce(policy, functor, result);

    return binaryOperator(initialValue, result);
  }

  template <bool P1, typename BinaryOperator, typename ResultType>
  struct UseKokkosReduceP1 : std::false_type
  {
  };

  template <typename BinaryOperator, typename ResultType>
  struct UseKokkosReduceP1<true, BinaryOperator, ResultType>
    : vtkm::internal::is_type_complete<
        kokkos::internal::ReductionIdentity<BinaryOperator, ResultType>>
  {
  };

  template <typename BinaryOperator, typename ResultType>
  struct UseKokkosReduce
    : UseKokkosReduceP1<
        vtkm::internal::is_type_complete<Kokkos::reduction_identity<ResultType>>::value,
        BinaryOperator,
        ResultType>
  {
  };

public:
  template <typename T, typename U, class CIn, class BinaryOperator>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryOperator binaryOperator)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    if (input.GetNumberOfValues() == 0)
    {
      return initialValue;
    }
    if (input.GetNumberOfValues() == 1)
    {
      return binaryOperator(initialValue, input.ReadPortal().Get(0));
    }

#if defined(VTKM_KOKKOS_CUDA)
    // Kokkos reduce is having some issues with the cuda backend. Please refer to issue #586.
    // Following is a work around where we use the Superclass reduce implementation when using
    // Cuda execution space.
    std::integral_constant<
      bool,
      !std::is_same<vtkm::cont::kokkos::internal::ExecutionSpace, Kokkos::Cuda>::value &&
        UseKokkosReduce<BinaryOperator, U>::value>
      use_kokkos_reduce;
#else
    typename UseKokkosReduce<BinaryOperator, U>::type use_kokkos_reduce;
#endif
    return ReduceImpl(input, binaryOperator, initialValue, use_kokkos_reduce);
  }

  template <typename T, typename U, class CIn>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    return Reduce(input, initialValue, vtkm::Add());
  }

  //----------------------------------------------------------------------------
#ifndef VTKM_CUDA
  // nvcc doesn't like the private class declaration so disable under CUDA
private:
#endif
  // Scan and Reduce have the same conditions
  template <typename BinaryOperator, typename ResultType>
  using UseKokkosScan = UseKokkosReduce<BinaryOperator, ResultType>;

  template <typename T, typename StorageIn, typename StorageOut, typename BinaryOperator>
  VTKM_CONT static T ScanExclusiveImpl(const vtkm::cont::ArrayHandle<T, StorageIn>& input,
                                       vtkm::cont::ArrayHandle<T, StorageOut>& output,
                                       BinaryOperator binaryOperator,
                                       const T& initialValue,
                                       std::false_type)
  {
    return Superclass::ScanExclusive(input, output, binaryOperator, initialValue);
  }

  template <typename T, typename StorageIn, typename StorageOut, typename BinaryOperator>
  class ScanExclusiveOperator
  {
  private:
    using ArrayPortalIn = typename ArrayHandle<T, StorageIn>::ReadPortalType;
    using ArrayPortalOut = typename ArrayHandle<T, StorageOut>::WritePortalType;

  public:
    KOKKOS_INLINE_FUNCTION
    ScanExclusiveOperator() {}

    KOKKOS_INLINE_FUNCTION
    explicit ScanExclusiveOperator(const ArrayPortalIn& portalIn,
                                   const ArrayPortalOut& portalOut,
                                   const T& initialValue)
      : PortalIn(portalIn)
      , PortalOut(portalOut)
      , InitialValue(initialValue)
    {
    }

    KOKKOS_INLINE_FUNCTION
    void operator()(const BinaryOperator& op, const vtkm::Id i, T& update, const bool final) const
    {
      auto val = this->PortalIn.Get(i);
      if (i == 0)
      {
        update = InitialValue;
      }
      if (final)
      {
        this->PortalOut.Set(i, update);
      }
      update = op(update, val);
    }

  private:
    ArrayPortalIn PortalIn;
    ArrayPortalOut PortalOut;
    T InitialValue;
  };

  template <typename BinaryOperator, typename T, typename StorageIn, typename StorageOut>
  using ScanExclusiveFunctor =
    KokkosReduceFunctor<BinaryOperator,
                        ScanExclusiveOperator<T, StorageIn, StorageOut, BinaryOperator>,
                        T>;

  template <typename T, typename StorageIn, typename StorageOut, typename BinaryOperator>
  VTKM_CONT static T ScanExclusiveImpl(const vtkm::cont::ArrayHandle<T, StorageIn>& input,
                                       vtkm::cont::ArrayHandle<T, StorageOut>& output,
                                       BinaryOperator binaryOperator,
                                       const T& initialValue,
                                       std::true_type)
  {
    vtkm::Id length = input.GetNumberOfValues();

    vtkm::cont::Token token;
    auto inputPortal = input.PrepareForInput(vtkm::cont::DeviceAdapterTagKokkos{}, token);
    auto outputPortal =
      output.PrepareForOutput(length, vtkm::cont::DeviceAdapterTagKokkos{}, token);

    ScanExclusiveFunctor<BinaryOperator, T, StorageIn, StorageOut> functor(
      binaryOperator, inputPortal, outputPortal, initialValue);

    T result = vtkm::TypeTraits<T>::ZeroInitialization();
    Kokkos::RangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace, vtkm::Id> policy(
      vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), 0, length);
    Kokkos::parallel_scan(policy, functor, result);

    return result;
  }

public:
  template <typename T, class CIn, class COut, class BinaryOperator>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryOperator binaryOperator,
                                   const T& initialValue)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id length = input.GetNumberOfValues();
    if (length == 0)
    {
      output.ReleaseResources();
      return initialValue;
    }
    if (length == 1)
    {
      auto v0 = input.ReadPortal().Get(0);
      Fill(output, initialValue, 1);
      return binaryOperator(initialValue, v0);
    }

#if defined(VTKM_KOKKOS_CUDA)
    // Kokkos scan for the cuda backend is not working correctly for int/uint types of 8 and 16 bits.
    std::integral_constant<bool,
                           !(std::is_integral<T>::value && sizeof(T) < 4) &&
                             UseKokkosScan<BinaryOperator, T>::value>
      use_kokkos_scan;
#else
    typename UseKokkosScan<BinaryOperator, T>::type use_kokkos_scan;
#endif
    return ScanExclusiveImpl(input, output, binaryOperator, initialValue, use_kokkos_scan);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    return ScanExclusive(input, output, vtkm::Sum(), vtkm::TypeTraits<T>::ZeroInitialization());
  }

  //----------------------------------------------------------------------------
#ifndef VTKM_CUDA
  // nvcc doesn't like the private class declaration so disable under CUDA
private:
#endif
  template <typename T, typename StorageIn, typename StorageOut, typename BinaryOperator>
  VTKM_CONT static T ScanInclusiveImpl(const vtkm::cont::ArrayHandle<T, StorageIn>& input,
                                       vtkm::cont::ArrayHandle<T, StorageOut>& output,
                                       BinaryOperator binaryOperator,
                                       std::false_type)
  {
    return Superclass::ScanInclusive(input, output, binaryOperator);
  }

  template <typename T, typename StorageIn, typename StorageOut, typename BinaryOperator>
  class ScanInclusiveOperator
  {
  private:
    using ArrayPortalIn = typename ArrayHandle<T, StorageIn>::ReadPortalType;
    using ArrayPortalOut = typename ArrayHandle<T, StorageOut>::WritePortalType;

  public:
    KOKKOS_INLINE_FUNCTION
    ScanInclusiveOperator() {}

    KOKKOS_INLINE_FUNCTION
    explicit ScanInclusiveOperator(const ArrayPortalIn& portalIn, const ArrayPortalOut& portalOut)
      : PortalIn(portalIn)
      , PortalOut(portalOut)
    {
    }

    KOKKOS_INLINE_FUNCTION
    void operator()(const BinaryOperator& op, const vtkm::Id i, T& update, const bool final) const
    {
      update = op(update, this->PortalIn.Get(i));
      if (final)
      {
        this->PortalOut.Set(i, update);
      }
    }

  private:
    ArrayPortalIn PortalIn;
    ArrayPortalOut PortalOut;
  };

  template <typename BinaryOperator, typename T, typename StorageIn, typename StorageOut>
  using ScanInclusiveFunctor =
    KokkosReduceFunctor<BinaryOperator,
                        ScanInclusiveOperator<T, StorageIn, StorageOut, BinaryOperator>,
                        T>;

  template <typename T, typename StorageIn, typename StorageOut, typename BinaryOperator>
  VTKM_CONT static T ScanInclusiveImpl(const vtkm::cont::ArrayHandle<T, StorageIn>& input,
                                       vtkm::cont::ArrayHandle<T, StorageOut>& output,
                                       BinaryOperator binaryOperator,
                                       std::true_type)
  {
    vtkm::Id length = input.GetNumberOfValues();

    vtkm::cont::Token token;
    auto inputPortal = input.PrepareForInput(vtkm::cont::DeviceAdapterTagKokkos{}, token);
    auto outputPortal =
      output.PrepareForOutput(length, vtkm::cont::DeviceAdapterTagKokkos{}, token);

    ScanInclusiveFunctor<BinaryOperator, T, StorageIn, StorageOut> functor(
      binaryOperator, inputPortal, outputPortal);

    T result = vtkm::TypeTraits<T>::ZeroInitialization();
    Kokkos::RangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace, vtkm::Id> policy(
      vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), 0, length);
    Kokkos::parallel_scan(policy, functor, result);

    return result;
  }

public:
  template <typename T, class CIn, class COut, class BinaryOperator>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryOperator binaryOperator)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id length = input.GetNumberOfValues();
    if (length == 0)
    {
      return vtkm::TypeTraits<T>::ZeroInitialization();
    }
    if (length == 1)
    {
      auto result = input.ReadPortal().Get(0);
      Fill(output, result, 1);
      return result;
    }

#if defined(VTKM_KOKKOS_CUDA)
    // Kokkos scan for the cuda backend is not working correctly for int/uint types of 8 and 16 bits.
    std::integral_constant<bool,
                           !(std::is_integral<T>::value && sizeof(T) < 4) &&
                             UseKokkosScan<BinaryOperator, T>::value>
      use_kokkos_scan;
#else
    typename UseKokkosScan<BinaryOperator, T>::type use_kokkos_scan;
#endif
    return ScanInclusiveImpl(input, output, binaryOperator, use_kokkos_scan);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    return ScanInclusive(input, output, vtkm::Add());
  }

  //----------------------------------------------------------------------------
  template <typename WType, typename IType, typename Hints>
  VTKM_CONT static void ScheduleTask(
    vtkm::exec::kokkos::internal::TaskBasic1D<WType, IType, Hints>& functor,
    vtkm::Id numInstances)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    if (numInstances < 1)
    {
      // No instances means nothing to run. Just return.
      return;
    }

    functor.SetErrorMessageBuffer(GetErrorMessageBufferInstance());

    constexpr vtkm::IdComponent maxThreadsPerBlock =
      vtkm::cont::internal::HintFind<Hints,
                                     vtkm::cont::internal::HintThreadsPerBlock<0>,
                                     vtkm::cont::DeviceAdapterTagKokkos>::MaxThreads;

    Kokkos::RangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace,
                        Kokkos::LaunchBounds<maxThreadsPerBlock, 0>,
                        Kokkos::IndexType<vtkm::Id>>
      policy(vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), 0, numInstances);
    Kokkos::parallel_for(policy, functor);
    CheckForErrors(); // synchronizes
  }

  template <typename WType, typename IType, typename Hints>
  VTKM_CONT static void ScheduleTask(
    vtkm::exec::kokkos::internal::TaskBasic3D<WType, IType, Hints>& functor,
    vtkm::Id3 rangeMax)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    if ((rangeMax[0] < 1) || (rangeMax[1] < 1) || (rangeMax[2] < 1))
    {
      // No instances means nothing to run. Just return.
      return;
    }

    functor.SetErrorMessageBuffer(GetErrorMessageBufferInstance());

    constexpr vtkm::IdComponent maxThreadsPerBlock =
      vtkm::cont::internal::HintFind<Hints,
                                     vtkm::cont::internal::HintThreadsPerBlock<0>,
                                     vtkm::cont::DeviceAdapterTagKokkos>::MaxThreads;

    Kokkos::MDRangePolicy<vtkm::cont::kokkos::internal::ExecutionSpace,
                          Kokkos::LaunchBounds<maxThreadsPerBlock, 0>,
                          Kokkos::Rank<3>,
                          Kokkos::IndexType<vtkm::Id>>
      policy(vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(),
             { 0, 0, 0 },
             { rangeMax[0], rangeMax[1], rangeMax[2] });

    // Calling rangeMax[X] inside KOKKOS_LAMBDA confuses some compilers since
    // at first it tries to use the non-const inline vec_base::operator[0]
    // method, however, KOKKOS_LAMBDA DOES converts rangeMax to a const
    // vec_base. This convertion is somehow catched by the compiler making it
    // complain that we are using a non-const method for a const object.
    const auto rMax_0 = rangeMax[0];
    const auto rMax_1 = rangeMax[1];

    Kokkos::parallel_for(
      policy, KOKKOS_LAMBDA(vtkm::Id i, vtkm::Id j, vtkm::Id k) {
        auto flatIdx = i + (j * rMax_0) + (k * rMax_0 * rMax_1);
        functor(vtkm::Id3(i, j, k), flatIdx);
      });
    CheckForErrors(); // synchronizes
  }

  template <typename Hints, typename Functor>
  VTKM_CONT static void Schedule(Hints, Functor functor, vtkm::Id numInstances)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::exec::kokkos::internal::TaskBasic1D<Functor, vtkm::internal::NullType, Hints> kernel(
      functor);
    ScheduleTask(kernel, numInstances);
  }

  template <typename FunctorType>
  VTKM_CONT static inline void Schedule(FunctorType&& functor, vtkm::Id numInstances)
  {
    Schedule(vtkm::cont::internal::HintList<>{}, functor, numInstances);
  }

  template <typename Hints, typename Functor>
  VTKM_CONT static void Schedule(Hints, Functor functor, const vtkm::Id3& rangeMax)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::exec::kokkos::internal::TaskBasic3D<Functor, vtkm::internal::NullType, Hints> kernel(
      functor);
    ScheduleTask(kernel, rangeMax);
  }

  template <typename FunctorType>
  VTKM_CONT static inline void Schedule(FunctorType&& functor, vtkm::Id3 rangeMax)
  {
    Schedule(vtkm::cont::internal::HintList<>{}, functor, rangeMax);
  }

  //----------------------------------------------------------------------------
private:
  template <typename T>
  VTKM_CONT static void SortImpl(vtkm::cont::ArrayHandle<T>& values, vtkm::SortLess, std::true_type)
  {
    // In Kokkos 3.7, we have noticed some errors when sorting with zero-length arrays (which
    // should do nothing). There is no check, and the bin size computation gets messed up.
    if (values.GetNumberOfValues() <= 1)
    {
      return;
    }

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

protected:
  // Kokkos currently (11/10/2022) does not support a sort_by_key operator
  // so instead we are using thrust if and only if HIP or CUDA are the backends for Kokkos
#if defined(VTKM_USE_KOKKOS_THRUST)

  template <typename T, typename U, typename BinaryCompare>
  VTKM_CONT static std::enable_if_t<(std::is_same<BinaryCompare, vtkm::SortLess>::value ||
                                     std::is_same<BinaryCompare, vtkm::SortGreater>::value)>
  SortByKeyImpl(vtkm::cont::ArrayHandle<T>& keys,
                vtkm::cont::ArrayHandle<U>& values,
                BinaryCompare,
                std::true_type,
                std::true_type)
  {
    vtkm::cont::Token token;
    auto keys_portal = keys.PrepareForInPlace(vtkm::cont::DeviceAdapterTagKokkos{}, token);
    auto values_portal = values.PrepareForInPlace(vtkm::cont::DeviceAdapterTagKokkos{}, token);

    kokkos::internal::KokkosViewExec<T> keys_view(keys_portal.GetArray(),
                                                  keys_portal.GetNumberOfValues());
    kokkos::internal::KokkosViewExec<U> values_view(values_portal.GetArray(),
                                                    values_portal.GetNumberOfValues());

    thrust::device_ptr<T> keys_begin(keys_view.data());
    thrust::device_ptr<T> keys_end(keys_view.data() + keys_view.size());
    thrust::device_ptr<U> values_begin(values_view.data());

    if (std::is_same<BinaryCompare, vtkm::SortLess>::value)
    {
      thrust::sort_by_key(keys_begin, keys_end, values_begin, thrust::less<T>());
    }
    else
    {
      thrust::sort_by_key(keys_begin, keys_end, values_begin, thrust::greater<T>());
    }
  }

#endif

  template <typename T,
            typename U,
            class StorageT,
            class StorageU,
            class BinaryCompare,
            typename ValidKeys,
            typename ValidValues>
  VTKM_CONT static void SortByKeyImpl(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                      vtkm::cont::ArrayHandle<U, StorageU>& values,
                                      BinaryCompare binary_compare,
                                      ValidKeys,
                                      ValidValues)
  {
    // Default to general algorithm
    Superclass::SortByKey(keys, values, binary_compare);
  }

public:
  template <typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    // Make sure not to use the general algorithm here since
    // it will use Sort algorithm instead of SortByKey
    SortByKey(keys, values, internal::DefaultCompareFunctor());
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    // If T or U are not scalar types, or the BinaryCompare is not supported
    // then the general algorithm is called, otherwise we will run thrust
    SortByKeyImpl(keys,
                  values,
                  binary_compare,
                  typename std::is_scalar<T>::type{},
                  typename std::is_scalar<U>::type{});
  }

  //----------------------------------------------------------------------------
  // Reduce By Key

#ifdef VTKM_USE_KOKKOS_THRUST

protected:
  template <typename K, typename V, class BinaryFunctor>
  VTKM_CONT static void ReduceByKeyImpl(const vtkm::cont::ArrayHandle<K>& keys,
                                        const vtkm::cont::ArrayHandle<V>& values,
                                        vtkm::cont::ArrayHandle<K>& keys_output,
                                        vtkm::cont::ArrayHandle<V>& values_output,
                                        BinaryFunctor binary_functor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfKeys = keys.GetNumberOfValues();

    vtkm::Id num_unique_keys;
    {
      vtkm::cont::Token token;

      auto keys_portal = keys.PrepareForInput(vtkm::cont::DeviceAdapterTagKokkos{}, token);
      auto values_portal = values.PrepareForInput(vtkm::cont::DeviceAdapterTagKokkos{}, token);

      auto keys_output_portal =
        keys_output.PrepareForOutput(numberOfKeys, vtkm::cont::DeviceAdapterTagKokkos{}, token);
      auto values_output_portal =
        values_output.PrepareForOutput(numberOfKeys, vtkm::cont::DeviceAdapterTagKokkos{}, token);

      thrust::device_ptr<const K> keys_begin(keys_portal.GetArray());
      thrust::device_ptr<const K> keys_end(keys_portal.GetArray() + numberOfKeys);
      thrust::device_ptr<const V> values_begin(values_portal.GetArray());
      thrust::device_ptr<K> keys_output_begin(keys_output_portal.GetArray());
      thrust::device_ptr<V> values_output_begin(values_output_portal.GetArray());

      auto ends = thrust::reduce_by_key(keys_begin,
                                        keys_end,
                                        values_begin,
                                        keys_output_begin,
                                        values_output_begin,
                                        thrust::equal_to<K>(),
                                        binary_functor);

      num_unique_keys = ends.first - keys_output_begin;
    }

    // Resize output (reduce allocation)
    keys_output.Allocate(num_unique_keys, CopyFlag::On);
    values_output.Allocate(num_unique_keys, CopyFlag::On);
  }


  template <typename K, typename V, class BinaryFunctor>
  VTKM_CONT static void ReduceByKeyImpl(
    const vtkm::cont::ArrayHandle<K>& keys,
    const vtkm::cont::ArrayHandle<V, vtkm::cont::StorageTagConstant>& values,
    vtkm::cont::ArrayHandle<K>& keys_output,
    vtkm::cont::ArrayHandle<V>& values_output,
    BinaryFunctor binary_functor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfKeys = keys.GetNumberOfValues();

    vtkm::Id num_unique_keys;
    {
      vtkm::cont::Token token;

      auto keys_portal = keys.PrepareForInput(vtkm::cont::DeviceAdapterTagKokkos{}, token);
      auto value = values.ReadPortal().Get(0);

      auto keys_output_portal =
        keys_output.PrepareForOutput(numberOfKeys, vtkm::cont::DeviceAdapterTagKokkos{}, token);
      auto values_output_portal =
        values_output.PrepareForOutput(numberOfKeys, vtkm::cont::DeviceAdapterTagKokkos{}, token);

      thrust::device_ptr<const K> keys_begin(keys_portal.GetArray());
      thrust::device_ptr<const K> keys_end(keys_portal.GetArray() + numberOfKeys);
      thrust::constant_iterator<const V> values_begin(value);
      thrust::device_ptr<K> keys_output_begin(keys_output_portal.GetArray());
      thrust::device_ptr<V> values_output_begin(values_output_portal.GetArray());

      auto ends = thrust::reduce_by_key(keys_begin,
                                        keys_end,
                                        values_begin,
                                        keys_output_begin,
                                        values_output_begin,
                                        thrust::equal_to<K>(),
                                        binary_functor);

      num_unique_keys = ends.first - keys_output_begin;
    }

    // Resize output (reduce allocation)
    keys_output.Allocate(num_unique_keys, CopyFlag::On);
    values_output.Allocate(num_unique_keys, CopyFlag::On);
  }

  template <typename T,
            typename U,
            class KIn,
            class VIn,
            class KOut,
            class VOut,
            class BinaryFunctor>
  VTKM_CONT static void ReduceByKeyImpl(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                        const vtkm::cont::ArrayHandle<U, VIn>& values,
                                        vtkm::cont::ArrayHandle<T, KOut>& keys_output,
                                        vtkm::cont::ArrayHandle<U, VOut>& values_output,
                                        BinaryFunctor binary_functor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    Superclass::ReduceByKey(keys, values, keys_output, values_output, binary_functor);
  }

public:
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

    ReduceByKeyImpl(keys, values, keys_output, values_output, binary_functor);
  }

#endif

  //--------------------------------------------------------------------------

  VTKM_CONT static void Synchronize()
  {
    vtkm::cont::kokkos::internal::GetExecutionSpaceInstance().fence();
  }
};

//=============================================================================
template <>
class DeviceTaskTypes<vtkm::cont::DeviceAdapterTagKokkos>
{
public:
  template <typename Hints, typename WorkletType, typename InvocationType>
  VTKM_CONT static vtkm::exec::kokkos::internal::TaskBasic1D<WorkletType, InvocationType, Hints>
  MakeTask(WorkletType& worklet, InvocationType& invocation, vtkm::Id, Hints = Hints{})
  {
    return vtkm::exec::kokkos::internal::TaskBasic1D<WorkletType, InvocationType, Hints>(
      worklet, invocation);
  }

  template <typename Hints, typename WorkletType, typename InvocationType>
  VTKM_CONT static vtkm::exec::kokkos::internal::TaskBasic3D<WorkletType, InvocationType, Hints>
  MakeTask(WorkletType& worklet, InvocationType& invocation, vtkm::Id3, Hints = {})
  {
    return vtkm::exec::kokkos::internal::TaskBasic3D<WorkletType, InvocationType, Hints>(
      worklet, invocation);
  }

  template <typename WorkletType, typename InvocationType, typename RangeType>
  VTKM_CONT static auto MakeTask(WorkletType& worklet,
                                 InvocationType& invocation,
                                 const RangeType& range)
  {
    return MakeTask<vtkm::cont::internal::HintList<>>(worklet, invocation, range);
  }
};
}
} // namespace vtkm::cont

#undef VTKM_VOLATILE

#endif //vtk_m_cont_kokkos_internal_DeviceAdapterAlgorithmKokkos_h
