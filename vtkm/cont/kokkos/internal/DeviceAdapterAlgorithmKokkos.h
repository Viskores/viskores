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

#include <vtkm/cont/kokkos/internal/DeviceAdapterTagKokkos.h>

#include <vtkm/exec/kokkos/internal/TaskBasic.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <Kokkos_Core.hpp>
#include <Kokkos_DualView.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

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
  constexpr static vtkm::Id ErrorMessageMaxLength = 1024;
  using ErrorMessageStorage =
    Kokkos::DualView<char*, Kokkos::LayoutLeft, Kokkos::DefaultExecutionSpace>;

  VTKM_CONT static void CheckForErrors(ErrorMessageStorage& errorMessageStorage)
  {
    errorMessageStorage.template modify<ErrorMessageStorage::execution_space>();
    errorMessageStorage.template sync<ErrorMessageStorage::host_mirror_space>();
    if (errorMessageStorage.h_view(0) != '\0')
    {
      auto excep = vtkm::cont::ErrorExecution(errorMessageStorage.h_view.data());
      errorMessageStorage.h_view(0) = '\0'; // clear
      throw excep;
    }
  }

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

  template <typename WType, typename IType>
  VTKM_CONT static void ScheduleTask(
    vtkm::exec::kokkos::internal::TaskBasic1D<WType, IType>& functor,
    vtkm::Id numInstances)
  {
    if (numInstances < 1)
    {
      // No instances means nothing to run. Just return.
      return;
    }

    ErrorMessageStorage errorMessageStorage;
    errorMessageStorage.realloc(ErrorMessageMaxLength);
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(errorMessageStorage.d_view.data(),
                                                          ErrorMessageMaxLength);
    functor.SetErrorMessageBuffer(errorMessage);

    Kokkos::parallel_for(static_cast<std::size_t>(numInstances), functor);

    CheckForErrors(errorMessageStorage);
  }

  template <typename WType, typename IType>
  VTKM_CONT static void ScheduleTask(
    vtkm::exec::kokkos::internal::TaskBasic3D<WType, IType>& functor,
    vtkm::Id3 rangeMax)
  {
    if ((rangeMax[0] < 1) || (rangeMax[1] < 1) || (rangeMax[2] < 1))
    {
      // No instances means nothing to run. Just return.
      return;
    }

    ErrorMessageStorage errorMessageStorage;
    errorMessageStorage.realloc(ErrorMessageMaxLength);
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(errorMessageStorage.d_view.data(),
                                                          ErrorMessageMaxLength);
    functor.SetErrorMessageBuffer(errorMessage);

    Kokkos::MDRangePolicy<Kokkos::Rank<3>, Kokkos::IndexType<vtkm::Id>> policy(
      { 0, 0, 0 }, { rangeMax[0], rangeMax[1], rangeMax[2] });
    Kokkos::parallel_for(
      policy, KOKKOS_LAMBDA(vtkm::Id i, vtkm::Id j, vtkm::Id k) {
        auto flatIdx = i + (j * rangeMax[0]) + (k * rangeMax[0] * rangeMax[1]);
        functor(vtkm::Id3(i, j, k), flatIdx);
      });

    CheckForErrors(errorMessageStorage);
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

  VTKM_CONT static void Synchronize() {}
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
