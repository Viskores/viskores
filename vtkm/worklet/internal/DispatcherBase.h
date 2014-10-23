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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_internal_DispatcherBase_h
#define vtk_m_worklet_internal_DispatcherBase_h

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ErrorControlBadType.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/Transport.h>
#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/cont/internal/DynamicTransform.h>

#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>

#include <vtkm/exec/internal/WorkletInvokeFunctor.h>

#include <boost/mpl/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/utility/enable_if.hpp>

#include <sstream>

namespace vtkm {
namespace worklet {
namespace internal {

namespace detail {

// Checks that an argument in a ControlSignature is a valid control signature
// tag. Causes a compile error otherwise.
struct DispatcherBaseControlSignatureTagCheck
{
  template<typename ControlSignatureTag>
  struct ReturnType {
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
  template<typename ExecutionSignatureTag>
  struct ReturnType {
    // If you get a compile error here, it means there is something that is not
    // a valid execution signature tag in a worklet's ExecutionSignature.
    VTKM_IS_EXECUTION_SIGNATURE_TAG(ExecutionSignatureTag);
    typedef ExecutionSignatureTag type;
  };
};

// Used in the dynamic cast to check to make sure that the type passed into
// the Invoke method matches the type accepted by the ControlSignature.
template<typename ContinueFunctor, typename TypeCheckTag>
struct DispatcherBaseTypeCheckFunctor
{
  const ContinueFunctor &Continue;
  vtkm::IdComponent ParameterIndex;

  VTKM_CONT_EXPORT
  DispatcherBaseTypeCheckFunctor(const ContinueFunctor &continueFunc,
                                 vtkm::IdComponent parameterIndex)
    : Continue(continueFunc), ParameterIndex(parameterIndex) {  }

  template<typename T>
  VTKM_CONT_EXPORT
  typename boost::enable_if_c<vtkm::cont::arg::TypeCheck<TypeCheckTag,T>::value>::type
  operator()(const T &x) const
  {
    this->Continue(x);
  }

  // This code is actually taking an error found at compile-time and not
  // reporting it until run-time. This seems strange at first, but this
  // behavior is actually important. With dynamic arrays and similar dynamic
  // classes, there may be types that are technically possible (such as using a
  // vector where a scalar is expected) but in reality never happen. Thus, for
  // these unsported combinations we just silently halt the compiler from
  // attempting to create code for these errant conditions and throw a run-time
  // error if one every tries to create one.
  template<typename T>
  VTKM_CONT_EXPORT
  typename boost::disable_if_c<vtkm::cont::arg::TypeCheck<TypeCheckTag,T>::value>::type
  operator()(const T &) const
  {
    std::stringstream message;
    message << "Encountered bad type for parameter "
            << this->ParameterIndex
            << " when calling Invoke on a dispatcher.";
    throw vtkm::cont::ErrorControlBadType(message.str());
  }
};

// Uses vtkm::cont::internal::DynamicTransform and the DynamicTransformCont
// method of FunctionInterface to convert all DynamicArrayHandles and any
// other arguments declaring themselves as dynamic to static versions.
struct DispatcherBaseDynamicTransform
{
  vtkm::cont::internal::DynamicTransform BasicDynamicTransform;
  vtkm::IdComponent *ParameterCounter;

  VTKM_CONT_EXPORT
  DispatcherBaseDynamicTransform(vtkm::IdComponent *parameterCounter)
    : ParameterCounter(parameterCounter)
  {
    *this->ParameterCounter = 0;
  }

  template<typename ControlSignatureTag,
           typename InputType,
           typename ContinueFunctor>
  VTKM_CONT_EXPORT
  void operator()(const vtkm::Pair<ControlSignatureTag, InputType> &input,
                  const ContinueFunctor &continueFunc) const
  {
    (*this->ParameterCounter)++;

    typedef DispatcherBaseTypeCheckFunctor<
        ContinueFunctor, typename ControlSignatureTag::TypeCheckTag>
        TypeCheckFunctor;
    this->BasicDynamicTransform(input.second,
                                TypeCheckFunctor(continueFunc,
                                                 *this->ParameterCounter));
  }
};

// A functor called at the end of the dynamic transform to call the next
// step in the dynamic transform.
template<typename DispatcherBaseType>
struct DispatcherBaseDynamicTransformHelper
{
  const DispatcherBaseType *Dispatcher;

