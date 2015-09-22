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
#ifndef vtk_m_worklet_internal_DispatcherBase_h
#define vtk_m_worklet_internal_DispatcherBase_h

#include <vtkm/StaticAssert.h>

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

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/mpl/at.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/zip_view.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/utility/enable_if.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <sstream>

namespace vtkm {
namespace worklet {
namespace internal {

namespace detail {

// This code is actually taking an error found at compile-time and not
// reporting it until run-time. This seems strange at first, but this
// behavior is actually important. With dynamic arrays and similar dynamic
// classes, there may be types that are technically possible (such as using a
// vector where a scalar is expected) but in reality never happen. Thus, for
// these unsupported combinations we just silently halt the compiler from
// attempting to create code for these errant conditions and throw a run-time
// error if one every tries to create one.
inline void PrintFailureMessage(int, boost::true_type) {}
inline void PrintFailureMessage(int index, boost::false_type)
{
  std::stringstream message;
  message << "Encountered bad type for parameter "
          << index
          << " when calling Invoke on a dispatcher.";
  throw vtkm::cont::ErrorControlBadType(message.str());
}

// Is designed as a boost mpl binary metafunction.
struct DetermineIfHasDynamicParameter
{
  template<typename T, typename U>
  struct apply
  {
    typedef typename vtkm::cont::internal::DynamicTransformTraits<U>::DynamicTag DynamicTag;
    typedef typename boost::is_same<
            DynamicTag,
            vtkm::cont::internal::DynamicTransformTagCastAndCall>::type UType;

    typedef typename boost::mpl::or_<T,UType>::type type;
  };
};


template<typename ValueType, typename TagList>
void NiceInCorrectParameterErrorMessage()
{
 VTKM_STATIC_ASSERT_MSG(ValueType() == TagList(),
                        "Unable to match 'ValueType' to the signature tag 'ControlSignatureTag'" );
}

template<typename T>
void ShowInCorrectParameter(boost::mpl::true_, T) {}

template<typename T>
void ShowInCorrectParameter(boost::mpl::false_, T)
{
  typedef typename boost::mpl::deref<T>::type ZipType;
  typedef typename boost::mpl::at_c<ZipType,0>::type ValueType;
  typedef typename boost::mpl::at_c<ZipType,1>::type ControlSignatureTag;
  NiceInCorrectParameterErrorMessage<ValueType,ControlSignatureTag>();
};

// Is designed as a boost mpl unary metafunction.
struct DetermineHasInCorrectParameters
{
  //When we find parameters that don't match, we set our 'type' to true_
  //otherwise we are false_
  template<typename T>
  struct apply
  {
    typedef typename boost::mpl::at_c<T,0>::type ValueType;
    typedef typename boost::mpl::at_c<T,1>::type ControlSignatureTag;

    typedef typename ControlSignatureTag::TypeCheckTag TypeCheckTag;

    typedef boost::mpl::bool_<
       vtkm::cont::arg::TypeCheck<TypeCheckTag,ValueType>::value> CanContinueTagType;

    //We need to not the result of CanContinueTagType, because we want to return
    //true when we have the first parameter that DOES NOT match the control
    //signature requirements
    typedef typename boost::mpl::not_< typename CanContinueTagType::type
                        >::type type;
  };
};

// Checks that an argument in a ControlSignature is a valid control signature
// tag. Causes a compile error otherwise.
struct DispatcherBaseControlSignatureTagCheck
{
  template<typename ControlSignatureTag, vtkm::IdComponent Index>
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
  template<typename ExecutionSignatureTag, vtkm::IdComponent Index>
  struct ReturnType {
    // If you get a compile error here, it means there is something that is not
    // a valid execution signature tag in a worklet's ExecutionSignature.
    VTKM_IS_EXECUTION_SIGNATURE_TAG(ExecutionSignatureTag);
    typedef ExecutionSignatureTag type;
  };
};

// Used in the dynamic cast to check to make sure that the type passed into
// the Invoke method matches the type accepted by the ControlSignature.
template<typename ContinueFunctor,
         typename TypeCheckTag,
         vtkm::IdComponent Index>
struct DispatcherBaseTypeCheckFunctor
{
  const ContinueFunctor &Continue;

