//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_Invocation_h
#define vtk_m_internal_Invocation_h

#include <vtkm/Types.h>
#include <vtkm/internal/FunctionInterface.h>

namespace vtkm
{
namespace internal
{

/// \brief Container for types when dispatching worklets.
///
/// When a dispatcher and associated class invoke a worklet, they need to keep
/// track of the types of all parameters and the associated features of the
/// worklet. \c Invocation is a class that manages all these types.
///
template <typename ParameterInterface_,
          typename ControlInterface_,
          typename ExecutionInterface_,
          vtkm::IdComponent InputDomainIndex_,
          typename OutputToInputMapType_ = vtkm::internal::NullType,
          typename VisitArrayType_ = vtkm::internal::NullType,
          typename ThreadToOutputMapType_ = vtkm::internal::NullType,
          typename DeviceAdapterTag_ = vtkm::internal::NullType>
struct Invocation
{
  /// \brief The types of the parameters
  ///
  /// \c ParameterInterface is (expected to be) a \c FunctionInterface class
  /// that lists the types of the parameters for the invocation.
  ///
  using ParameterInterface = ParameterInterface_;

  /// \brief The tags of the \c ControlSignature.
  ///
  /// \c ControlInterface is (expected to be) a \c FunctionInterface class that
  /// represents the \c ControlSignature of a worklet (although dispatchers
  /// might modify the control signature to provide auxiliary information).
  ///
  using ControlInterface = ControlInterface_;

  /// \brief The tags of the \c ExecutionSignature.
  ///
  /// \c ExecutionInterface is (expected to be) a \c FunctionInterface class that
  /// represents the \c ExecutionSignature of a worklet (although dispatchers
  /// might modify the execution signature to provide auxiliary information).
  ///
  using ExecutionInterface = ExecutionInterface_;

  /// \brief The index of the input domain.
  ///
  /// When a worklet is invoked, the pool of working threads is based of some
  /// constituent element of the input (such as the points or cells). This
  /// index points to the parameter that defines this input domain.
  ///
  static constexpr vtkm::IdComponent InputDomainIndex = InputDomainIndex_;

  /// \brief An array representing the output to input map.
  ///
  /// When a worklet is invoked, there is an optional scatter operation that
  /// allows you to vary the number of outputs each input affects. This is
  /// represented with a map where each output points to an input that creates
  /// it.
  ///
  using OutputToInputMapType = OutputToInputMapType_;

  /// \brief An array containing visit indices.
  ///
  /// When a worklet is invoked, there is an optinonal scatter operation that
  /// allows you to vary the number of outputs each input affects. Thus,
  /// multiple outputs may point to the same input. The visit index uniquely
  /// identifies which instance each is.
  ///
  using VisitArrayType = VisitArrayType_;

  /// \brief An array representing the thread to output map.
  ///
  /// When a worklet is invoked, there is an optional mask operation that will
  /// prevent the worklet to be run on masked-out elements of the output. This
  /// is represented with a map where each thread points to an output it creates.
  ///
  using ThreadToOutputMapType = ThreadToOutputMapType_;

  /// \brief The tag for the device adapter on which the worklet is run.
  ///
  /// When the worklet is dispatched on a particular device, this type in the
  /// Invocation is set to the tag associated with that device.
  ///
  using DeviceAdapterTag = DeviceAdapterTag_;

  /// \brief Default Invocation constructors that holds the given parameters
  /// by reference.
  VTKM_CONT
  Invocation(const ParameterInterface& parameters,
             OutputToInputMapType outputToInputMap = OutputToInputMapType(),
             VisitArrayType visitArray = VisitArrayType(),
             ThreadToOutputMapType threadToOutputMap = ThreadToOutputMapType())
    : Parameters(parameters)
    , OutputToInputMap(outputToInputMap)
    , VisitArray(visitArray)
    , ThreadToOutputMap(threadToOutputMap)
  {
  }

