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
#ifndef vtk_m_worklet_internal_DispatcherBase_h
#define vtk_m_worklet_internal_DispatcherBase_h

#include <vtkm/StaticAssert.h>

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/ErrorBadType.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/Transport.h>
#include <vtkm/cont/arg/TypeCheck.h>
#include <vtkm/cont/internal/DynamicTransform.h>

#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>

#include <vtkm/internal/IntegerSequence.h>
#include <vtkm/internal/brigand.hpp>

#include <sstream>

namespace vtkm
{
namespace worklet
{
namespace internal
{

namespace detail
{

// This code is actually taking an error found at compile-time and not
// reporting it until run-time. This seems strange at first, but this
// behavior is actually important. With dynamic arrays and similar dynamic
// classes, there may be types that are technically possible (such as using a
// vector where a scalar is expected) but in reality never happen. Thus, for
// these unsupported combinations we just silently halt the compiler from
// attempting to create code for these errant conditions and throw a run-time
// error if one every tries to create one.
inline void PrintFailureMessage(int, std::true_type)
{
}
inline void PrintFailureMessage(int index, std::false_type)
{
  std::stringstream message;
  message << "Encountered bad type for parameter " << index
          << " when calling Invoke on a dispatcher.";
  throw vtkm::cont::ErrorBadType(message.str());
}

// Is designed as a brigand fold operation.
template <typename T, typename State>
struct DetermineIfHasDynamicParameter
{
  using DynamicTag = typename vtkm::cont::internal::DynamicTransformTraits<T>::DynamicTag;
  using isDynamic =
    typename std::is_same<DynamicTag, vtkm::cont::internal::DynamicTransformTagCastAndCall>::type;

  using type = std::integral_constant<bool, (State::value || isDynamic::value)>;
};

// Is designed as a brigand fold operation.
template <typename Index, typename Params, typename SigTypes>
struct DetermineHasInCorrectParameters
{
  using T = typename brigand::at_c<Params, Index::value>;
  using ControlSignatureTag = typename brigand::at_c<SigTypes, Index::value>;
  using TypeCheckTag = typename ControlSignatureTag::TypeCheckTag;

  using type = std::integral_constant<bool, vtkm::cont::arg::TypeCheck<TypeCheckTag, T>::value>;

  static_assert(type::value,
                "Unable to match 'ValueType' to the signature tag 'ControlSignatureTag'");
};

// Checks that an argument in a ControlSignature is a valid control signature
// tag. Causes a compile error otherwise.
struct DispatcherBaseControlSignatureTagCheck
{
  template <typename ControlSignatureTag, vtkm::IdComponent Index>
  struct ReturnType
  {
    // If you get a compile error here, it means there is something that is
    // not a valid control signature tag in a worklet's ControlSignature.
    VTKM_IS_CONTROL_SIGNATURE_TAG(ControlSignatureTag);
    typedef ControlSignatureTag type;
  };
};

// Checks that an argument in a ExecutionSignature is a valid execution
// signature tag. Causes a compile error otherwise.
struct DispatcherBaseExecutionSignatureTagCheck
{
  template <typename ExecutionSignatureTag, vtkm::IdComponent Index>
  struct ReturnType
  {
    // If you get a compile error here, it means there is something that is not
    // a valid execution signature tag in a worklet's ExecutionSignature.
    VTKM_IS_EXECUTION_SIGNATURE_TAG(ExecutionSignatureTag);
    typedef ExecutionSignatureTag type;
  };
};

// Used in the dynamic cast to check to make sure that the type passed into
// the Invoke method matches the type accepted by the ControlSignature.
template <typename ContinueFunctor, typename TypeCheckTag, vtkm::IdComponent Index>
struct DispatcherBaseTypeCheckFunctor
{
  const ContinueFunctor& Continue;

  VTKM_CONT
  DispatcherBaseTypeCheckFunctor(const ContinueFunctor& continueFunc)
    : Continue(continueFunc)
  {
  }

  template <typename T>
  VTKM_CONT void operator()(const T& x) const
  {
    typedef std::integral_constant<bool, vtkm::cont::arg::TypeCheck<TypeCheckTag, T>::value>
      CanContinueTagType;

    vtkm::worklet::internal::detail::PrintFailureMessage(Index, CanContinueTagType());
    this->WillContinue(x, CanContinueTagType());
  }

private:
  template <typename T>
  VTKM_CONT void WillContinue(const T& x, std::true_type) const
  {
    this->Continue(x);
  }