  VTKM_CONT_EXPORT
  DispatcherBaseTypeCheckFunctor(const ContinueFunctor &continueFunc)
    : Continue(continueFunc) {  }

  template<typename T>
  VTKM_CONT_EXPORT
  void operator()(const T &x) const
  {
    typedef boost::integral_constant<bool,
            vtkm::cont::arg::TypeCheck<TypeCheckTag,T>::value> CanContinueTagType;

    vtkm::worklet::internal::detail::PrintFailureMessage(Index,CanContinueTagType());
    this->WillContinue(x, CanContinueTagType());
  }

private:
  template<typename T>
  VTKM_CONT_EXPORT
  void WillContinue(const T &x, boost::true_type) const
  {
    this->Continue(x);
  }

  template<typename T>
  VTKM_CONT_EXPORT
  void WillContinue(const T&, boost::false_type) const
  { }
};

// Uses vtkm::cont::internal::DynamicTransform and the DynamicTransformCont
// method of FunctionInterface to convert all DynamicArrayHandles and any
// other arguments declaring themselves as dynamic to static versions.
template<typename ControlInterface>
struct DispatcherBaseDynamicTransform
{
  template<typename InputType,
           typename ContinueFunctor,
           vtkm::IdComponent Index>
  VTKM_CONT_EXPORT
  void operator()(const InputType &input,
                  const ContinueFunctor &continueFunc,
                  vtkm::internal::IndexTag<Index> indexTag) const
  {
    typedef typename ControlInterface::template ParameterType<Index>::type
        ControlSignatureTag;

    typedef DispatcherBaseTypeCheckFunctor<
        ContinueFunctor, typename ControlSignatureTag::TypeCheckTag, Index>
        TypeCheckFunctor;

    vtkm::cont::internal::DynamicTransform basicDynamicTransform;

    basicDynamicTransform(input, TypeCheckFunctor(continueFunc), indexTag);
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
    this->Dispatcher->DynamicTransformInvoke(parameters, boost::mpl::true_() );
  }
};

// A look up helper used by DispatcherBaseTransportFunctor to determine
//the types independent of the device we are templated on.
template<typename ControlInterface, vtkm::IdComponent Index>
struct DispatcherBaseTransportInvokeTypes
{
  //Moved out of DispatcherBaseTransportFunctor to reduce code generation
  typedef typename ControlInterface::template ParameterType<Index>::type
        ControlSignatureTag;
  typedef typename ControlSignatureTag::TransportTag TransportTag;
};

// A functor used in a StaticCast of a FunctionInterface to transport arguments
// from the control environment to the execution environment.
template<typename ControlInterface, typename Device>
struct DispatcherBaseTransportFunctor
{
  vtkm::Id NumInstances;

  VTKM_CONT_EXPORT
  DispatcherBaseTransportFunctor(vtkm::Id numInstances)
    : NumInstances(numInstances) {  }

  // TODO: We need to think harder about how scheduling on 3D arrays works.
  // Chances are we need to allow the transport for each argument to manage
  // 3D indices (for example, allocate a 3D array instead of a 1D array).
  // But for now, just treat all transports as 1D arrays.
  VTKM_CONT_EXPORT
  DispatcherBaseTransportFunctor(vtkm::Id3 dimensions)
    : NumInstances(dimensions[0]*dimensions[1]*dimensions[2]) {  }


  template<typename ControlParameter, vtkm::IdComponent Index>
  struct ReturnType {
    typedef typename DispatcherBaseTransportInvokeTypes<ControlInterface, Index>::TransportTag TransportTag;
    typedef typename vtkm::cont::arg::Transport<TransportTag,ControlParameter,Device> TransportType;
    typedef typename TransportType::ExecObjectType type;
  };