  VTKM_CONT_EXPORT
  DispatcherBaseDynamicTransformHelper(const DispatcherBaseType *dispatcher)
    : Dispatcher(dispatcher) {  }

  template<typename FunctionInterface>
  VTKM_CONT_EXPORT
  void operator()(const FunctionInterface &parameters) const {
    this->Dispatcher->DynamicTransformInvoke(parameters);
  }
};

// A functor used in a StaticCast of a FunctionInterface to transport arguments
// from the control environment to the execution environment.
template<typename Device>
struct DispatcherBaseTransportFunctor
{
  vtkm::Id NumInstances;

  DispatcherBaseTransportFunctor(vtkm::Id numInstances)
    : NumInstances(numInstances) {  }

  template<typename T>
  struct InvokeTypes {
    typedef typename T::FirstType::TransportTag TransportTag;
    typedef typename T::SecondType ControlParameter;
    typedef vtkm::cont::arg::Transport<TransportTag,ControlParameter,Device>
        TransportType;
  };

  template<typename T>
  struct ReturnType {
    typedef typename InvokeTypes<T>::TransportType::ExecObjectType type;
  };

  template<typename T>
  VTKM_CONT_EXPORT
  typename ReturnType<T>::type
  operator()(const T &invokeData) const {
    typename InvokeTypes<T>::TransportType transport;
    return transport(invokeData.second, this->NumInstances);
  }
};

} // namespace detail

/// Base class for all dispatcher classes. Every worklet type should have its
/// own dispatcher.
///
template<typename DerivedClass,
         typename WorkletType,
         typename BaseWorkletType,
         typename Device>
class DispatcherBase
{
private:
  typedef DispatcherBase<DerivedClass,WorkletType,BaseWorkletType,Device> MyType;

  friend struct detail::DispatcherBaseDynamicTransformHelper<MyType>;

protected:
  typedef vtkm::internal::FunctionInterface<
      typename WorkletType::ControlSignature> ControlInterface;
  typedef vtkm::internal::FunctionInterface<
      typename WorkletType::ExecutionSignature> ExecutionInterface;

  static const vtkm::IdComponent NUM_INVOKE_PARAMS = ControlInterface::ARITY;

private:
  // We don't really need these types, but declaring them checks the arguments
  // of the control and execution signatures.
  typedef typename ControlInterface::
      template StaticTransformType<
        detail::DispatcherBaseControlSignatureTagCheck>::type
      ControlSignatureCheck;
  typedef typename ExecutionInterface::
      template StaticTransformType<
        detail::DispatcherBaseExecutionSignatureTagCheck>::type
      ExecutionSignatureCheck;

  template<typename Signature>
  VTKM_CONT_EXPORT
  void StartInvoke(
      const vtkm::internal::FunctionInterface<Signature> &parameters) const
  {
    typedef vtkm::internal::FunctionInterface<Signature> ParameterInterface;
    BOOST_STATIC_ASSERT_MSG(ParameterInterface::ARITY == NUM_INVOKE_PARAMS,
                            "Dispatcher Invoke called with wrong number of arguments.");

    BOOST_MPL_ASSERT(( boost::is_base_of<BaseWorkletType,WorkletType> ));

    // As we do the dynamic transform, we are also going to check the static
    // type against the TypeCheckTag in the ControlSignature tags. To do this,
    // the check needs access to both the parameter (in the parameters
    // argument) and the ControlSignature tags (in the ControlInterface type).
    // To make this possible, we use the zip mechanism of FunctionInterface to
    // combine these two separate function interfaces into a single
    // FunctionInterface with each parameter being a Pair containing both
    // the ControlSignature tag and the control object itself.
    typedef typename vtkm::internal::FunctionInterfaceZipType<
        ControlInterface, ParameterInterface>::type ZippedInterface;
    ZippedInterface zippedInterface =
        vtkm::internal::make_FunctionInterfaceZip(ControlInterface(),
                                                  parameters);

    vtkm::IdComponent parameterIndexCounter;

    zippedInterface.DynamicTransformCont(
          detail::DispatcherBaseDynamicTransform(&parameterIndexCounter),
          detail::DispatcherBaseDynamicTransformHelper<MyType>(this));
  }

