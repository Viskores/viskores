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

#include <vtkm/internal/brigand.hpp>

#include <sstream>

namespace vtkm
{
namespace cont
{

// Forward declaration.
template <typename CellSetList>
class DynamicCellSetBase;
}
}

namespace vtkm
{
namespace worklet
{
namespace internal
{

template <typename Domain>
inline auto scheduling_range(const Domain& inputDomain) -> decltype(inputDomain.GetNumberOfValues())
{
  return inputDomain.GetNumberOfValues();
}

template <typename Domain>
inline auto scheduling_range(const Domain* const inputDomain)
  -> decltype(inputDomain->GetNumberOfValues())
{
  return inputDomain->GetNumberOfValues();
}

template <typename Domain, typename SchedulingRangeType>
inline auto scheduling_range(const Domain& inputDomain, SchedulingRangeType type)
  -> decltype(inputDomain.GetSchedulingRange(type))
{
  return inputDomain.GetSchedulingRange(type);
}

template <typename Domain, typename SchedulingRangeType>
inline auto scheduling_range(const Domain* const inputDomain, SchedulingRangeType type)
  -> decltype(inputDomain->GetSchedulingRange(type))
{
  return inputDomain->GetSchedulingRange(type);
}

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
inline void PrintFailureMessage(int index)
{
  std::stringstream message;
  message << "Encountered bad type for parameter " << index
          << " when calling Invoke on a dispatcher.";
  throw vtkm::cont::ErrorBadType(message.str());
}

inline void PrintNullPtrMessage(int index, int mode)
{
  std::stringstream message;
  if (mode == 0)
  {
    message << "Encountered nullptr for parameter " << index;
  }
  else
  {
    message << "Encountered nullptr for " << index << " from last parameter ";
  }
  message << " when calling Invoke on a dispatcher.";
  throw vtkm::cont::ErrorBadValue(message.str());
}

template <typename T>
inline void not_nullptr(T* ptr, int index, int mode = 0)
{
  if (!ptr)
  {
    PrintNullPtrMessage(index, mode);
  }
}
template <typename T>
inline void not_nullptr(T&&, int, int mode = 0)
{
  (void)mode;
}

template <typename T>
inline T& as_ref(T* ptr)
{
  return *ptr;
}
template <typename T>
inline T&& as_ref(T&& t)
{
  return std::forward<T>(t);
}


template <typename T, bool noError>
struct ReportTypeOnError;
template <typename T>
struct ReportTypeOnError<T, true> : std::true_type
{
};

template <int Value, bool noError>
struct ReportValueOnError;
template <int Value>
struct ReportValueOnError<Value, true> : std::true_type
{
};

template <typename T>
struct remove_pointer_and_decay : std::remove_pointer<typename std::decay<T>::type>
{
};

// Is designed as a brigand fold operation.
template <typename Type, typename State>
struct DetermineIfHasDynamicParameter
{
  using T = typename std::remove_pointer<Type>::type;
  using DynamicTag = typename vtkm::cont::internal::DynamicTransformTraits<T>::DynamicTag;
  using isDynamic =
    typename std::is_same<DynamicTag, vtkm::cont::internal::DynamicTransformTagCastAndCall>::type;

  using type = std::integral_constant<bool, (State::value || isDynamic::value)>;
};


// Is designed as a brigand fold operation.
template <typename WorkletType>
struct DetermineHasCorrectParameters
{
  template <typename Type, typename State, typename SigTypes>
  struct Functor
  {
    //T is the type of the Param at the current index
    //State if the index to use to fetch the control signature tag
    using ControlSignatureTag = typename brigand::at_c<SigTypes, State::value>;
    using TypeCheckTag = typename ControlSignatureTag::TypeCheckTag;

    using T = typename std::remove_pointer<Type>::type;
    static constexpr bool isCorrect = vtkm::cont::arg::TypeCheck<TypeCheckTag, T>::value;