  template<typename ControlParameter, vtkm::IdComponent Index>
  VTKM_CONT_EXPORT
  typename ReturnType<ControlParameter, Index>::type
  operator()(const ControlParameter &invokeData,
             vtkm::internal::IndexTag<Index>) const
  {
    typedef typename DispatcherBaseTransportInvokeTypes<ControlInterface, Index>::TransportTag TransportTag;
    vtkm::cont::arg::Transport<TransportTag,ControlParameter,Device> transport;
    return transport(invokeData, this->NumInstances);
  }
};

} // namespace detail

/// Base class for all dispatcher classes. Every worklet type should have its
/// own dispatcher.
///
template<typename DerivedClass,
         typename WorkletType,
         typename BaseWorkletType>
class DispatcherBase
{
private:
  typedef DispatcherBase<DerivedClass,WorkletType,BaseWorkletType> MyType;

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
    VTKM_STATIC_ASSERT_MSG(ParameterInterface::ARITY == NUM_INVOKE_PARAMS,
                           "Dispatcher Invoke called with wrong number of arguments.");

    BOOST_MPL_ASSERT(( boost::is_base_of<BaseWorkletType,WorkletType> ));

    //We need to determine if we have the need to do any dynamic
    //transforms. This is fairly simple of a query. We just need to check
    //everything in the FunctionInterface and see if any of them have the
    //proper dynamic trait. Doing this, allows us to generate zero dynamic
    //check & convert code when we already know all the types. This results
    //in smaller executables and libraries.
    typedef boost::function_types::parameter_types<Signature> MPLSignatureForm;
    typedef typename boost::mpl::fold<
                                MPLSignatureForm,
                                boost::mpl::false_,
                                detail::DetermineIfHasDynamicParameter>::type HasDynamicTypes;

    this->StartInvokeDynamic(parameters, HasDynamicTypes() );
  }


  template<typename Signature>
  VTKM_CONT_EXPORT
  void StartInvokeDynamic(
      const vtkm::internal::FunctionInterface<Signature> &parameters,
      boost::mpl::true_) const
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
    parameters.DynamicTransformCont(
          detail::DispatcherBaseDynamicTransform<ControlInterface>(),
          detail::DispatcherBaseDynamicTransformHelper<MyType>(this));
  }

  template<typename Signature>
  VTKM_CONT_EXPORT
  void StartInvokeDynamic(
      const vtkm::internal::FunctionInterface<Signature> &parameters,
      boost::mpl::false_) const
  {
    //Nothing requires a conversion from dynamic to static types, so
    //next we need to verify that each argument's type is correct. If not
    //we need to throw a nice compile time error
    typedef boost::function_types::parameter_types<Signature> MPLSignatureForm;
    typedef typename boost::function_types::parameter_types<
                          typename WorkletType::ControlSignature > WorkletContSignature;

    typedef boost::mpl::vector< MPLSignatureForm, WorkletContSignature > ZippedSignatures;
    typedef boost::mpl::zip_view<ZippedSignatures> ZippedView;

    typedef typename boost::mpl::find_if<
                                ZippedView,
                                detail::DetermineHasInCorrectParameters>::type LocationOfIncorrectParameter;

    typedef typename boost::is_same< LocationOfIncorrectParameter,
                                     typename boost::mpl::end< ZippedView>::type >::type HasOnlyCorrectTypes;

    //When HasOnlyCorrectTypes is false we produce an error
    //message which should state what the parameter type and tag type is
    //that failed to match.
    detail::ShowInCorrectParameter(HasOnlyCorrectTypes(),
                                   LocationOfIncorrectParameter());

    this->DynamicTransformInvoke(parameters, HasOnlyCorrectTypes());
  }

