//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_Dispatcher_Streaming_MapField_h
#define vtk_m_worklet_Dispatcher_Streaming_MapField_h

#include <vtkm/cont/ArrayHandleStreaming.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{

namespace detail
{

struct DispatcherStreamingTryExecuteFunctor
{
  template <typename Device, typename DispatcherBaseType, typename Invocation, typename RangeType>
  VTKM_CONT bool operator()(Device device,
                            const DispatcherBaseType* self,
                            Invocation& invocation,
                            const RangeType& dimensions,
                            const RangeType& globalIndexOffset)
  {
    self->InvokeTransportParameters(
      invocation, dimensions, globalIndexOffset, self->Scatter.GetOutputRange(dimensions), device);
    return true;
  }
};

template <typename ControlInterface>
struct DispatcherStreamingMapFieldTransformFunctor
{
  vtkm::Id BlockIndex;
  vtkm::Id BlockSize;
  vtkm::Id CurBlockSize;
  vtkm::Id FullSize;

  VTKM_CONT
  DispatcherStreamingMapFieldTransformFunctor(vtkm::Id blockIndex,
                                              vtkm::Id blockSize,
                                              vtkm::Id curBlockSize,
                                              vtkm::Id fullSize)
    : BlockIndex(blockIndex)
    , BlockSize(blockSize)
    , CurBlockSize(curBlockSize)
    , FullSize(fullSize)
  {
  }

  template <typename ParameterType, bool IsArrayHandle>
  struct DetermineReturnType;

  template <typename ArrayHandleType>
  struct DetermineReturnType<ArrayHandleType, true>
  {
    using type = vtkm::cont::ArrayHandleStreaming<ArrayHandleType>;
  };

  template <typename NotArrayHandleType>
  struct DetermineReturnType<NotArrayHandleType, false>
  {
    using type = NotArrayHandleType;
  };

  template <typename ParameterType, vtkm::IdComponent Index>
  struct ReturnType
  {
    using type = typename DetermineReturnType<
      ParameterType,
      vtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>::type;
  };

  template <typename ParameterType, bool IsArrayHandle>
  struct TransformImpl;

  template <typename ArrayHandleType>
  struct TransformImpl<ArrayHandleType, true>
  {
    VTKM_CONT
    vtkm::cont::ArrayHandleStreaming<ArrayHandleType> operator()(const ArrayHandleType& array,
                                                                 vtkm::Id blockIndex,
                                                                 vtkm::Id blockSize,
                                                                 vtkm::Id curBlockSize,
                                                                 vtkm::Id fullSize) const
    {
      vtkm::cont::ArrayHandleStreaming<ArrayHandleType> result =
        vtkm::cont::ArrayHandleStreaming<ArrayHandleType>(
          array, blockIndex, blockSize, curBlockSize);
      if (blockIndex == 0)
        result.AllocateFullArray(fullSize);
      return result;
    }
  };

  template <typename NotArrayHandleType>
  struct TransformImpl<NotArrayHandleType, false>
  {
    VTKM_CONT
    NotArrayHandleType operator()(const NotArrayHandleType& notArray) const { return notArray; }
  };

  template <typename ParameterType, vtkm::IdComponent Index>
  VTKM_CONT typename ReturnType<ParameterType, Index>::type operator()(
    const ParameterType& invokeData,
    vtkm::internal::IndexTag<Index>) const
  {
    return TransformImpl<ParameterType,
                         vtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>()(
      invokeData, this->BlockIndex, this->BlockSize, this->CurBlockSize, this->FullSize);
  }
};

template <typename ControlInterface>
struct DispatcherStreamingMapFieldTransferFunctor
{
  VTKM_CONT
  DispatcherStreamingMapFieldTransferFunctor() {}

  template <typename ParameterType, vtkm::IdComponent Index>
  struct ReturnType
  {
    using type = ParameterType;
  };

  template <typename ParameterType, bool IsArrayHandle>
  struct TransformImpl;

  template <typename ArrayHandleType>
  struct TransformImpl<ArrayHandleType, true>
  {
    VTKM_CONT
    ArrayHandleType operator()(const ArrayHandleType& array) const
    {
      array.SyncControlArray();
      return array;
    }
  };

  template <typename NotArrayHandleType>
  struct TransformImpl<NotArrayHandleType, false>
  {
    VTKM_CONT
    NotArrayHandleType operator()(const NotArrayHandleType& notArray) const { return notArray; }
  };

  template <typename ParameterType, vtkm::IdComponent Index>
  VTKM_CONT typename ReturnType<ParameterType, Index>::type operator()(
    const ParameterType& invokeData,
    vtkm::internal::IndexTag<Index>) const
  {
    return TransformImpl<ParameterType,
                         vtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>()(
      invokeData);
  }
};
}

