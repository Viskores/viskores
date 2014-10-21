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

#include <vtkm/cont/arg/Transport.h>

#include <vtkm/cont/internal/DynamicTransform.h>

#include <vtkm/exec/internal/WorkletInvokeFunctor.h>

#include <boost/mpl/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>

namespace vtkm {
namespace worklet {
namespace internal {

namespace detail {

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

  template<typename Signature>
  VTKM_CONT_EXPORT
  void StartInvoke(
      const vtkm::internal::FunctionInterface<Signature> &parameters) const
  {
    typedef vtkm::internal::FunctionInterface<Signature> ParameterInterface;
    BOOST_STATIC_ASSERT_MSG(ParameterInterface::ARITY == NUM_INVOKE_PARAMS,
                            "Dispatcher Invoke called with wrong number of arguments.");

    BOOST_MPL_ASSERT(( boost::is_base_of<BaseWorkletType,WorkletType> ));

    parameters.DynamicTransformCont(
          vtkm::cont::internal::DynamicTransform(),
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
    // will make a Function interface with each parameter being a Pair
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