  Invocation(const Invocation& src) = default;

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c Parameters replaced.
  ///
  template <typename NewParameterInterface>
  struct ChangeParametersType
  {
    using type = Invocation<NewParameterInterface,
                            ControlInterface,
                            ExecutionInterface,
                            InputDomainIndex,
                            OutputToInputMapType,
                            VisitArrayType,
                            ThreadToOutputMapType,
                            DeviceAdapterTag>;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c Parameters are replaced with those provided.
  ///
  template <typename NewParameterInterface>
  VTKM_CONT typename ChangeParametersType<NewParameterInterface>::type ChangeParameters(
    const NewParameterInterface& newParameters) const
  {
    return typename ChangeParametersType<NewParameterInterface>::type(
      newParameters, this->OutputToInputMap, this->VisitArray, this->ThreadToOutputMap);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c ControlInterface replaced.
  ///
  template <typename NewControlInterface>
  struct ChangeControlInterfaceType
  {
    using type = Invocation<ParameterInterface,
                            NewControlInterface,
                            ExecutionInterface,
                            InputDomainIndex,
                            OutputToInputMapType,
                            VisitArrayType,
                            ThreadToOutputMapType,
                            DeviceAdapterTag>;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c ControlInterface type is changed to the type given.
  ///
  template <typename NewControlInterface>
  typename ChangeControlInterfaceType<NewControlInterface>::type ChangeControlInterface(
    NewControlInterface) const
  {
    return typename ChangeControlInterfaceType<NewControlInterface>::type(
      this->Parameters, this->OutputToInputMap, this->VisitArray, this->ThreadToOutputMap);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c ExecutionInterface replaced.
  ///
  template <typename NewExecutionInterface>
  struct ChangeExecutionInterfaceType
  {
    using type = Invocation<ParameterInterface,
                            NewExecutionInterface,
                            ExecutionInterface,
                            InputDomainIndex,
                            OutputToInputMapType,
                            VisitArrayType,
                            ThreadToOutputMapType,
                            DeviceAdapterTag>;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c ExecutionInterface type is changed to the type given.
  ///
  template <typename NewExecutionInterface>
  typename ChangeExecutionInterfaceType<NewExecutionInterface>::type ChangeExecutionInterface(
    NewExecutionInterface) const
  {
    return typename ChangeExecutionInterfaceType<NewExecutionInterface>::type(
      this->Parameters, this->OutputToInputMap, this->VisitArray, this->ThreadToOutputMap);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c InputDomainIndex replaced.
  ///
  template <vtkm::IdComponent NewInputDomainIndex>
  struct ChangeInputDomainIndexType
  {
    using type = Invocation<ParameterInterface,
                            ControlInterface,
                            ExecutionInterface,
                            NewInputDomainIndex,
                            OutputToInputMapType,
                            VisitArrayType,
                            ThreadToOutputMapType,
                            DeviceAdapterTag>;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c InputDomainIndex is changed to the static number given.
  ///
  template <vtkm::IdComponent NewInputDomainIndex>
  VTKM_EXEC_CONT typename ChangeInputDomainIndexType<NewInputDomainIndex>::type
  ChangeInputDomainIndex() const
  {
    return typename ChangeInputDomainIndexType<NewInputDomainIndex>::type(
      this->Parameters, this->OutputToInputMap, this->VisitArray, this->ThreadToOutputMap);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c OutputToInputMapType replaced.
  ///
  template <typename NewOutputToInputMapType>
  struct ChangeOutputToInputMapType
  {
    using type = Invocation<ParameterInterface,
                            ControlInterface,
                            ExecutionInterface,
                            InputDomainIndex,
                            NewOutputToInputMapType,
                            VisitArrayType,
                            ThreadToOutputMapType,
                            DeviceAdapterTag>;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c OutputToInputMap is replaced with that provided.
  ///
  template <typename NewOutputToInputMapType>
  VTKM_CONT typename ChangeOutputToInputMapType<NewOutputToInputMapType>::type
  ChangeOutputToInputMap(NewOutputToInputMapType newOutputToInputMap) const
  {
    return typename ChangeOutputToInputMapType<NewOutputToInputMapType>::type(
      this->Parameters, newOutputToInputMap, this->VisitArray, this->ThreadToOutputMap);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c VisitArrayType replaced.
  ///
  template <typename NewVisitArrayType>
  struct ChangeVisitArrayType
  {
    using type = Invocation<ParameterInterface,
                            ControlInterface,
                            ExecutionInterface,
                            InputDomainIndex,
                            OutputToInputMapType,
                            NewVisitArrayType,
                            ThreadToOutputMapType,
                            DeviceAdapterTag>;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c VisitArray is replaced with that provided.
  ///
  template <typename NewVisitArrayType>
  VTKM_CONT typename ChangeVisitArrayType<NewVisitArrayType>::type ChangeVisitArray(
    NewVisitArrayType newVisitArray) const
  {
    return typename ChangeVisitArrayType<NewVisitArrayType>::type(
      this->Parameters, this->OutputToInputMap, newVisitArray, this->ThreadToOutputMap);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c ThreadToOutputMapType replaced.
  ///
  template <typename NewThreadToOutputMapType>
  struct ChangeThreadToOutputMapType
  {
    using type = Invocation<ParameterInterface,
                            ControlInterface,
                            ExecutionInterface,
                            InputDomainIndex,
                            OutputToInputMapType,
                            VisitArrayType,
                            NewThreadToOutputMapType,
                            DeviceAdapterTag>;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c OutputToInputMap is replaced with that provided.
  ///
  template <typename NewThreadToOutputMapType>
  VTKM_CONT typename ChangeThreadToOutputMapType<NewThreadToOutputMapType>::type
  ChangeThreadToOutputMap(NewThreadToOutputMapType newThreadToOutputMap) const
  {
    return typename ChangeThreadToOutputMapType<NewThreadToOutputMapType>::type(
      this->Parameters, this->OutputToInputMap, this->VisitArray, newThreadToOutputMap);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c DeviceAdapterTag replaced.
  ///
  template <typename NewDeviceAdapterTag>
  struct ChangeDeviceAdapterTagType
  {
    using type = Invocation<ParameterInterface,
                            ControlInterface,
                            ExecutionInterface,
                            InputDomainIndex,
                            OutputToInputMapType,
                            VisitArrayType,
                            ThreadToOutputMapType,
                            NewDeviceAdapterTag>;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c DeviceAdapterTag is replaced with that provided.
  ///
  template <typename NewDeviceAdapterTag>
  VTKM_CONT typename ChangeDeviceAdapterTagType<NewDeviceAdapterTag>::type ChangeDeviceAdapterTag(
    NewDeviceAdapterTag) const
  {
    return typename ChangeDeviceAdapterTagType<NewDeviceAdapterTag>::type(
      this->Parameters, this->OutputToInputMap, this->VisitArray, this->ThreadToOutputMap);
  }

  /// A convenience alias for the input domain type.
  ///
  using InputDomainType =
    typename ParameterInterface::template ParameterType<InputDomainIndex>::type;

  /// A convenience alias for the control signature tag of the input domain.
  ///
  using InputDomainTag = typename ControlInterface::template ParameterType<InputDomainIndex>::type;

  /// A convenience method to get the input domain object.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const InputDomainType& GetInputDomain() const
  {
    return vtkm::internal::ParameterGet<InputDomainIndex>(this->Parameters);
  }

  /// The state of an \c Invocation object holds the parameters of the
  /// invocation. As well as the output to input map and the visit array.
  ///
  /// This is held by by value so that when we transfer the invocation object
  /// over to CUDA it gets properly copied to the device. While we want to
  /// hold by reference to reduce the number of copies, it is not possible
  /// currently.
  ParameterInterface Parameters;
  OutputToInputMapType OutputToInputMap;
  VisitArrayType VisitArray;
  ThreadToOutputMapType ThreadToOutputMap;

private:
  // Do not allow assignment of one Invocation to another. It is too expensive.
  void operator=(const Invocation<ParameterInterface,
                                  ControlInterface,
                                  ExecutionInterface,
                                  InputDomainIndex,
                                  OutputToInputMapType,
                                  VisitArrayType,
                                  ThreadToOutputMapType,
                                  DeviceAdapterTag>&) = delete;
};

/// Convenience function for creating an Invocation object.
///
template <vtkm::IdComponent InputDomainIndex,
          typename ControlInterface,
          typename ExecutionInterface,
          typename ParameterInterface,
          typename OutputToInputMapType,
          typename VisitArrayType,
          typename ThreadToOutputMapType>
VTKM_CONT vtkm::internal::Invocation<ParameterInterface,
                                     ControlInterface,
                                     ExecutionInterface,
                                     InputDomainIndex,
                                     OutputToInputMapType,
                                     VisitArrayType,
                                     ThreadToOutputMapType>
make_Invocation(const ParameterInterface& params,
                ControlInterface,
                ExecutionInterface,
                OutputToInputMapType outputToInputMap,
                VisitArrayType visitArray,
                ThreadToOutputMapType threadToOutputMap)
{
  return vtkm::internal::Invocation<ParameterInterface,
                                    ControlInterface,
                                    ExecutionInterface,
                                    InputDomainIndex,
                                    OutputToInputMapType,
                                    VisitArrayType,
                                    ThreadToOutputMapType>(
    params, outputToInputMap, visitArray, threadToOutputMap);
}
template <vtkm::IdComponent InputDomainIndex,
          typename ControlInterface,
          typename ExecutionInterface,
          typename ParameterInterface>
VTKM_CONT vtkm::internal::
  Invocation<ParameterInterface, ControlInterface, ExecutionInterface, InputDomainIndex>
  make_Invocation(const ParameterInterface& params,
                  ControlInterface = ControlInterface(),
                  ExecutionInterface = ExecutionInterface())
{
  return vtkm::internal::make_Invocation<InputDomainIndex>(params,
                                                           ControlInterface(),
                                                           ExecutionInterface(),
                                                           vtkm::internal::NullType(),
                                                           vtkm::internal::NullType(),
                                                           vtkm::internal::NullType());
}
}
} // namespace vtkm::internal

#endif //vtk_m_internal_Invocation_h