    // If you get an error on the line below, that means that your code has called the
    // Invoke method on a dispatcher, and one of the arguments of the Invoke is the wrong
    // type. Each argument of Invoke corresponds to a tag in the arguments of the
    // ControlSignature of the worklet. If there is a mismatch, then you get an error here
    // (instead of where you called the dispatcher). For example, if the worklet has a
    // control signature as ControlSignature(CellSetIn, ...) and the first argument passed
    // to Invoke is an ArrayHandle, you will get an error here because you cannot use an
    // ArrayHandle in place of a CellSetIn argument. (You need to use a CellSet.) See a few
    // lines later for some diagnostics to help you trace where the error occurred.
    VTKM_READ_THE_SOURCE_CODE_FOR_HELP(isCorrect);

    // If you are getting the error described above, the following lines will give you some
    // diagnostics (in the form of compile errors). Each one will result in a compile error
    // reporting an undefined type for ReportTypeOnError (or ReportValueOnError). What we are
    // really reporting is the first template argument, which is one of the types or values that
    // should help pinpoint where the error is. The comment for static_assert provides the
    // type/value being reported. (Note that some compilers report better types than others. If
    // your compiler is giving unhelpful types like "T" or "WorkletType", you may need to try a
    // different compiler.)
    static_assert(ReportTypeOnError<T, isCorrect>::value, "Type passed to Invoke");
    static_assert(ReportTypeOnError<WorkletType, isCorrect>::value, "Worklet being invoked.");
    static_assert(ReportValueOnError<State::value, isCorrect>::value, "Index of Invoke parameter");
    static_assert(ReportTypeOnError<TypeCheckTag, isCorrect>::value, "Type check tag used");

    // This final static_assert gives a human-readable error message. Ideally, this would be
    // placed first, but some compilers will suppress further errors when a static_assert
    // fails, so you would not see the other diagnostic error messages.
    static_assert(isCorrect,
                  "The type of one of the arguments to the dispatcher's Invoke method is "
                  "incompatible with the corresponding tag in the worklet's ControlSignature.");

    using type = std::integral_constant<std::size_t, State::value + 1>;
  };
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
    using type = ControlSignatureTag;
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
    using type = ExecutionSignatureTag;
  };
};

// A look up helper used by DispatcherBaseTransportFunctor to determine
//the types independent of the device we are templated on.
template <typename ControlInterface, vtkm::IdComponent Index>
struct DispatcherBaseTransportInvokeTypes
{
  //Moved out of DispatcherBaseTransportFunctor to reduce code generation
  using ControlSignatureTag = typename ControlInterface::template ParameterType<Index>::type;
  using TransportTag = typename ControlSignatureTag::TransportTag;
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
    using T = typename remove_pointer_and_decay<ControlParameter>::type;
    using TransportType = typename vtkm::cont::arg::Transport<TransportTag, T, Device>;
    using type = typename TransportType::ExecObjectType;
  };

  // template<typename ControlParameter, vtkm::IdComponent Index>
  // VTKM_CONT typename ReturnType<ControlParameter, Index>::type operator()(
  //   ControlParameter const& invokeData,
  //   vtkm::internal::IndexTag<Index>) const
  // {
  //   using TransportTag =
  //     typename DispatcherBaseTransportInvokeTypes<ControlInterface, Index>::TransportTag;
  //   using T = typename remove_pointer_and_decay<ControlParameter>::type;
  //   vtkm::cont::arg::Transport<TransportTag, T, Device> transport;
  //   return transport(invokeData, as_ref(this->InputDomain), this->InputRange, this->OutputRange);
  // }

