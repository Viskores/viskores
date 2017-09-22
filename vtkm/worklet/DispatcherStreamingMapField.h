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

template <typename ControlInterface, typename Device>
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
    typedef vtkm::cont::ArrayHandleStreaming<ArrayHandleType> type;
  };

  template <typename NotArrayHandleType>
  struct DetermineReturnType<NotArrayHandleType, false>
  {
    typedef NotArrayHandleType type;
  };

  template <typename ParameterType, vtkm::IdComponent Index>
  struct ReturnType
  {
    typedef typename DetermineReturnType<
      ParameterType,
      vtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>::type type;
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

template <typename ControlInterface, typename Device>
struct DispatcherStreamingMapFieldTransferFunctor
{
  VTKM_CONT
  DispatcherStreamingMapFieldTransferFunctor() {}

  template <typename ParameterType, vtkm::IdComponent Index>
  struct ReturnType
  {
    typedef ParameterType type;
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
template <typename WorkletType, typename Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class DispatcherStreamingMapField
  : public vtkm::worklet::internal::DispatcherBase<DispatcherStreamingMapField<WorkletType, Device>,
                                                   WorkletType,
                                                   vtkm::worklet::WorkletMapField>
{
  typedef vtkm::worklet::internal::DispatcherBase<DispatcherStreamingMapField<WorkletType, Device>,
                                                  WorkletType,
                                                  vtkm::worklet::WorkletMapField>
    Superclass;

public:
  VTKM_CONT
  DispatcherStreamingMapField(const WorkletType& worklet = WorkletType())
    : Superclass(worklet)
    , NumberOfBlocks(1)
  {
  }

  VTKM_CONT
  void SetNumberOfBlocks(vtkm::Id numberOfBlocks) { NumberOfBlocks = numberOfBlocks; }

  template <typename Invocation, typename DeviceAdapter>
  VTKM_CONT void BasicInvoke(const Invocation& invocation,
                             vtkm::Id numInstances,
                             vtkm::Id globalIndexOffset,
                             DeviceAdapter device) const
  {
    this->InvokeTransportParameters(invocation,
                                    numInstances,
                                    globalIndexOffset,
                                    this->Worklet.GetScatter().GetOutputRange(numInstances),
                                    device);
  }

  template <typename Invocation>
  VTKM_CONT void DoInvoke(const Invocation& invocation) const
  {
    // This is the type for the input domain
    typedef typename Invocation::InputDomainType InputDomainType;

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // For a DispatcherStreamingMapField, the inputDomain must be an ArrayHandle (or
    // a DynamicArrayHandle that gets cast to one). The size of the domain
    // (number of threads/worklet instances) is equal to the size of the
    // array.
    vtkm::Id fullSize = inputDomain.GetNumberOfValues();
    vtkm::Id blockSize = fullSize / NumberOfBlocks;
    if (fullSize % NumberOfBlocks != 0)
      blockSize += 1;

    typedef detail::
      DispatcherStreamingMapFieldTransformFunctor<typename Invocation::ControlInterface, Device>
        TransformFunctorType;
    typedef detail::
      DispatcherStreamingMapFieldTransferFunctor<typename Invocation::ControlInterface, Device>
        TransferFunctorType;

    for (vtkm::Id block = 0; block < NumberOfBlocks; block++)
    {
      // Account for domain sizes not evenly divisable by the number of blocks
      vtkm::Id numberOfInstances = blockSize;
      if (block == NumberOfBlocks - 1)
        numberOfInstances = fullSize - blockSize * block;
      vtkm::Id globalIndexOffset = blockSize * block;

      typedef typename Invocation::ParameterInterface ParameterInterfaceType;
      typedef
        typename ParameterInterfaceType::template StaticTransformType<TransformFunctorType>::type
          ReportedType;
      ReportedType newParams = invocation.Parameters.StaticTransformCont(
        TransformFunctorType(block, blockSize, numberOfInstances, fullSize));

      typedef typename Invocation::template ChangeParametersType<ReportedType>::type ChangedType;
      ChangedType changedParams = invocation.ChangeParameters(newParams);

      this->BasicInvoke(changedParams, numberOfInstances, globalIndexOffset, Device());

      // Loop over parameters again to sync results for this block into control array
      typedef typename ChangedType::ParameterInterface ParameterInterfaceType2;
      const ParameterInterfaceType2& parameters2 = changedParams.Parameters;
      parameters2.StaticTransformCont(TransferFunctorType());
    }
  }

private:
  template <typename Invocation,
            typename InputRangeType,
            typename OutputRangeType,
            typename DeviceAdapter>
  VTKM_CONT void InvokeTransportParameters(const Invocation& invocation,
                                           const InputRangeType& inputRange,
                                           const InputRangeType& globalIndexOffset,
                                           const OutputRangeType& outputRange,
                                           DeviceAdapter device) const
  {
    typedef typename Invocation::ParameterInterface ParameterInterfaceType;
    const ParameterInterfaceType& parameters = invocation.Parameters;

    typedef vtkm::worklet::internal::detail::DispatcherBaseTransportFunctor<
      typename Invocation::ControlInterface,
      typename Invocation::InputDomainType,
      DeviceAdapter>
      TransportFunctorType;
    typedef
      typename ParameterInterfaceType::template StaticTransformType<TransportFunctorType>::type
        ExecObjectParameters;

    ExecObjectParameters execObjectParameters = parameters.StaticTransformCont(
      TransportFunctorType(invocation.GetInputDomain(), inputRange, outputRange));

    // Get the arrays used for scattering input to output.
    typename WorkletType::ScatterType::OutputToInputMapType outputToInputMap =
      this->Worklet.GetScatter().GetOutputToInputMap(inputRange);
    typename WorkletType::ScatterType::VisitArrayType visitArray =
      this->Worklet.GetScatter().GetVisitArray(inputRange);

    // Replace the parameters in the invocation with the execution object and
    // pass to next step of Invoke. Also add the scatter information.
    this->InvokeSchedule(invocation.ChangeParameters(execObjectParameters)
                           .ChangeOutputToInputMap(outputToInputMap.PrepareForInput(device))
                           .ChangeVisitArray(visitArray.PrepareForInput(device)),
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