  template<typename Signature>
  VTKM_CONT_EXPORT
  void DynamicTransformInvoke(
      const vtkm::internal::FunctionInterface<Signature> &parameters,
      boost::mpl::true_ ) const
  {
    // TODO: Check parameters
    static const vtkm::IdComponent INPUT_DOMAIN_INDEX =
        WorkletType::InputDomain::INDEX;
    reinterpret_cast<const DerivedClass *>(this)->DoInvoke(
          vtkm::internal::make_Invocation<INPUT_DOMAIN_INDEX>(
            parameters, ControlInterface(), ExecutionInterface()));
  }

  template<typename Signature>
  VTKM_CONT_EXPORT
  void DynamicTransformInvoke(
      const vtkm::internal::FunctionInterface<Signature> &,
      boost::mpl::false_ ) const
  {
  }

public:
  // Implementation of the Invoke method is in this generated file.
#include <vtkm/worklet/internal/DispatcherBaseDetailInvoke.h>

protected:
  VTKM_CONT_EXPORT
  DispatcherBase(const WorkletType &worklet) : Worklet(worklet) {  }

  template<typename Invocation, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  void BasicInvoke(const Invocation &invocation,
                   vtkm::Id numInstances,
                   DeviceAdapter tag) const
  {
    this->InvokeTransportParameters(invocation, numInstances, tag);
  }

  template<typename Invocation, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  void BasicInvoke(const Invocation &invocation,
                   vtkm::Id2 dimensions,
                   DeviceAdapter tag) const
  {
    vtkm::Id3 dim3d(dimensions[0], dimensions[1], 1);
    this->InvokeTransportParameters(invocation, dim3d, tag);
  }


  template<typename Invocation, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  void BasicInvoke(const Invocation &invocation,
                   vtkm::Id3 dimensions,
                   DeviceAdapter tag) const
  {
    this->InvokeTransportParameters(invocation, dimensions, tag);
  }

  WorkletType Worklet;

private:
  // These are not implemented. Dispatchers cannot be copied.
  DispatcherBase(const MyType &);
  void operator=(const MyType &);

  template<typename Invocation, typename RangeType, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  void InvokeTransportParameters(const Invocation &invocation,
                                 RangeType range,
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
    const ParameterInterfaceType &parameters = invocation.Parameters;

    typedef detail::DispatcherBaseTransportFunctor<
        typename Invocation::ControlInterface, DeviceAdapter> TransportFunctorType;
    typedef typename ParameterInterfaceType::template StaticTransformType<
        TransportFunctorType>::type ExecObjectParameters;

    ExecObjectParameters execObjectParameters =
        parameters.StaticTransformCont(TransportFunctorType(range));

    // Get the arrays used for scattering input to output.
    typename WorkletType::ScatterType::OutputToInputMapType outputToInputMap =
        this->Worklet.GetScatter().GetOutputToInputMap(range);
    typename WorkletType::ScatterType::VisitArrayType visitArray =
        this->Worklet.GetScatter().GetVisitArray(range);

    // Replace the parameters in the invocation with the execution object and
    // pass to next step of Invoke. Also add the scatter information.
    this->InvokeSchedule(
          invocation
          .ChangeParameters(execObjectParameters)
          .ChangeOutputToInputMap(outputToInputMap.PrepareForInput(device))
          .ChangeVisitArray(visitArray.PrepareForInput(device)),
          range,
          device);
  }

  template<typename Invocation, typename RangeType, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  void InvokeSchedule(const Invocation &invocation,
                      RangeType range,
                      DeviceAdapter) const
  {
    // The WorkletInvokeFunctor class handles the magic of fetching values
    // for each instance and calling the worklet's function. So just create
    // a WorkletInvokeFunctor and schedule it with the device adapter.
    typedef vtkm::exec::internal::WorkletInvokeFunctor<WorkletType,Invocation>
        WorkletInvokeFunctorType;
    WorkletInvokeFunctorType workletFunctor =
        WorkletInvokeFunctorType(this->Worklet, invocation);

    typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;

    Algorithm::Schedule(workletFunctor, range);
  }
};

}
}
} // namespace vtkm::worklet::internal

#endif //vtk_m_worklet_internal_DispatcherBase_h
