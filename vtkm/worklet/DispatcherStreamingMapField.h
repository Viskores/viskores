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
#ifndef vtk_m_worklet_Dispatcher_MapField_h
#define vtk_m_worklet_Dispatcher_MapField_h

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm {
namespace worklet {

namespace detail {


template<typename ControlInterface, typename Device>
struct DispatcherStreamingMapFieldTransformFunctor
{
  vtkm::Id BlockIndex;
  vtkm::Id BlockSize;
  vtkm::Id CurBlockSize;
  vtkm::Id FullSize;

  VTKM_CONT_EXPORT
  DispatcherStreamingMapFieldTransformFunctor(
      vtkm::Id blockIndex, vtkm::Id blockSize, vtkm::Id curBlockSize, vtkm::Id fullSize)
    : BlockIndex(blockIndex), BlockSize(blockSize), 
      CurBlockSize(curBlockSize), FullSize(fullSize) {  }

  template<typename ParameterType, bool IsArrayHandle>
  struct DetermineReturnType;

  template<typename ArrayHandleType>
  struct DetermineReturnType<ArrayHandleType, true>
  {
    typedef vtkm::cont::ArrayHandleStreaming<ArrayHandleType> type;
  };

  template<typename NotArrayHandleType>
  struct DetermineReturnType<NotArrayHandleType, false>
  {
    typedef NotArrayHandleType type;
  };

  template<typename ParameterType, vtkm::IdComponent Index>
  struct ReturnType {
    typedef typename DetermineReturnType<ParameterType,
        vtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>::type type;
  };

  template<typename ParameterType, bool IsArrayHandle>
  struct TransformImpl;

  template<typename ArrayHandleType>
  struct TransformImpl<ArrayHandleType, true>
  {
    VTKM_CONT_EXPORT
    vtkm::cont::ArrayHandleStreaming<ArrayHandleType>
    operator()(const ArrayHandleType &array, vtkm::Id blockIndex, 
               vtkm::Id blockSize, vtkm::Id curBlockSize, vtkm::Id fullSize) const
    {
      vtkm::cont::ArrayHandleStreaming<ArrayHandleType> result = 
          vtkm::cont::ArrayHandleStreaming<ArrayHandleType>(
          array, blockIndex, blockSize, curBlockSize);
      if (blockIndex == 0) result.AllocateFullArray(fullSize);
      return result;
    }
  };

  template<typename NotArrayHandleType>
  struct TransformImpl<NotArrayHandleType, false>
  {
    VTKM_CONT_EXPORT
    NotArrayHandleType operator()(const NotArrayHandleType &notArray) const
    {
      return notArray;
    }
  };

  template<typename ParameterType, vtkm::IdComponent Index>
  VTKM_CONT_EXPORT
  typename ReturnType<ParameterType, Index>::type
  operator()(const ParameterType &invokeData,
             vtkm::internal::IndexTag<Index>) const
  {
    return TransformImpl<ParameterType, 
      vtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>()
         (invokeData, this->BlockIndex, this->BlockSize, this->CurBlockSize, this->FullSize);
  }
};


template<typename ControlInterface, typename Device>
struct DispatcherStreamingMapFieldTransferFunctor
{
  VTKM_CONT_EXPORT
  DispatcherStreamingMapFieldTransferFunctor()  {  }

  template<typename ParameterType, bool IsArrayHandle>
  struct DetermineReturnType;

  template<typename ArrayHandleType>
  struct DetermineReturnType<ArrayHandleType, true>
  {
    typedef vtkm::cont::ArrayHandleStreaming<ArrayHandleType> type;
  };

  template<typename NotArrayHandleType>
  struct DetermineReturnType<NotArrayHandleType, false>
  {
    typedef NotArrayHandleType type;
  };

  template<typename ParameterType, vtkm::IdComponent Index>
  struct ReturnType {
    typedef typename DetermineReturnType<ParameterType,
        vtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>::type type;
  };

  template<typename ParameterType, bool IsArrayHandle>
  struct TransformImpl;

  template<typename ArrayHandleType>
  struct TransformImpl<ArrayHandleType, true>
  {
    VTKM_CONT_EXPORT
    vtkm::cont::ArrayHandleStreaming<ArrayHandleType>
    operator()(const ArrayHandleType &array) const
    {
      vtkm::cont::ArrayHandleStreaming<ArrayHandleType> result =
          vtkm::cont::ArrayHandleStreaming<ArrayHandleType>(
          array, 0, 0, 0);
      //if (blockIndex == 0) result.AllocateFullArray(fullSize);
      return result;
    }
  };

  template<typename NotArrayHandleType>
  struct TransformImpl<NotArrayHandleType, false>
  {
    VTKM_CONT_EXPORT
    NotArrayHandleType operator()(const NotArrayHandleType &notArray) const
    {
      return notArray;
    }
  };

  template<typename ParameterType, vtkm::IdComponent Index>
  VTKM_CONT_EXPORT
  typename ReturnType<ParameterType, Index>::type
  operator()(const ParameterType &invokeData,
             vtkm::internal::IndexTag<Index>) const
  {
    return TransformImpl<ParameterType,
      vtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>()(invokeData);
  }

  /*template<typename ParameterType, vtkm::IdComponent Index>
  struct ReturnType {
    typedef ParameterType type;
  };

  template<typename ParameterType, vtkm::IdComponent Index>
  VTKM_CONT_EXPORT
  ParameterType operator()(const ParameterType &invokeData, vtkm::internal::IndexTag<Index>) const
  {
    return invokeData;
  }*/
};

}


/// \brief Dispatcher for worklets that inherit from \c WorkletMapField.
///
template<typename WorkletType,
         typename Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class DispatcherStreamingMapField :
    public vtkm::worklet::internal::DispatcherBase<
      DispatcherStreamingMapField<WorkletType,Device>,
      WorkletType,
      vtkm::worklet::WorkletMapField>
{
  typedef vtkm::worklet::internal::DispatcherBase<
    DispatcherStreamingMapField<WorkletType,Device>,
    WorkletType,
    vtkm::worklet::WorkletMapField> Superclass;

public:
  VTKM_CONT_EXPORT
  DispatcherStreamingMapField(const WorkletType &worklet = WorkletType())
    : Superclass(worklet), NumberOfBlocks(1) {  }

  VTKM_CONT_EXPORT
  void SetNumberOfBlocks(vtkm::Id numberOfBlocks) 
  {
    NumberOfBlocks = numberOfBlocks;
  }

  template<typename Invocation>
  VTKM_CONT_EXPORT
  void DoInvoke(const Invocation &invocation) const
  {
    // This is the type for the input domain
    typedef typename Invocation::InputDomainType InputDomainType;

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType &inputDomain = invocation.GetInputDomain();

    // For a DispatcherStreamingMapField, the inputDomain must be an ArrayHandle (or
    // a DynamicArrayHandle that gets cast to one). The size of the domain
    // (number of threads/worklet instances) is equal to the size of the
    // array.
    vtkm::Id fullSize = inputDomain.GetNumberOfValues();
    vtkm::Id blockSize = fullSize / NumberOfBlocks;
    if (fullSize % NumberOfBlocks != 0) blockSize += 1;
   
    typedef detail::DispatcherStreamingMapFieldTransformFunctor<
        typename Invocation::ControlInterface, Device> TransformFunctorType;
    typedef detail::DispatcherStreamingMapFieldTransferFunctor<
        typename Invocation::ControlInterface, Device> TransferFunctorType;


    for (vtkm::Id block=0; block<NumberOfBlocks; block++)
    {
      // Account for domain sizes not evenly divisable by the number of blocks
      vtkm::Id numberOfInstances = blockSize;
      if (block == NumberOfBlocks-1) 
        numberOfInstances = fullSize - blockSize*block;

/*      Temp test(invocation.ChangeParameters(
          invocation.Parameters.StaticTransformCont(
              TransformFunctorType(block, blockSize, numberOfInstances, fullSize))));
*/
      this->BasicInvoke(
        invocation.ChangeParameters(
          invocation.Parameters.StaticTransformCont(
              TransformFunctorType(block, blockSize, numberOfInstances, fullSize))),
          numberOfInstances,
          Device());

      invocation.Parameters.StaticTransformCont(TransferFunctorType());
    }
  }

protected:
  vtkm::Id NumberOfBlocks;

};

}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_Dispatcher_MapField_h