  template <typename ControlParameter, vtkm::IdComponent Index>
  VTKM_CONT typename ReturnType<ControlParameter, Index>::type operator()(
    ControlParameter&& invokeData,
    vtkm::internal::IndexTag<Index>) const
  {
    using TransportTag =
      typename DispatcherBaseTransportInvokeTypes<ControlInterface, Index>::TransportTag;
    using T = typename remove_pointer_and_decay<ControlParameter>::type;
    vtkm::cont::arg::Transport<TransportTag, T, Device> transport;

    not_nullptr(invokeData, Index);
    return transport(
      as_ref(invokeData), as_ref(this->InputDomain), this->InputRange, this->OutputRange);
  }



private:
  void operator=(const DispatcherBaseTransportFunctor&) = delete;
};

//forward declares
template <std::size_t LeftToProcess>
struct for_each_dynamic_arg;

template <std::size_t LeftToProcess, typename TypeCheckTag>
struct convert_arg_wrapper
{
  template <typename T, typename... Args>
  void operator()(T&& t, Args&&... args) const
  {
    using Type = typename std::decay<T>::type;
    using valid =
      std::integral_constant<bool, vtkm::cont::arg::TypeCheck<TypeCheckTag, Type>::value>;
    this->WillContinue(valid(), std::forward<T>(t), std::forward<Args>(args)...);
  }
  template <typename T, typename... Args>
  void WillContinue(std::true_type, T&& t, Args&&... args) const
  {
    for_each_dynamic_arg<LeftToProcess - 1>()(std::forward<Args>(args)..., std::forward<T>(t));
  }
  template <typename... Args>
  void WillContinue(std::false_type, Args&&...) const
  {
    vtkm::worklet::internal::detail::PrintFailureMessage(LeftToProcess);
  }
};

template <std::size_t LeftToProcess,
          typename T,
          typename ContParams,
          typename Trampoline,
          typename... Args>
inline void convert_arg(vtkm::cont::internal::DynamicTransformTagStatic,
                        T&& t,
                        const ContParams&,
                        const Trampoline& trampoline,
                        Args&&... args)
{ //This is a static array, so just push it to the back
  using popped_sig = brigand::pop_front<ContParams>;
  for_each_dynamic_arg<LeftToProcess - 1>()(
    trampoline, popped_sig(), std::forward<Args>(args)..., std::forward<T>(t));
}

template <std::size_t LeftToProcess,
          typename T,
          typename ContParams,
          typename Trampoline,
          typename... Args>
inline void convert_arg(vtkm::cont::internal::DynamicTransformTagCastAndCall,
                        T&& t,
                        const ContParams&,
                        const Trampoline& trampoline,
                        Args&&... args)
{ //This is something dynamic so cast and call
  using tag_check = typename brigand::at_c<ContParams, 0>::TypeCheckTag;
  using popped_sig = brigand::pop_front<ContParams>;

  not_nullptr(t, LeftToProcess, 1);
  vtkm::cont::CastAndCall(as_ref(t),
                          convert_arg_wrapper<LeftToProcess, tag_check>(),
                          trampoline,
                          popped_sig(),
                          std::forward<Args>(args)...);
}

template <std::size_t LeftToProcess>
struct for_each_dynamic_arg
{
  template <typename Trampoline, typename ContParams, typename T, typename... Args>
  void operator()(const Trampoline& trampoline, ContParams&& sig, T&& t, Args&&... args) const
  {
    //Determine that state of T when it is either a `cons&` or a `* const&`
    using Type = typename std::remove_pointer<typename std::decay<T>::type>::type;
    using tag = typename vtkm::cont::internal::DynamicTransformTraits<Type>::DynamicTag;
    //convert the first item to a known type
    convert_arg<LeftToProcess>(
      tag(), std::forward<T>(t), sig, trampoline, std::forward<Args>(args)...);
  }
};

template <>
struct for_each_dynamic_arg<0>
{
  template <typename Trampoline, typename ContParams, typename... Args>
  void operator()(const Trampoline& trampoline, ContParams&&, Args&&... args) const
  {
    trampoline.StartInvokeDynamic(std::false_type(), std::forward<Args>(args)...);
  }
};

template <typename Trampoline, typename ContParams, typename... Args>
inline void deduce(Trampoline&& trampoline, ContParams&& sig, Args&&... args)
{
  for_each_dynamic_arg<sizeof...(Args)>()(std::forward<Trampoline>(trampoline), sig, args...);
}


#if defined(VTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4068) //unknown pragma
#endif
#if defined(__NVCC__) && defined(__CUDACC_VER_MAJOR__)
// Disable warning "calling a __host__ function from a __host__ __device__"
// In some cases nv_exec_check_disable doesn't work and therefore you need
// to use the following suppressions
#pragma push

#if (__CUDACC_VER_MAJOR__ < 8)
#pragma diag_suppress 2670
#pragma diag_suppress 2668
#endif

#if (__CUDACC_VER_MAJOR__ >= 8)
#pragma diag_suppress 2735
#pragma diag_suppress 2737
#pragma diag_suppress 2739
#endif

#if (__CUDACC_VER_MAJOR__ >= 9)
#pragma diag_suppress 2828
#pragma diag_suppress 2864
#pragma diag_suppress 2867
#pragma diag_suppress 2885
#endif

#endif
//This is a separate function as the pragma guards can cause nvcc
//to have an internal compiler error (codegen #3028)
template <typename... Args>
inline auto make_funcIFace(Args&&... args) -> decltype(
  vtkm::internal::make_FunctionInterface<void, typename std::decay<Args>::type...>(args...))
{
  return vtkm::internal::make_FunctionInterface<void, typename std::decay<Args>::type...>(args...);
}
#if defined(__NVCC__) && defined(__CUDACC_VER_MAJOR__)
#pragma pop
#endif
#if defined(VTKM_MSVC)
#pragma warning(pop)
#endif


} // namespace detail