  template <typename T>
  VTKM_CONT void WillContinue(const T&, std::false_type) const
  {
  }

  void operator=(const DispatcherBaseTypeCheckFunctor<ContinueFunctor, TypeCheckTag, Index>&) =
    delete;
};

// Uses vtkm::cont::internal::DynamicTransform and the DynamicTransformCont
// method of FunctionInterface to convert all DynamicArrayHandles and any
// other arguments declaring themselves as dynamic to static versions.
template <typename ControlInterface>
struct DispatcherBaseDynamicTransform
{
  vtkm::cont::internal::DynamicTransform BasicDynamicTransform;

  template <typename InputType, typename ContinueFunctor, vtkm::IdComponent Index>
  VTKM_CONT void operator()(const InputType& input,
                            const ContinueFunctor& continueFunc,
                            const vtkm::internal::IndexTag<Index>& indexTag) const
  {
    typedef typename ControlInterface::template ParameterType<Index>::type ControlSignatureTag;

    typedef DispatcherBaseTypeCheckFunctor<ContinueFunctor,
                                           typename ControlSignatureTag::TypeCheckTag,
                                           Index>
      TypeCheckFunctor;

    this->BasicDynamicTransform(input, TypeCheckFunctor(continueFunc), indexTag);
  }
};

// A functor called at the end of the dynamic transform to call the next
// step in the dynamic transform.
template <typename DispatcherBaseType>
struct DispatcherBaseDynamicTransformHelper
{
  const DispatcherBaseType* Dispatcher;

  VTKM_CONT
  DispatcherBaseDynamicTransformHelper(const DispatcherBaseType* dispatcher)
    : Dispatcher(dispatcher)
  {
  }

  template <typename FunctionInterface>
  VTKM_CONT void operator()(const FunctionInterface& parameters) const
  {
    this->Dispatcher->DynamicTransformInvoke(parameters, std::true_type());
  }
};

// A look up helper used by DispatcherBaseTransportFunctor to determine
//the types independent of the device we are templated on.
template <typename ControlInterface, vtkm::IdComponent Index>
struct DispatcherBaseTransportInvokeTypes
{
  //Moved out of DispatcherBaseTransportFunctor to reduce code generation
  typedef typename ControlInterface::template ParameterType<Index>::type ControlSignatureTag;
  typedef typename ControlSignatureTag::TransportTag TransportTag;
};

VTKM_CONT
inline vtkm::Id FlatRange(vtkm::Id range)
{
  return range;
}

VTKM_CONT
inline vtkm::Id FlatRange(const vtkm::Id3& range)
{
  return range[0] * range[1] * range[2];
}

// A functor used in a StaticCast of a FunctionInterface to transport arguments
// from the control environment to the execution environment.
template <typename ControlInterface, typename InputDomainType, typename Device>
struct DispatcherBaseTransportFunctor
{
  const InputDomainType& InputDomain; // Warning: this is a reference
  vtkm::Id InputRange;
  vtkm::Id OutputRange;

  // TODO: We need to think harder about how scheduling on 3D arrays works.
  // Chances are we need to allow the transport for each argument to manage
  // 3D indices (for example, allocate a 3D array instead of a 1D array).
  // But for now, just treat all transports as 1D arrays.
  template <typename InputRangeType, typename OutputRangeType>
  VTKM_CONT DispatcherBaseTransportFunctor(const InputDomainType& inputDomain,
                                           const InputRangeType& inputRange,
                                           const OutputRangeType& outputRange)
    : InputDomain(inputDomain)
    , InputRange(FlatRange(inputRange))
    , OutputRange(FlatRange(outputRange))
  {
  }

  template <typename ControlParameter, vtkm::IdComponent Index>
  struct ReturnType
  {
    using TransportTag =
      typename DispatcherBaseTransportInvokeTypes<ControlInterface, Index>::TransportTag;
    using TransportType =
      typename vtkm::cont::arg::Transport<TransportTag, ControlParameter, Device>;
    using type = typename TransportType::ExecObjectType;
  };