/// \brief Dispatcher for worklets that inherit from \c WorkletMapField.
///
template <typename WorkletType>
class DispatcherStreamingMapField
  : public vtkm::worklet::internal::DispatcherBase<DispatcherStreamingMapField<WorkletType>,
                                                   WorkletType,
                                                   vtkm::worklet::WorkletMapField>
{
  using Superclass =
    vtkm::worklet::internal::DispatcherBase<DispatcherStreamingMapField<WorkletType>,
                                            WorkletType,
                                            vtkm::worklet::WorkletMapField>;
  using ScatterType = typename Superclass::ScatterType;
  using MaskType = typename WorkletType::MaskType;

public:
  template <typename... T>
  VTKM_CONT DispatcherStreamingMapField(T&&... args)
    : Superclass(std::forward<T>(args)...)
    , NumberOfBlocks(1)
  {
  }

  VTKM_CONT
  void SetNumberOfBlocks(vtkm::Id numberOfBlocks) { NumberOfBlocks = numberOfBlocks; }

  friend struct detail::DispatcherStreamingTryExecuteFunctor;

  template <typename Invocation>
  VTKM_CONT void BasicInvoke(Invocation& invocation,
                             vtkm::Id numInstances,
                             vtkm::Id globalIndexOffset) const
  {
    bool success = vtkm::cont::TryExecuteOnDevice(this->GetDevice(),
                                                  detail::DispatcherStreamingTryExecuteFunctor(),
                                                  this,
                                                  invocation,
                                                  numInstances,
                                                  globalIndexOffset);
    if (!success)
    {
      throw vtkm::cont::ErrorExecution("Failed to execute worklet on any device.");
    }
  }

  template <typename Invocation>
  VTKM_CONT void DoInvoke(Invocation& invocation) const
  {
    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // For a DispatcherStreamingMapField, the inputDomain must be an ArrayHandle (or
    // an VariantArrayHandle that gets cast to one). The size of the domain
    // (number of threads/worklet instances) is equal to the size of the
    // array.
    vtkm::Id fullSize = internal::scheduling_range(inputDomain);
    vtkm::Id blockSize = fullSize / NumberOfBlocks;
    if (fullSize % NumberOfBlocks != 0)
      blockSize += 1;

    using TransformFunctorType =
      detail::DispatcherStreamingMapFieldTransformFunctor<typename Invocation::ControlInterface>;
    using TransferFunctorType =
      detail::DispatcherStreamingMapFieldTransferFunctor<typename Invocation::ControlInterface>;

    for (vtkm::Id block = 0; block < NumberOfBlocks; block++)
    {
      // Account for domain sizes not evenly divisable by the number of blocks
      vtkm::Id numberOfInstances = blockSize;
      if (block == NumberOfBlocks - 1)
        numberOfInstances = fullSize - blockSize * block;
      vtkm::Id globalIndexOffset = blockSize * block;

      using ParameterInterfaceType = typename Invocation::ParameterInterface;
      using ReportedType =
        typename ParameterInterfaceType::template StaticTransformType<TransformFunctorType>::type;
      ReportedType newParams = invocation.Parameters.StaticTransformCont(
        TransformFunctorType(block, blockSize, numberOfInstances, fullSize));

      using ChangedType = typename Invocation::template ChangeParametersType<ReportedType>::type;
      ChangedType changedParams = invocation.ChangeParameters(newParams);

      this->BasicInvoke(changedParams, numberOfInstances, globalIndexOffset);

      // Loop over parameters again to sync results for this block into control array
      using ParameterInterfaceType2 = typename ChangedType::ParameterInterface;
      ParameterInterfaceType2& parameters2 = changedParams.Parameters;
      parameters2.StaticTransformCont(TransferFunctorType());
    }
  }

private:
  template <typename Invocation,
            typename InputRangeType,
            typename OutputRangeType,
            typename DeviceAdapter>
  VTKM_CONT void InvokeTransportParameters(Invocation& invocation,
                                           const InputRangeType& inputRange,
                                           const InputRangeType& globalIndexOffset,
                                           const OutputRangeType& outputRange,
                                           DeviceAdapter device) const
  {
    using ParameterInterfaceType = typename Invocation::ParameterInterface;
    ParameterInterfaceType& parameters = invocation.Parameters;

    using TransportFunctorType = vtkm::worklet::internal::detail::DispatcherBaseTransportFunctor<
      typename Invocation::ControlInterface,
      typename Invocation::InputDomainType,
      DeviceAdapter>;
    using ExecObjectParameters =
      typename ParameterInterfaceType::template StaticTransformType<TransportFunctorType>::type;

    ExecObjectParameters execObjectParameters = parameters.StaticTransformCont(
      TransportFunctorType(invocation.GetInputDomain(), inputRange, outputRange));

    // Get the arrays used for scattering input to output.
    typename ScatterType::OutputToInputMapType outputToInputMap =
      this->Scatter.GetOutputToInputMap(inputRange);
    typename ScatterType::VisitArrayType visitArray = this->Scatter.GetVisitArray(inputRange);

    // Get the arrays used for masking output elements.
    typename MaskType::ThreadToOutputMapType threadToOutputMap =
      this->Mask.GetThreadToOutputMap(inputRange);

    // Replace the parameters in the invocation with the execution object and
    // pass to next step of Invoke. Also add the scatter information.
    this->InvokeSchedule(invocation.ChangeParameters(execObjectParameters)
                           .ChangeOutputToInputMap(outputToInputMap.PrepareForInput(device))
                           .ChangeVisitArray(visitArray.PrepareForInput(device))
                           .ChangeThreadToOutputMap(threadToOutputMap.PrepareForInput(device)),
                         outputRange,
                         globalIndexOffset,
                         device);
  }

  template <typename Invocation, typename RangeType, typename DeviceAdapter>
  VTKM_CONT void InvokeSchedule(const Invocation& invocation,
                                RangeType range,
                                RangeType globalIndexOffset,
                                DeviceAdapter) const
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    using TaskTypes = typename vtkm::cont::DeviceTaskTypes<DeviceAdapter>;

    // The TaskType class handles the magic of fetching values
    // for each instance and calling the worklet's function.
    // The TaskType will evaluate to one of the following classes:
    //
    // vtkm::exec::internal::TaskSingular
    // vtkm::exec::internal::TaskTiling1D
    // vtkm::exec::internal::TaskTiling3D
    auto task = TaskTypes::MakeTask(this->Worklet, invocation, range, globalIndexOffset);
    Algorithm::ScheduleTask(task, range);
  }

  vtkm::Id NumberOfBlocks;
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_Dispatcher_Streaming_MapField_h
