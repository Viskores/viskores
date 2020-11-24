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

VTKM_THIRDPARTY_PRE_INCLUDE
#include <Kokkos_Core.hpp>
#include <Kokkos_DualView.hpp>
#include <Kokkos_Sort.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <type_traits>

namespace vtkm
{
namespace cont
{

namespace kokkos
{
namespace internal
{

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