  template <typename ControlParameter, vtkm::IdComponent Index>
  VTKM_CONT typename ReturnType<ControlParameter, Index>::type operator()(
    const ControlParameter& invokeData,
    vtkm::internal::IndexTag<Index>) const
  {
    using TransportTag =
      typename DispatcherBaseTransportInvokeTypes<ControlInterface, Index>::TransportTag;
    vtkm::cont::arg::Transport<TransportTag, ControlParameter, Device> transport;
    return transport(invokeData, this->InputDomain, this->InputRange, this->OutputRange);
  }

private:
  void operator=(const DispatcherBaseTransportFunctor&) = delete;
};

} // namespace detail

/// Base class for all dispatcher classes. Every worklet type should have its
/// own dispatcher.
///
template <typename DerivedClass, typename WorkletType, typename BaseWorkletType>
class DispatcherBase
{
private:
  typedef DispatcherBase<DerivedClass, WorkletType, BaseWorkletType> MyType;

  friend struct detail::DispatcherBaseDynamicTransformHelper<MyType>;

protected:
  typedef vtkm::internal::FunctionInterface<typename WorkletType::ControlSignature>
    ControlInterface;
  typedef vtkm::internal::FunctionInterface<typename WorkletType::ExecutionSignature>
    ExecutionInterface;

  static const vtkm::IdComponent NUM_INVOKE_PARAMS = ControlInterface::ARITY;

private:
  // We don't really need these types, but declaring them checks the arguments
  // of the control and execution signatures.
  typedef typename ControlInterface::template StaticTransformType<
    detail::DispatcherBaseControlSignatureTagCheck>::type ControlSignatureCheck;
  typedef typename ExecutionInterface::template StaticTransformType<
    detail::DispatcherBaseExecutionSignatureTagCheck>::type ExecutionSignatureCheck;

  template <typename Signature>
  VTKM_CONT void StartInvoke(const vtkm::internal::FunctionInterface<Signature>& parameters) const
  {
    using ParameterInterface = vtkm::internal::FunctionInterface<Signature>;

    VTKM_STATIC_ASSERT_MSG(ParameterInterface::ARITY == NUM_INVOKE_PARAMS,
                           "Dispatcher Invoke called with wrong number of arguments.");

    static_assert(
      std::is_base_of<BaseWorkletType, WorkletType>::value,
      "The worklet being scheduled by this dispatcher doesn't match the type of the dispatcher");

    //We need to determine if we have the need to do any dynamic
    //transforms. This is fairly simple of a query. We just need to check
    //everything in the FunctionInterface and see if any of them have the
    //proper dynamic trait. Doing this, allows us to generate zero dynamic
    //check & convert code when we already know all the types. This results
    //in smaller executables and libraries.
    using ParamTypes = typename ParameterInterface::ParameterSig;
    using HasDynamicTypes =
      brigand::fold<ParamTypes,
                    std::false_type,
                    detail::DetermineIfHasDynamicParameter<brigand::_element, brigand::_state>>;

    this->StartInvokeDynamic(parameters, HasDynamicTypes());
  }

  template <typename Signature>
  VTKM_CONT void StartInvokeDynamic(const vtkm::internal::FunctionInterface<Signature>& parameters,
                                    std::true_type) const
  {
    // As we do the dynamic transform, we are also going to check the static
    // type against the TypeCheckTag in the ControlSignature tags. To do this,
    // the check needs access to both the parameter (in the parameters
    // argument) and the ControlSignature tags (in the ControlInterface type).
    // To make this possible, we call DynamicTransform with a functor containing
    // the control signature tags. It uses the index provided by the
    // dynamic transform mechanism to get the right tag and make sure that
    // the dynamic type is correct. (This prevents the compiler from expanding
    // worklets with types that should not be.)
    parameters.DynamicTransformCont(detail::DispatcherBaseDynamicTransform<ControlInterface>(),
                                    detail::DispatcherBaseDynamicTransformHelper<MyType>(this));
  }

  template <typename Signature>
  VTKM_CONT void StartInvokeDynamic(const vtkm::internal::FunctionInterface<Signature>& parameters,
                                    std::false_type) const
  {
    using ParameterInterface = vtkm::internal::FunctionInterface<Signature>;

    //Nothing requires a conversion from dynamic to static types, so
    //next we need to verify that each argument's type is correct. If not
    //we need to throw a nice compile time error
    using ParamTypes = typename ParameterInterface::ParameterSig;
    using ContSigTypes = typename vtkm::internal::detail::FunctionSigInfo<
      typename WorkletType::ControlSignature>::Parameters;
    using NumParams = vtkm::internal::MakeIntegerSequence<ParameterInterface::ARITY>;

    using isAllValid = brigand::fold<
      NumParams,
      std::true_type,
      detail::DetermineHasInCorrectParameters<brigand::_element, ParamTypes, ContSigTypes>>;
    //When isAllValid is false we produce a second static_assert
    //stating that the static transform is not possible
    static_assert(isAllValid::value, "Unable to match all parameter types");

    this->DynamicTransformInvoke(parameters, isAllValid());
  }