  template<typename Signature>
  VTKM_CONT_EXPORT
  void DynamicTransformInvoke(
      const vtkm::internal::FunctionInterface<Signature> &parameters) const
  {
    // TODO: Check parameters
    static const vtkm::IdComponent INPUT_DOMAIN_INDEX =
        WorkletType::InputDomain::INDEX;
    reinterpret_cast<const DerivedClass *>(this)->DoInvoke(
          vtkm::internal::make_Invocation<INPUT_DOMAIN_INDEX>(
            parameters, ControlInterface(), ExecutionInterface()));
  }

public:
  // Implementation of the Invoke method is in this generated file.
#include <vtkm/worklet/internal/DispatcherBaseDetailInvoke.h>

protected:
  VTKM_CONT_EXPORT
  DispatcherBase(const WorkletType &worklet) : Worklet(worklet) {  }

  template<typename Invocation>
  VTKM_CONT_EXPORT
  void BasicInvoke(const Invocation &invocation, vtkm::Id numInstances) const
  {
    this->InvokeTransportParameters(invocation, numInstances);
  }

  WorkletType Worklet;

private:
  // These are not implemented. Dispatchers cannot be copied.
  DispatcherBase(const MyType &);
  void operator=(const MyType &);

  template<typename Invocation>
  VTKM_CONT_EXPORT
  void InvokeTransportParameters(const Invocation &invocation,
                                 vtkm::Id numInstances) const
  {
    // The first step in invoking a worklet is transport the arguments to the
    // execution environment. The invocation object passed to this function
    // contains the parameters passed to Invoke in the control environment. We
    // will use the template magic in the FunctionInterface class to invoke the
    // appropriate Transport class on each parameter to get a list of execution
    // objects (corresponding to the arguments of the Invoke in the control
    // environment) in a FunctionInterface.

    // The Transport relies on both the ControlSignature tag and the control
    // object itself. To make it easier to work with each parameter, use the
    // zip mechanism of FunctionInterface to combine the separate function
    // interfaces of the ControlSignature and the parameters into one. This
    // will make a FunctionInterface with each parameter being a Pair
    // containing both the ControlSignature tag and the control object itself.
    typedef typename vtkm::internal::FunctionInterfaceZipType<
        typename Invocation::ControlInterface,
        typename Invocation::ParameterInterface>::type ZippedInterface;
    ZippedInterface zippedInterface =
        vtkm::internal::make_FunctionInterfaceZip(
          typename Invocation::ControlInterface(), invocation.Parameters);

    // Use the StaticTransform mechanism to run the
    // DispatcherBaseTransportFunctor on each parameter of the zipped
    // interface. This functor will in turn run the appropriate Transform on
    // the parameter and return the associated execution object. The end result
    // of the transform is a FunctionInterface containing execution objects
    // corresponding to each Invoke argument.
    typedef detail::DispatcherBaseTransportFunctor<Device> TransportFunctor;
    typedef typename ZippedInterface::template StaticTransformType<
        TransportFunctor>::type ExecObjectParameters;
    ExecObjectParameters execObjectParameters =
        zippedInterface.StaticTransformCont(TransportFunctor(numInstances));

    // Replace the parameters in the invocation with the execution object and
    // pass to next step of Invoke.
    this->InvokeSchedule(invocation.ChangeParameters(execObjectParameters),
                         numInstances);
  }

  template<typename Invocation>
  VTKM_CONT_EXPORT
  void InvokeSchedule(const Invocation &invocation, vtkm::Id numInstances) const
  {
    // The WorkletInvokeFunctor class handles the magic of fetching values
    // for each instance and calling the worklet's function. So just create
    // a WorkletInvokeFunctor and schedule it with the device adapter.
    typedef vtkm::exec::internal::WorkletInvokeFunctor<WorkletType,Invocation>
        WorkletInvokeFunctorType;
    WorkletInvokeFunctorType workletFunctor =
        WorkletInvokeFunctorType(this->Worklet, invocation);

    typedef vtkm::cont::DeviceAdapterAlgorithm<Device> Algorithm;
    Algorithm::Schedule(workletFunctor, numInstances);
  }
};

}
}
} // namespace vtkm::worklet::internal

#endif //vtk_m_worklet_internal_DispatcherBase_h