/// Base class for all dispatcher classes. Every worklet type should have its
/// own dispatcher.
///
template <typename DerivedClass, typename WorkletType, typename BaseWorkletType>
class DispatcherBase
{
private:
  using MyType = DispatcherBase<DerivedClass, WorkletType, BaseWorkletType>;

  friend struct detail::for_each_dynamic_arg<0>;

protected:
  using ControlInterface =
    vtkm::internal::FunctionInterface<typename WorkletType::ControlSignature>;
  using ExecutionInterface =
    vtkm::internal::FunctionInterface<typename WorkletType::ExecutionSignature>;

  static constexpr vtkm::IdComponent NUM_INVOKE_PARAMS = ControlInterface::ARITY;

private:
  // We don't really need these types, but declaring them checks the arguments
  // of the control and execution signatures.
  using ControlSignatureCheck = typename ControlInterface::template StaticTransformType<
    detail::DispatcherBaseControlSignatureTagCheck>::type;
  using ExecutionSignatureCheck = typename ExecutionInterface::template StaticTransformType<
    detail::DispatcherBaseExecutionSignatureTagCheck>::type;

  template <typename... Args>
  VTKM_CONT void StartInvoke(Args&&... args) const
  {
    using ParameterInterface =
      vtkm::internal::FunctionInterface<void(typename std::decay<Args>::type...)>;

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

    this->StartInvokeDynamic(HasDynamicTypes(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  VTKM_CONT void StartInvokeDynamic(std::true_type, Args&&... args) const
  {
    // As we do the dynamic transform, we are also going to check the static
    // type against the TypeCheckTag in the ControlSignature tags. To do this,
    // the check needs access to both the parameter (in the parameters
    // argument) and the ControlSignature tags (in the ControlInterface type).
    using ContParamsInfo =
      vtkm::internal::detail::FunctionSigInfo<typename WorkletType::ControlSignature>;
    typename ContParamsInfo::Parameters parameters;
    detail::deduce(*this, parameters, std::forward<Args>(args)...);
  }

  template <typename... Args>
  VTKM_CONT void StartInvokeDynamic(std::false_type, Args&&... args) const
  {
    using ParameterInterface =
      vtkm::internal::FunctionInterface<void(typename std::decay<Args>::type...)>;

    //Nothing requires a conversion from dynamic to static types, so
    //next we need to verify that each argument's type is correct. If not
    //we need to throw a nice compile time error
    using ParamTypes = typename ParameterInterface::ParameterSig;
    using ContSigTypes = typename vtkm::internal::detail::FunctionSigInfo<
      typename WorkletType::ControlSignature>::Parameters;

    //isAllValid will throw a compile error if everything doesn't match
    using isAllValid = brigand::fold<
      ParamTypes,
      std::integral_constant<std::size_t, 0>,
      typename detail::DetermineHasCorrectParameters<WorkletType>::
        template Functor<brigand::_element, brigand::_state, brigand::pin<ContSigTypes>>>;

    //this warning exists so that we don't get a warning from not using isAllValid
    using expectedLen = std::integral_constant<std::size_t, sizeof...(Args)>;
    static_assert(isAllValid::value == expectedLen::value,
                  "All arguments failed the TypeCheck pass");

    //This is a separate function as the pragma guards can cause nvcc
    //to have an internal compiler error (codegen #3028)
    auto fi = detail::make_funcIFace(std::forward<Args>(args)...);

    auto ivc = vtkm::internal::Invocation<ParameterInterface,
                                          ControlInterface,
                                          ExecutionInterface,
                                          WorkletType::InputDomain::INDEX,
                                          vtkm::internal::NullType,
                                          vtkm::internal::NullType>(
      fi, vtkm::internal::NullType{}, vtkm::internal::NullType{});
    static_cast<const DerivedClass*>(this)->DoInvoke(ivc);
  }



public:
  using ScatterType = typename WorkletType::ScatterType;

  template <typename... Args>
  VTKM_CONT void Invoke(Args&&... args) const
  {
    this->StartInvoke(std::forward<Args>(args)...);
  }

protected:
  VTKM_CONT
  DispatcherBase(const WorkletType& worklet, const ScatterType& scatter)
    : Worklet(worklet)
    , Scatter(scatter)
  {
  }

  template <typename Invocation, typename DeviceAdapter>
  VTKM_CONT void BasicInvoke(Invocation& invocation,
                             vtkm::Id numInstances,
                             DeviceAdapter device) const
  {
    this->InvokeTransportParameters(
      invocation, numInstances, this->Scatter.GetOutputRange(numInstances), device);
  }

  template <typename Invocation, typename DeviceAdapter>
  VTKM_CONT void BasicInvoke(Invocation& invocation,
                             vtkm::Id2 dimensions,
                             DeviceAdapter device) const
  {
    this->BasicInvoke(invocation, vtkm::Id3(dimensions[0], dimensions[1], 1), device);
  }

  template <typename Invocation, typename DeviceAdapter>
  VTKM_CONT void BasicInvoke(Invocation& invocation,
                             vtkm::Id3 dimensions,
                             DeviceAdapter device) const
  {
    this->InvokeTransportParameters(
      invocation, dimensions, this->Scatter.GetOutputRange(dimensions), device);
  }

  WorkletType Worklet;
  ScatterType Scatter;

private:
  // Dispatchers cannot be copied
  DispatcherBase(const MyType&) = delete;
  void operator=(const MyType&) = delete;

  template <typename Invocation,
            typename InputRangeType,
            typename OutputRangeType,
            typename DeviceAdapter>
  VTKM_CONT void InvokeTransportParameters(Invocation& invocation,
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
    using ParameterInterfaceType = typename Invocation::ParameterInterface;
    ParameterInterfaceType& parameters = invocation.Parameters;

    using TransportFunctorType =
      detail::DispatcherBaseTransportFunctor<typename Invocation::ControlInterface,
                                             typename Invocation::InputDomainType,
                                             DeviceAdapter>;
    using ExecObjectParameters =
      typename ParameterInterfaceType::template StaticTransformType<TransportFunctorType>::type;

    ExecObjectParameters execObjectParameters = parameters.StaticTransformCont(
      TransportFunctorType(invocation.GetInputDomain(), inputRange, outputRange));

    // Get the arrays used for scattering input to output.
    typename WorkletType::ScatterType::OutputToInputMapType outputToInputMap =
      this->Scatter.GetOutputToInputMap(inputRange);
    typename WorkletType::ScatterType::VisitArrayType visitArray =
      this->Scatter.GetVisitArray(inputRange);

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