  template <typename Signature>
  VTKM_CONT void DynamicTransformInvoke(
    const vtkm::internal::FunctionInterface<Signature>& parameters,
    std::true_type) const
  {
    // TODO: Check parameters
    static const vtkm::IdComponent INPUT_DOMAIN_INDEX = WorkletType::InputDomain::INDEX;
    reinterpret_cast<const DerivedClass*>(this)->DoInvoke(
      vtkm::internal::make_Invocation<INPUT_DOMAIN_INDEX>(
        parameters, ControlInterface(), ExecutionInterface()));
  }

  template <typename Signature>
  VTKM_CONT void DynamicTransformInvoke(const vtkm::internal::FunctionInterface<Signature>&,
                                        std::false_type) const
  {
  }

public:
  template <typename... ArgTypes>
  VTKM_CONT void Invoke(ArgTypes... args) const
  {
    this->StartInvoke(vtkm::internal::make_FunctionInterface<void>(args...));
  }

protected:
  VTKM_CONT
  DispatcherBase(const WorkletType& worklet)
    : Worklet(worklet)
  {
  }

  template <typename Invocation, typename DeviceAdapter>
  VTKM_CONT void BasicInvoke(const Invocation& invocation,
                             vtkm::Id numInstances,
                             DeviceAdapter device) const
  {
    this->InvokeTransportParameters(
      invocation, numInstances, this->Worklet.GetScatter().GetOutputRange(numInstances), device);
  }

  template <typename Invocation, typename DeviceAdapter>
  VTKM_CONT void BasicInvoke(const Invocation& invocation,
                             vtkm::Id2 dimensions,
                             DeviceAdapter device) const
  {
    this->BasicInvoke(invocation, vtkm::Id3(dimensions[0], dimensions[1], 1), device);
  }

  template <typename Invocation, typename DeviceAdapter>
  VTKM_CONT void BasicInvoke(const Invocation& invocation,
                             vtkm::Id3 dimensions,
                             DeviceAdapter device) const
  {
    this->InvokeTransportParameters(
      invocation, dimensions, this->Worklet.GetScatter().GetOutputRange(dimensions), device);
  }

  WorkletType Worklet;

private:
  // Dispatchers cannot be copied
  DispatcherBase(const MyType&) = delete;
  void operator=(const MyType&) = delete;

  template <typename Invocation,
            typename InputRangeType,
            typename OutputRangeType,
            typename DeviceAdapter>
  VTKM_CONT void InvokeTransportParameters(const Invocation& invocation,
                                           const InputRangeType& inputRange,
                                           OutputRangeType&& outputRange,
                                           DeviceAdapter device) const
  {
    // The first step in invoking a worklet is to transport the arguments to
    // the execution environment. The invocation object passed to this function
    // contains the parameters passed to Invoke in the control environment. We
    // will use the template magic in the FunctionInterface class to invoke the
    // appropriate Transport class on each parameter and get a list of
    // execution objects (corresponding to the arguments of the Invoke in the
    // control environment) in a FunctionInterface. Specifically, we use a
    // static transform of the FunctionInterface to call the transport on each
    // argument and return the corresponding execution environment object.
    typedef typename Invocation::ParameterInterface ParameterInterfaceType;
    const ParameterInterfaceType& parameters = invocation.Parameters;

    typedef detail::DispatcherBaseTransportFunctor<typename Invocation::ControlInterface,
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
                         device);
  }

  template <typename Invocation, typename RangeType, typename DeviceAdapter>
  VTKM_CONT void InvokeSchedule(const Invocation& invocation, RangeType range, DeviceAdapter) const
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
    auto task = TaskTypes::MakeTask(this->Worklet, invocation, range);
    Algorithm::ScheduleTask(task, range);
  }
};
}
}
} // namespace vtkm::worklet::internal

#endif //vtk_m_worklet_internal_DispatcherBase_h
