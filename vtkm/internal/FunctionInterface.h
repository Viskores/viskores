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
#ifndef vtk_m_cont_internal_FunctionInterface_h
#define vtk_m_cont_internal_FunctionInterface_h

#include <vtkm/Types.h>
#include <vtkm/cont/ErrorControlBadValue.h>

#include <boost/function_types/components.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/function_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/integer/static_min_max.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/erase.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/preprocessor/inc.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted.hpp>
#include <boost/preprocessor/repetition/enum_shifted_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/utility/enable_if.hpp>

#define VTKM_MAX_FUNCTION_PARAMETERS 10

namespace vtkm {
namespace internal {

/// This struct is used internally by FunctionInterface to store the return
/// value of a function. There is a special implementation for a return type of
/// void, which stores nothing.
///
template<typename T>
struct FunctionInterfaceReturnContainer {
  T Value;
  static const bool VALID = true;
};

template<>
struct FunctionInterfaceReturnContainer<void> {
  // Nothing to store for void return.
  static const bool VALID = false;
};

namespace detail {

// If you get a compiler error stating that this class is not specialized, that
// probably means that you are using FunctionInterface with an unsupported
// number of arguments.
template<typename FunctionSignature>
struct ParameterContainer;

// The following code uses the Boost preprocessor utilities to create
// definitions of ParameterContainer for all supported number of arguments.
// The created classes are conceptually defined as follows:
//
// template<typename P0, // Return type
//          typename P1,
//          typename P2, ...>
// struct ParameterContainer<P0(P1,P2,...)> {
//   P1 Parameter1;
//   P2 Parameter2;
//   ...
// };
//
// These are defined for 0 to VTKM_MAX_FUNCTION_PARAMETERS parameters.

#define VTK_M_PARAMETER_DEFINITION(z, ParamIndex, data) \
  BOOST_PP_IF(ParamIndex, \
              BOOST_PP_CAT(P,ParamIndex) BOOST_PP_CAT(Parameter,ParamIndex);,)

#define VTK_M_PARAMETER_CONTAINER(NumParamsPlusOne) \
  template<BOOST_PP_ENUM_PARAMS(NumParamsPlusOne, typename P)> \
  struct ParameterContainer<P0(BOOST_PP_ENUM_SHIFTED_PARAMS(NumParamsPlusOne, P))> \
  { \
    BOOST_PP_REPEAT(NumParamsPlusOne, VTK_M_PARAMETER_DEFINITION,) \
  };

#define VTK_M_PARAMETER_CONTAINER_REPEAT(z, NumParams, data) \
  VTK_M_PARAMETER_CONTAINER(BOOST_PP_INC(NumParams))
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_PARAMETER_CONTAINER_REPEAT,)

#undef VTK_M_PARAMETER_CONTAINER_REPEAT
#undef VTK_M_PARAMETER_CONTAINER
#undef VTK_M_PARAMETER_DEFINITION

template<int ParameterIndex, typename FunctionSignature>
struct ParameterContainerAccess;

// The following code uses the Boost preprocessor utilities to create
// definitions of ParameterContainerAccess for all supported number of
// arguments. The created class specalizations conceptually create the
// following interface:
//
// template<int ParameterIndex, typename R(P1,P2,...)>
// struct ParameterContainerAccess
// {
//   VTKM_EXEC_CONT_EXPORT
//   static ParameterType
//   GetParameter(const ParameterContainer<R(P1,P2,...)> &parameters);
//
//   VTKM_EXEC_CONT_EXPORT
//   static void SetParameter(ParameterContainer<R(P1,P2,...)> &parameters,
//                            const ParameterType &value);
// };
//
// Here ParameterType is the P# type in the function signature for the given
// ParameterIndex. It is the same you would get for
// FunctionInterface::ParameterType.

#define VTK_M_PARAMETER_CONTAINER_ACCESS(ParameterIndex) \
  template<typename FunctionSignature> \
  struct ParameterContainerAccess<ParameterIndex, FunctionSignature> { \
    typedef typename boost::mpl::at_c< \
        boost::function_types::components<FunctionSignature>, \
        ParameterIndex>::type ParameterType; \
    VTKM_EXEC_CONT_EXPORT \
    static \
    ParameterType \
    GetParameter(const ParameterContainer<FunctionSignature> &parameters) { \
      return parameters.BOOST_PP_CAT(Parameter, ParameterIndex); \
    } \
    VTKM_EXEC_CONT_EXPORT \
    static \
    void SetParameter(ParameterContainer<FunctionSignature> &parameters, \
                      const ParameterType &value) { \
      parameters.BOOST_PP_CAT(Parameter, ParameterIndex) = value; \
    } \
  };

#define VTK_M_PARAMETER_CONTAINER_ACCESS_REPEAT(z, i, data) \
  VTK_M_PARAMETER_CONTAINER_ACCESS(BOOST_PP_INC(i))
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_PARAMETER_CONTAINER_ACCESS_REPEAT,)

#undef VTK_M_PARAMETER_CONTAINER_ACCESS_REPEAT
#undef VTK_M_PARAMETER_CONTAINER_ACCESS

template<int ParameterIndex, typename FunctionSignature>
VTKM_EXEC_CONT_EXPORT
typename ParameterContainerAccess<ParameterIndex,FunctionSignature>::ParameterType
GetParameter(const ParameterContainer<FunctionSignature> &parameters) {
  return ParameterContainerAccess<ParameterIndex,FunctionSignature>::GetParameter(parameters);
}

template<int ParameterIndex, typename FunctionSignature>
VTKM_EXEC_CONT_EXPORT

void SetParameter(ParameterContainer<FunctionSignature> &parameters,
                  const typename ParameterContainerAccess<ParameterIndex,FunctionSignature>::ParameterType &value) {
  return ParameterContainerAccess<ParameterIndex,FunctionSignature>::SetParameter(parameters, value);
}

struct IdentityFunctor {
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  T &operator()(T &x) const { return x; }

  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  const T &operator()(const T &x) const { return x; }
};

// The following code uses the Boost preprocessor utilities to create
// definitions of DoInvoke functions for all supported number of arguments.
// The created functions are conceptually defined as follows:
//
// template<typename Function,
//          typename TransformFunctor,
//          typename P0,
//          typename P1,
//          typename P2,...>
// VTKM_CONT_EXPORT
// void DoInvokeCont(const Function &f,
//                   ParameterContainer<P0(P1,P2,...)> &parameters,
//                   FunctionInterfaceReturnContainer<P0> &result,
//                   const TransformFunctor &transform)
// {
//   result.Value = transform(f(transform(parameters.Parameter1),...));
// }
//
// We define multiple DoInvokeCont and DoInvokeExec that do identical things
// with different exports. It is important to have these separate definitions
// instead of a single version with VTKM_EXEC_CONT_EXPORT because the function
// to be invoked may only be viable in one or the other. There are also
// separate versions that support const functions and non-const functions.
// (However, the structures from the FunctionInterface must always be
// non-const.) Finally, there is a special version for functions that return
// void so that the function does not try to invalidly save a void value.

#define VTK_M_DO_INVOKE_TPARAM(z, count, data) \
  transform(BOOST_PP_CAT(parameters.Parameter, count))

#define VTK_M_DO_INVOKE(NumParamsPlusOne) \
  template<typename Function, \
           typename TransformFunctor, \
           BOOST_PP_ENUM_PARAMS(NumParamsPlusOne, typename P)> \
  VTK_M_DO_INVOKE_EXPORT \
  void VTK_M_DO_INVOKE_NAME( \
      VTK_M_DO_INVOKE_FUNCTION_CONST Function &f, \
      ParameterContainer<P0(BOOST_PP_ENUM_SHIFTED_PARAMS(NumParamsPlusOne, P))> &parameters, \
      FunctionInterfaceReturnContainer<P0> &result, \
      const TransformFunctor &transform) \
  { \
    (void)parameters; \
    (void)transform; \
    result.Value = \
      transform( \
        f(BOOST_PP_ENUM_SHIFTED(NumParamsPlusOne, VTK_M_DO_INVOKE_TPARAM, ))); \
  } \
  \
  template<typename Function, \
           typename TransformFunctor BOOST_PP_COMMA_IF(BOOST_PP_DEC(NumParamsPlusOne)) \
           BOOST_PP_ENUM_SHIFTED_PARAMS(NumParamsPlusOne, typename P)> \
  VTK_M_DO_INVOKE_EXPORT \
  void VTK_M_DO_INVOKE_NAME( \
      VTK_M_DO_INVOKE_FUNCTION_CONST Function &f, \
      ParameterContainer<void(BOOST_PP_ENUM_SHIFTED_PARAMS(NumParamsPlusOne, P))> &parameters, \
      FunctionInterfaceReturnContainer<void> &, \
      const TransformFunctor &transform) \
  { \
    (void)parameters; \
    (void)transform; \
    f(BOOST_PP_ENUM_SHIFTED(NumParamsPlusOne, VTK_M_DO_INVOKE_TPARAM, )); \
  }
#define VTK_M_DO_INVOKE_REPEAT(z, NumParams, data) \
  VTK_M_DO_INVOKE(BOOST_PP_INC(NumParams))

#define VTK_M_DO_INVOKE_NAME DoInvokeCont
#define VTK_M_DO_INVOKE_EXPORT VTKM_CONT_EXPORT
#define VTK_M_DO_INVOKE_FUNCTION_CONST const
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_DO_INVOKE_REPEAT,)
#undef VTK_M_DO_INVOKE_NAME
#undef VTK_M_DO_INVOKE_EXPORT
#undef VTK_M_DO_INVOKE_FUNCTION_CONST

#define VTK_M_DO_INVOKE_NAME DoInvokeCont
#define VTK_M_DO_INVOKE_EXPORT VTKM_CONT_EXPORT
#define VTK_M_DO_INVOKE_FUNCTION_CONST
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_DO_INVOKE_REPEAT,)
#undef VTK_M_DO_INVOKE_NAME
#undef VTK_M_DO_INVOKE_EXPORT
#undef VTK_M_DO_INVOKE_FUNCTION_CONST

#define VTK_M_DO_INVOKE_NAME DoInvokeExec
#define VTK_M_DO_INVOKE_EXPORT VTKM_EXEC_EXPORT
#define VTK_M_DO_INVOKE_FUNCTION_CONST const
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_DO_INVOKE_REPEAT,)
#undef VTK_M_DO_INVOKE_NAME
#undef VTK_M_DO_INVOKE_EXPORT
#undef VTK_M_DO_INVOKE_FUNCTION_CONST

#define VTK_M_DO_INVOKE_NAME DoInvokeExec
#define VTK_M_DO_INVOKE_EXPORT VTKM_EXEC_EXPORT
#define VTK_M_DO_INVOKE_FUNCTION_CONST
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_DO_INVOKE_REPEAT,)
#undef VTK_M_DO_INVOKE_NAME
#undef VTK_M_DO_INVOKE_EXPORT
#undef VTK_M_DO_INVOKE_FUNCTION_CONST

#undef VTK_M_DO_INVOKE_REPEAT
#undef VTK_M_DO_INVOKE
#undef VTK_M_DO_INVOKE_TPARAM


// These functions exist to help copy components of a FunctionInterface.

template<int NumToCopy, int ParameterIndex = 1>
struct FunctionInterfaceCopyParameters {
  template<typename DestSignature, typename SrcSignature>
  static
  VTKM_EXEC_CONT_EXPORT
  void Copy(vtkm::internal::detail::ParameterContainer<DestSignature> &dest,
            const vtkm::internal::detail::ParameterContainer<SrcSignature> &src)
  {
    vtkm::internal::detail::SetParameter<ParameterIndex>(
          dest,vtkm::internal::detail::GetParameter<ParameterIndex>(src));
   FunctionInterfaceCopyParameters<NumToCopy-1,ParameterIndex+1>::Copy(dest, src);
  }
};

template<int ParameterIndex>
struct FunctionInterfaceCopyParameters<0, ParameterIndex> {
  template<typename DestSignature, typename SrcSignature>
  static
  VTKM_EXEC_CONT_EXPORT
  void Copy(vtkm::internal::detail::ParameterContainer<DestSignature> &,
            const vtkm::internal::detail::ParameterContainer<SrcSignature> &)
  {
    // Nothing left to copy.
  }
};

template<typename OriginalSignature, typename Transform>
struct FunctionInterfaceStaticTransformType;

// The following code uses the Boost preprocessor utilities to create
// definitions of DoStaticTransform functions for all supported number of
// arguments. The created functions are conceptually defined as follows:
//
// template<typename Transform,
//          typename OriginalSignature,
//          typename TransformedSignature>
// VTKM_CONT_EXPORT
// void DoStaticTransformCont(
//     const Transform &transform,
//     const ParameterContainer<OriginalSignature> &originalParameters,
//     ParameterContainer<TransformedSignature> &transformedParameters)
// {
//   transformedParameters.Parameter1 = transform(originalParameters.Parameter1);
//   transformedParameters.Parameter2 = transform(originalParameters.Parameter2);
//   ...
// }
//
// We define multiple DoStaticTransformCont and DoStaticTransformExec that do
// identical things with different exports. It is important to have these
// separate definitions instead of a single version with VTKM_EXEC_CONT_EXPORT
// because the transform to be invoked may only be viable in one or the other.

#define VTK_M_DO_STATIC_TRANSFORM_ASSIGN(z, count, data) \
  BOOST_PP_IF(count, \
              BOOST_PP_CAT(transformedParameters.Parameter, count) = \
                transform(BOOST_PP_CAT(originalParameters.Parameter, count));,)

#define VTK_M_DO_STATIC_TRANSFORM(NumParamsPlusOne) \
  template<typename Transform, \
           BOOST_PP_ENUM_PARAMS(NumParamsPlusOne, typename OriginalP), \
           BOOST_PP_ENUM_PARAMS(NumParamsPlusOne, typename TransformedP)> \
  VTK_M_DO_STATIC_TRANSFORM_EXPORT \
  void VTK_M_DO_STATIC_TRANSFORM_NAME( \
      const Transform &transform, \
      const ParameterContainer<OriginalP0(BOOST_PP_ENUM_SHIFTED_PARAMS(NumParamsPlusOne, OriginalP))> &originalParameters, \
      ParameterContainer<TransformedP0(BOOST_PP_ENUM_SHIFTED_PARAMS(NumParamsPlusOne, TransformedP))> &transformedParameters) \
  { \
    (void)transform; \
    (void)originalParameters; \
    (void)transformedParameters; \
    BOOST_PP_REPEAT(NumParamsPlusOne, VTK_M_DO_STATIC_TRANSFORM_ASSIGN,) \
  }
#define VTK_M_DO_STATIC_TRANSFORM_REPEAT(z, NumParams, data) \
  VTK_M_DO_STATIC_TRANSFORM(BOOST_PP_INC(NumParams))

#define VTK_M_DO_STATIC_TRANSFORM_NAME DoStaticTransformCont
#define VTK_M_DO_STATIC_TRANSFORM_EXPORT VTKM_CONT_EXPORT
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_DO_STATIC_TRANSFORM_REPEAT,)
#undef VTK_M_DO_STATIC_TRANSFORM_EXPORT
#undef VTK_M_DO_STATIC_TRANSFORM_NAME

#define VTK_M_DO_STATIC_TRANSFORM_NAME DoStaticTransformExec
#define VTK_M_DO_STATIC_TRANSFORM_EXPORT VTKM_EXEC_EXPORT
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_DO_STATIC_TRANSFORM_REPEAT,)
#undef VTK_M_DO_STATIC_TRANSFORM_EXPORT
#undef VTK_M_DO_STATIC_TRANSFORM_NAME

#undef VTK_M_DO_STATIC_TRANSFORM_REPEAT
#undef VTK_M_DO_STATIC_TRANSFORM
#undef VTK_M_DO_STATIC_TRANSFORM_ASSIGN

template<typename OriginalFunction,
         typename NewFunction,
         typename TransformFunctor,
         typename FinishFunctor>
class FunctionInterfaceDynamicTransformContContinue;

// The following code uses the Boost preprocessor utilities to create
// definitions of DoForEach functions for all supported number of arguments.
// The created functions are conceptually defined as follows:
//
// template<typename Functor, typename P0, typename P1, typename P2,...>
// VTKM_CONT_EXPORT
// void DoForEachCont(const Functor &f,
//                    ParameterContainer<P0(P1,P2,...)> &parameters)
//
// {
//   f(parameters.Parameter1);
//   f(parameters.Parameter2);
//   ...
// }
//
// We define multiple DoForEachCont and DoForEachExec that do identical things
// with different exports. It is important to have these separate definitions
// instead of a single version with VTKM_EXEC_CONT_EXPORT because the functor
// to be invoked on each parameter may only be viable in one or the other.
// There are also separate versions that support a const FunctionInterface and
// a non-const FunctionInterface.

#define VTK_M_DO_FOR_EACH_CALL_PARAM(z, count, data) \
  BOOST_PP_IF(count, f(BOOST_PP_CAT(parameters.Parameter, count));,)

#define VTK_M_DO_FOR_EACH(NumParamsPlusOne) \
  template<typename Functor, \
           BOOST_PP_ENUM_PARAMS(NumParamsPlusOne, typename P)> \
  VTK_M_DO_FOR_EACH_EXPORT \
  void VTK_M_DO_FOR_EACH_NAME( \
      const Functor &f, \
      VTK_M_DO_FOR_EACH_FI_CONST ParameterContainer<P0(BOOST_PP_ENUM_SHIFTED_PARAMS(NumParamsPlusOne, P))> &parameters) \
  { \
    (void)f; \
    (void)parameters; \
    BOOST_PP_REPEAT(NumParamsPlusOne, VTK_M_DO_FOR_EACH_CALL_PARAM,) \
  }
#define VTK_M_DO_FOR_EACH_REPEAT(z, NumParams, data) \
  VTK_M_DO_FOR_EACH(BOOST_PP_INC(NumParams))

#define VTK_M_DO_FOR_EACH_EXPORT VTKM_CONT_EXPORT
#define VTK_M_DO_FOR_EACH_NAME DoForEachCont
#define VTK_M_DO_FOR_EACH_FI_CONST const
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_DO_FOR_EACH_REPEAT,)
#undef VTK_M_DO_FOR_EACH_FI_CONST
#undef VTK_M_DO_FOR_EACH_NAME
#undef VTK_M_DO_FOR_EACH_EXPORT

#define VTK_M_DO_FOR_EACH_EXPORT VTKM_CONT_EXPORT
#define VTK_M_DO_FOR_EACH_NAME DoForEachCont
#define VTK_M_DO_FOR_EACH_FI_CONST
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_DO_FOR_EACH_REPEAT,)
#undef VTK_M_DO_FOR_EACH_FI_CONST
#undef VTK_M_DO_FOR_EACH_NAME
#undef VTK_M_DO_FOR_EACH_EXPORT

#define VTK_M_DO_FOR_EACH_EXPORT VTKM_EXEC_EXPORT
#define VTK_M_DO_FOR_EACH_NAME DoForEachExec
#define VTK_M_DO_FOR_EACH_FI_CONST const
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_DO_FOR_EACH_REPEAT,)
#undef VTK_M_DO_FOR_EACH_FI_CONST
#undef VTK_M_DO_FOR_EACH_NAME
#undef VTK_M_DO_FOR_EACH_EXPORT

#define VTK_M_DO_FOR_EACH_EXPORT VTKM_EXEC_EXPORT
#define VTK_M_DO_FOR_EACH_NAME DoForEachExec
#define VTK_M_DO_FOR_EACH_FI_CONST
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_DO_FOR_EACH_REPEAT,)
#undef VTK_M_DO_FOR_EACH_FI_CONST
#undef VTK_M_DO_FOR_EACH_NAME
#undef VTK_M_DO_FOR_EACH_EXPORT

#undef VTK_M_DO_FOR_EACH_REPEAT
#undef VTK_M_DO_FOR_EACH
#undef VTK_M_DO_FOR_EACH_CALL_PARAM

} // namespace detail

/// \brief Holds parameters and result of a function.
///
/// To make VTK-m easier for the end user developer, the
/// vtkm::cont::Dispatcher*::Invoke() method takes an arbitrary amount of
/// arguments that get transformed and swizzled into arguments and return value
/// for a worklet operator. In between these two invocations a complicated
/// series of transformations and operations can occur.
///
/// Supporting arbitrary function and template arguments is difficult and
/// really requires seperate implementations for ANSI and C++11 versions of
/// compilers. Thus, variatic template arguments are, at this point in time,
/// something to be avoided when possible. The intention of \c
/// FunctionInterface is to collect most of the variatic template code into one
/// place. The \c FunctionInterface template class takes a function signature,
/// which can have a variable number of arguments. The \c FunctionInterface
/// will hold in its state a copy of all input parameters (regardless of number
/// or type) and the return value if it exists (i.e. non-null) and the function
/// has been invoked. This means that all arguments can be passed around in a
/// single object so that objects and functions dealing with these variadic
/// parameters can be templated on a single type (the type of \c
/// FunctionInterface).
///
/// Note that the indexing of the parameters in a \c FunctionInterface starts
/// at 1. You can think of the return value being the parameter at index 0,
/// even if there is no return value. Although this is uncommon in C++, it
/// matches better the parameter indexing for other classes that deal with
/// function signatures.
///
/// The \c FunctionInterface contains several ways to invoke a functor whose
/// parameters match those of the parameter pack. This allows you to complete
/// the transition of calling an arbitrary function (like a worklet).
///
/// The following is a rundown of a \c FunctionInterface is created and used.
/// See the independent documentation for more details.
///
/// Use the \c make_FunctionInterface function to create a \c FunctionInterface
/// and initialize the state of all the parameters. \c make_FunctionInterface
/// takes a variable number of arguments, one for each parameter. Since the
/// return type is not specified as an argument, you must always specify it as
/// a template parameter.
///
/// \code{.cpp}
/// vtkm::internal::FunctionInterface<void(int,double,char)> functionInterface =
///     vtkm::internal::make_FunctionInterface<void>(1, 2.5, 'a');
/// \endcode
///
/// The number of parameters can be retrieved either with the constant field
/// \c ARITY or with the \c GetArity method.
///
/// \code{.cpp}
/// functionInterface.GetArity();
/// \endcode
///
/// You can get a particular parameter using the templated method \c
/// GetParameter. The template parameter is the index of the parameter
/// (starting at 1). Note that if the \c FunctionInterface is used in a
/// templated function or method where the type is not fully resolved, you need
/// to use the \c template keyword. One of the two forms should work. Try
/// switching if you get a compiler error.
///
/// \code{.cpp}
/// // Use this form if functionInterface is a fully resolved type.
/// functionInterface.GetParameter<1>();
///
/// // Use this form if functionInterface is partially specified.
/// functionInterface.template GetParameter<1>();
/// \endcode
///
/// Likewise, there is a \c SetParameter method for changing parameters. The
/// same rules for indexing and template specification apply.
///
/// \code{.cpp}
/// // Use this form if functionInterface is a fully resolved type.
/// functionInterface.SetParameter<1>(100);
///
/// // Use this form if functionInterface is partially specified.
/// functionInterface.template SetParameter<1>(100);
/// \endcode
///
/// \c FunctionInterface can invoke a functor of a matching signature using the
/// parameters stored within. If the functor returns a value, that return value
/// will be stored in the \c FunctionInterface object for later retrieval.
/// There are several versions of the invoke method including those for the
/// control and execution environments as well as methods that allow
/// transformation of the parameters and return value. See the method document
/// for more details.
///
/// \code{.cpp}
/// functionInterface.InvokeCont(Functor());
/// \endcode
///
/// Once a functor has been invoked, the return value can be retrieved with the
/// \c GetReturnValue method. \c GetReturnValue should only be used if the
/// function signature has a non-void return value. Otherwise calling this
/// method will result in a compile error.
///
/// \code{.cpp}
/// functionInterface.GetReturnValue();
/// \endcode
///
/// Providing the appropriate template specification to specialize when there
/// is no return value can be done but can be tricky. To make it easier, \c
/// FunctionInterface also has a \c GetReturnValueSafe method that provides the
/// return value wrapped in a \c FunctionInterfaceReturnContainer structure.
/// This will work regardless of whether the return value exists (although this
/// container might be empty). Specializing on the type of \c
/// FunctionInterfaceReturnContainer is much easier.
///
/// \code{.cpp}
/// functionInterface.GetReturnValueSafe();
/// \endcode
///
/// \c FunctionInterface also provides several methods for modifying the
/// parameters. First, the \c Append method tacks an additional parameter to
/// the end of the function signature.
///
/// \code{.cpp}
/// functionInterface.Append<std::string>(std::string("New Arg"));
/// \endcode
///
/// Next, the \c Replace method removes a parameter at a particular position
/// and replaces it with another object of a different type.
///
/// \code{.cpp}
/// functionInterface.Replace<1>(std::string("new first argument"));
/// \endcode
///
/// Finally, there are a couple of ways to replace all of the parameters at
/// once. The \c StaticTransform methods take a transform functor that modifies
/// each of the parameters. The \c DynamicTransform methods similarly take a
/// transform functor, but is called in a different way to defer the type
/// resolution to run time. See the documentation for each of these methods for
/// details on how they are used.
///
template<typename FunctionSignature>
class FunctionInterface
{
  template<typename OtherSignature>
  friend class FunctionInterface;

public:
  typedef FunctionSignature Signature;

  typedef typename boost::function_types::result_type<FunctionSignature>::type
      ResultType;
  template<int ParameterIndex>
  struct ParameterType {
    typedef typename boost::mpl::at_c<
        boost::function_types::components<FunctionSignature>,
        ParameterIndex>::type type;
  };
  static const bool RETURN_VALID = FunctionInterfaceReturnContainer<ResultType>::VALID;

  /// The number of parameters in this \c Function Interface.
  ///
  static const int ARITY =
      boost::function_types::function_arity<FunctionSignature>::value;

  /// Returns the number of parameters held in this \c FunctionInterface. The
  /// return value is the same as \c ARITY.
  ///
  VTKM_EXEC_CONT_EXPORT
  int GetArity() const { return ARITY; }

  /// Retrieves the return value from the last invocation called. This method
  /// will result in a compiler error if used with a function having a void
  /// return type.
  ///
  VTKM_EXEC_CONT_EXPORT
  ResultType GetReturnValue() const { return this->Result.Value; }

  /// Retrieves the return value from the last invocation wrapped in a \c
  /// FunctionInterfaceReturnContainer object. This call can succeed even if
  /// the return type is void. You still have to somehow check to make sure the
  /// return is non-void before trying to use it, but using this method can
  /// simplify templated programming.
  ///
  VTKM_EXEC_CONT_EXPORT
  const FunctionInterfaceReturnContainer<ResultType> &GetReturnValueSafe() const
  {
    return this->Result;
  }
  VTKM_EXEC_CONT_EXPORT
  FunctionInterfaceReturnContainer<ResultType> &GetReturnValueSafe()
  {
    return this->Result;
  }

  /// Gets the value for the parameter of the given index. Parameters are
  /// indexed starting at 1. To use this method you have to specify the index
  /// as a template parameter. If you are using FunctionInterface within a
  /// template (which is almost always the case), then you will have to use the
  /// template keyword. For example, here is a simple implementation of a
  /// method that grabs the first parameter of FunctionInterface.
  ///
  /// \code{.cpp}
  /// template<FunctionSignature>
  /// void Foo(const vtkm::cont::internal::FunctionInterface<FunctionSignature> &fInterface)
  /// {
  ///   bar(fInterface.template GetParameter<1>());
  /// }
  /// \endcode
  ///
  template<int ParameterIndex>
  VTKM_EXEC_CONT_EXPORT
  typename ParameterType<ParameterIndex>::type
  GetParameter() const {
    return detail::GetParameter<ParameterIndex>(this->Parameters);
  }

  /// Sets the value for the parameter of the given index. Parameters are
  /// indexed starting at 1. To use this method you have to specify the index
  /// as a template parameter. If you are using FunctionInterface within a
  /// template (which is almost always the case), then you will have to use the
  /// template keyword.
  ///
  template<int ParameterIndex>
  VTKM_EXEC_CONT_EXPORT
  void SetParameter(typename ParameterType<ParameterIndex>::type parameter)
  {
    detail::SetParameter<ParameterIndex>(this->Parameters, parameter);
  }

  /// Copies the parameters and return values from the given \c
  /// FunctionInterface to this object. The types must be copiable from source
  /// to destination. If the number of parameters in the two objects are not
  /// the same, copies the first N arguments, where N is the smaller arity of
  /// the two function interfaces.
  ///
  template<typename SrcFunctionSignature>
  void Copy(const FunctionInterface<SrcFunctionSignature> &src)
  {
    this->Result = src.GetReturnValueSafe();
    detail::FunctionInterfaceCopyParameters<
        boost::static_unsigned_min<ARITY, FunctionInterface<SrcFunctionSignature>::ARITY>::value>::
        Copy(this->Parameters, src.Parameters);
  }

  /// Invoke a function \c f using the arguments stored in this
  /// FunctionInterface.
  ///
  /// If this FunctionInterface specifies a non-void return value, then the
  /// result of the function call is stored within this FunctionInterface and
  /// can be retrieved with GetReturnValue().
  ///
  template<typename Function>
  VTKM_CONT_EXPORT
  void InvokeCont(const Function &f) {
    detail::DoInvokeCont(f,
                         this->Parameters,
                         this->Result,
                         detail::IdentityFunctor());
  }
  template<typename Function>
  VTKM_CONT_EXPORT
  void InvokeCont(Function &f) {
    detail::DoInvokeCont(f,
                         this->Parameters,
                         this->Result,
                         detail::IdentityFunctor());
  }
  template<typename Function>
  VTKM_EXEC_EXPORT
  void InvokeExec(const Function &f) {
    detail::DoInvokeExec(f,
                         this->Parameters,
                         this->Result,
                         detail::IdentityFunctor());
  }
  template<typename Function>
  VTKM_EXEC_EXPORT
  void InvokeExec(Function &f) {
    detail::DoInvokeExec(f,
                         this->Parameters,
                         this->Result,
                         detail::IdentityFunctor());
  }

  /// Invoke a function \c f using the arguments stored in this
  /// FunctionInterface and a transform.
  ///
  /// These versions of invoke also apply a transform to the input arguments.
  /// The transform is a second functor passed a second argument. If this
  /// FunctionInterface specifies a non-void return value, then the result of
  /// the function call is also transformed and stored within this
  /// FunctionInterface and can be retrieved with GetReturnValue().
  ///
  template<typename Function, typename TransformFunctor>
  VTKM_CONT_EXPORT
  void InvokeCont(const Function &f, const TransformFunctor &transform) {
    detail::DoInvokeCont(f, this->Parameters, this->Result, transform);
  }
  template<typename Function, typename TransformFunctor>
  VTKM_CONT_EXPORT
  void InvokeCont(Function &f, const TransformFunctor &transform) {
    detail::DoInvokeCont(f, this->Parameters, this->Result, transform);
  }
  template<typename Function, typename TransformFunctor>
  VTKM_EXEC_EXPORT
  void InvokeExec(const Function &f, const TransformFunctor &transform) {
    detail::DoInvokeExec(f, this->Parameters, this->Result, transform);
  }
  template<typename Function, typename TransformFunctor>
  VTKM_EXEC_EXPORT
  void InvokeExec(Function &f, const TransformFunctor &transform) {
    detail::DoInvokeExec(f, this->Parameters, this->Result, transform);
  }

  template<typename NewType>
  struct AppendType {
    typedef FunctionInterface<
        typename boost::function_types::function_type<
          typename boost::mpl::push_back<
            boost::function_types::components<FunctionSignature>,
            NewType
          >::type
        >::type
      > type;
  };

  /// Returns a new \c FunctionInterface with all the parameters of this \c
  /// FunctionInterface and the given method argument appended to these
  /// parameters. The return type can be determined with the \c AppendType
  /// template.
  ///
  template<typename NewType>
  VTKM_EXEC_CONT_EXPORT
  typename AppendType<NewType>::type
  Append(NewType newParameter) const {
    typename AppendType<NewType>::type appendedFuncInterface;
    appendedFuncInterface.Copy(*this);
    appendedFuncInterface.template SetParameter<ARITY+1>(newParameter);
    return appendedFuncInterface;
  }

  template<int ParameterIndex, typename NewType>
  class ReplaceType {
    typedef boost::function_types::components<FunctionSignature> ThisFunctionComponents;
    typedef typename boost::mpl::advance_c<typename boost::mpl::begin<ThisFunctionComponents>::type, ParameterIndex>::type ToRemovePos;
    typedef typename boost::mpl::erase<ThisFunctionComponents, ToRemovePos>::type ComponentRemoved;
    typedef typename boost::mpl::advance_c<typename boost::mpl::begin<ComponentRemoved>::type, ParameterIndex>::type ToInsertPos;
    typedef typename boost::mpl::insert<ComponentRemoved, ToInsertPos, NewType>::type ComponentInserted;
    typedef typename boost::function_types::function_type<ComponentInserted>::type NewSignature;
  public:
    typedef FunctionInterface<NewSignature> type;
  };

  /// Returns a new \c FunctionInterface with all the parameters of this \c
  /// FunctionInterface except that the parameter indexed at the template
  /// parameter \c ParameterIndex is replaced with the given argument. This
  /// method can be used in place of SetParameter when the parameter type
  /// changes. The return type can be determined with the \c ReplaceType
  /// template.
  ///
  template<int ParameterIndex, typename NewType>
  VTKM_EXEC_CONT_EXPORT
  typename ReplaceType<ParameterIndex, NewType>::type
  Replace(NewType newParameter) const {
    typename ReplaceType<ParameterIndex, NewType>::type replacedFuncInterface;
    detail::FunctionInterfaceCopyParameters<ParameterIndex-1>::
        Copy(replacedFuncInterface.Parameters, this->Parameters);
    replacedFuncInterface.template SetParameter<ParameterIndex>(newParameter);
    detail::FunctionInterfaceCopyParameters<ARITY-ParameterIndex,ParameterIndex+1>::
        Copy(replacedFuncInterface.Parameters, this->Parameters);
    return replacedFuncInterface;
  }

  template<typename Transform>
  struct StaticTransformType {
    typedef FunctionInterface<
        typename detail::FunctionInterfaceStaticTransformType<
          FunctionSignature,Transform>::type> type;
  };

  /// \brief Transforms the \c FunctionInterface based on compile-time
  /// information.
  ///
  /// The \c StaticTransform methods transform all the parameters of this \c
  /// FunctionInterface to different types and values based on compile-time
  /// information. It operates by accepting a functor that defines a unary
  /// function whose argument is the parameter to transform and the return
  /// value is the transformed value. The functor must also contain a templated
  /// struct name ReturnType with an internal type named \c type that defines
  /// the return type of the transform for a given input type.
  ///
  /// The transformation is only applied to the parameters of the function. The
  /// return argument is uneffected.
  ///
  /// The return type can be determined with the \c StaticTransformType
  /// template.
  ///
  /// Here is an example of a transformation that converts a \c
  /// FunctionInterface to another \c FunctionInterface containing pointers to
  /// all of the parameters.
  ///
  /// \code
  /// struct MyTransformFunctor {
  ///   template<typename T>
  ///   struct ReturnType {
  ///     typedef const T *type;
  ///   };
  ///
  ///   template<typename T>
  ///   DAX_CONT_EXPORT
  ///   const T *operator()(const T &x) const {
  ///     return &x;
  ///   }
  /// };
  ///
  /// template<typename FunctionSignature>
  /// typename vtkm::internal::FunctionInterface<FunctionSignature>::template StaticTransformType<MyTransformFunctor>::type
  /// ImportantStuff(const vtkm::internal::FunctionInterface<FunctionSignature> &funcInterface)
  /// {
  ///   return funcInterface.StaticTransformCont(MyTransformFunctor());
  /// }
  /// \endcode
  ///
  template<typename Transform>
  VTKM_CONT_EXPORT
  typename StaticTransformType<Transform>::type
  StaticTransformCont(const Transform &transform) const
  {
    typename StaticTransformType<Transform>::type newFuncInterface;
    detail::DoStaticTransformCont(transform,
                                  this->Parameters,
                                  newFuncInterface.Parameters);
    return newFuncInterface;
  }
  template<typename Transform>
  VTKM_EXEC_EXPORT
  typename StaticTransformType<Transform>::type
  StaticTransformExec(const Transform &transform) const
  {
    typename StaticTransformType<Transform>::type newFuncInterface;
    detail::DoStaticTransformExec(transform,
                                  this->Parameters,
                                  newFuncInterface.Parameters);
    return newFuncInterface;
  }

  /// \brief Transforms the \c FunctionInterface based on run-time information.
  ///
  /// The \c DynamicTransform method transforms all the parameters of this \c
  /// FunctionInterface to different types and values based on run-time
  /// information. It operates by accepting two functors. The first functor
  /// accepts two arguments. The first argument is a parameter to transform and
  /// the second is a functor to call with the transformed result.
  ///
  /// The second argument to \c DynamicTransform is another function that
  /// accepts the transformed \c FunctionInterface and does something. If that
  /// transformed \c FunctionInterface has a return value, that return value
  /// will be passed back to this \c FunctionInterface.
  ///
  /// Here is a contrived but illustrative example. This transformation will
  /// pass all arguments except any string that looks like a number will be
  /// converted to a vtkm::Scalar. Note that because the types are not
  /// determined till runtime, this transform cannot be determined at compile
  /// time with meta-template programming.
  ///
  /// \code
  /// struct MyTransformFunctor {
  ///   template<typename InputType, typename ContinueFunctor>
  ///   VTKM_CONT_EXPORT
  ///   void operator()(const InputType &input,
  ///                   const ContinueFunctor &continueFunc) const
  ///   {
  ///     continueFunc(input);
  ///   }
  ///
  ///   template<typename ContinueFunctor>
  ///   VTKM_CONT_EXPORT
  ///   void operator()(const std::string &input,
  ///                   const ContinueFunctor &continueFunc) const
  ///   {
  ///     if ((input[0] >= '0' && (input[0] <= '9'))
  ///     {
  ///       std::stringstream stream(input);
  ///       vtkm::Scalar value;
  ///       stream >> value;
  ///       continueFunc(value);
  ///     }
  ///     else
  ///     {
  ///       continueFunc(input);
  ///     }
  ///   }
  /// };
  ///
  /// struct MyFinishFunctor {
  ///   template<typename FunctionSignature>
  ///   VTKM_CONT_EXPORT
  ///   void operator()(vtkm::internal::FunctionInterface<FunctionSignature> &funcInterface) const
  ///   {
  ///     // Do something
  ///   }
  /// };
  ///
  /// template<typename FunctionSignature>
  /// void ImportantStuff(vtkm::internal::FunctionInterface<FunctionSignature> &funcInterface)
  /// {
  ///   funcInterface.DynamicTransformCont(MyContinueFunctor(), MyFinishFunctor());
  /// }
  /// \endcode
  ///
  /// An interesting feature of \c DynamicTransform is that there does not have
  /// to be a one-to-one transform. It is possible to make many valid
  /// transforms by calling the continue functor multiple times within the
  /// transform functor. It is also possible to abort the transform by not
  /// calling the continue functor.
  ///
  template<typename TransformFunctor, typename FinishFunctor>
  VTKM_CONT_EXPORT
  void DynamicTransformCont(const TransformFunctor &transform,
                            const FinishFunctor &finish) {
    typedef detail::FunctionInterfaceDynamicTransformContContinue<
        FunctionSignature,
        ResultType(),
        TransformFunctor,
        FinishFunctor> ContinueFunctorType;

    FunctionInterface<ResultType()> emptyInterface;
    ContinueFunctorType continueFunctor =
        ContinueFunctorType(*this, emptyInterface, transform, finish);

    continueFunctor.DoNextTransform(emptyInterface);
    this->Result = emptyInterface.GetReturnValueSafe();
  }

  /// \brief Applies a function to all the parameters.
  ///
  /// The \c ForEach methods take a function and apply that function to each
  /// of the parameters in the \c FunctionInterface. (Return values are not
  /// effected.)
  ///
  template<typename Functor>
  VTKM_CONT_EXPORT
  void ForEachCont(const Functor &f) const {
    detail::DoForEachCont(f, this->Parameters);
  }
  template<typename Functor>
  VTKM_CONT_EXPORT
  void ForEachCont(const Functor &f) {
    detail::DoForEachCont(f, this->Parameters);
  }
  template<typename Functor>
  VTKM_EXEC_EXPORT
  void ForEachExec(const Functor &f) const {
    detail::DoForEachExec(f, this->Parameters);
  }
  template<typename Functor>
  VTKM_EXEC_EXPORT
  void ForEachExec(const Functor &f) {
    detail::DoForEachExec(f, this->Parameters);
  }

private:
  vtkm::internal::FunctionInterfaceReturnContainer<ResultType> Result;
  detail::ParameterContainer<FunctionSignature> Parameters;
};

namespace detail {

// The following code uses the Boost preprocessor utilities to create
// definitions of FunctionInterfaceStaticTransformType for all supported number
// of arguments. The created classes are conceptually defined as follows:
//
// template<typename Transform,
//          typename P0, // Return type
//          typename P1,
//          typename P2, ...>
// struct FunctionInterfaceStaticTransformType<P0(P1,P2,...), Transform> {
//   typedef P0(type)(typename Transform::template ReturnType<P1>::type,
//                    typename Transform::template ReturnType<P2>::type, ...);
// };

#define VTK_M_STATIC_TRANSFORM_TPARAM(z, ParamIndex, data) \
  BOOST_PP_IF( \
    ParamIndex, \
    typename Transform::template ReturnType<BOOST_PP_CAT(P,ParamIndex)>::type,)

#define VTK_M_STATIC_TRANSFORM_TYPE(NumParamsPlusOne) \
  template<typename Transform, \
           BOOST_PP_ENUM_PARAMS(NumParamsPlusOne, typename P)> \
  struct FunctionInterfaceStaticTransformType< \
      P0(BOOST_PP_ENUM_SHIFTED_PARAMS(NumParamsPlusOne, P)), \
      Transform> \
  { \
    typedef P0(type)( \
        BOOST_PP_ENUM_SHIFTED(NumParamsPlusOne, VTK_M_STATIC_TRANSFORM_TPARAM,) \
      ); \
  };
#define VTK_M_STATIC_TRANSFORM_TYPE_REPEAT(z, NumParams, data) \
  VTK_M_STATIC_TRANSFORM_TYPE(BOOST_PP_INC(NumParams))

BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_STATIC_TRANSFORM_TYPE_REPEAT,)

#undef VTK_M_STATIC_TRANSFORM_TYPE_REPEAT
#undef VTK_M_STATIC_TRANSFORM_TYPE
#undef VTK_M_STATIC_TRANSFORM_TPARAM


template<typename OriginalFunction,
         typename NewFunction,
         typename TransformFunctor,
         typename FinishFunctor>
class FunctionInterfaceDynamicTransformContContinue
{
public:
  FunctionInterfaceDynamicTransformContContinue(
      vtkm::internal::FunctionInterface<OriginalFunction> &originalInterface,
      vtkm::internal::FunctionInterface<NewFunction> &newInterface,
      const TransformFunctor &transform,
      const FinishFunctor &finish)
    : OriginalInterface(originalInterface),
      NewInterface(newInterface),
      Transform(transform),
      Finish(finish)
  {  }

  template<typename T>
  VTKM_CONT_EXPORT
  void operator()(T newParameter) const
  {
    typedef typename vtkm::internal::FunctionInterface<NewFunction>::template AppendType<T>::type
        NextInterfaceType;
    NextInterfaceType nextInterface = this->NewInterface.Append(newParameter);
    this->DoNextTransform(nextInterface);
    this->NewInterface.GetReturnValueSafe()
        = nextInterface.GetReturnValueSafe();
  }

  template<typename NextFunction>
  VTKM_CONT_EXPORT
  typename boost::enable_if_c<
    vtkm::internal::FunctionInterface<NextFunction>::ARITY
    < vtkm::internal::FunctionInterface<OriginalFunction>::ARITY>::type
  DoNextTransform(
      vtkm::internal::FunctionInterface<NextFunction> &nextInterface) const
  {
    typedef FunctionInterfaceDynamicTransformContContinue<
        OriginalFunction,NextFunction,TransformFunctor,FinishFunctor> NextContinueType;
    NextContinueType nextContinue = NextContinueType(this->OriginalInterface,
                                                     nextInterface,
                                                     this->Transform,
                                                     this->Finish);
    this->Transform(this->OriginalInterface.template GetParameter<vtkm::internal::FunctionInterface<NextFunction>::ARITY + 1>(),
                    nextContinue);
  }

  template<typename NextFunction>
  VTKM_CONT_EXPORT
  typename boost::disable_if_c<
    vtkm::internal::FunctionInterface<NextFunction>::ARITY
    < vtkm::internal::FunctionInterface<OriginalFunction>::ARITY>::type
  DoNextTransform(
      vtkm::internal::FunctionInterface<NextFunction> &nextInterface) const
  {
    this->Finish(nextInterface);
  }

private:
  vtkm::internal::FunctionInterface<OriginalFunction> &OriginalInterface;
  vtkm::internal::FunctionInterface<NewFunction> &NewInterface;
  const TransformFunctor &Transform;
  const FinishFunctor &Finish;
};

} // namespace detail

#ifdef VTKM_DOXYGEN_ONLY
/// \brief Create a \c FunctionInterface
///
/// \c make_FunctionInterface is a function that takes a variable number of
/// arguments and returns a \c FunctionInterface object containing these
/// objects. Since the return type for the function signature is not specified,
/// you must always specify it as a template parameter
///
/// \code{.cpp}
/// vtkm::internal::FunctionInterface<void(int,double,char)> functionInterface =
///     vtkm::internal::make_FunctionInterface<void>(1, 2.5, 'a');
/// \endcode
///
template<typename P0, typename... P>
VTKM_EXEC_CONT_EXPORT
vtkm::internal::FunctionInterface<P0(P...)>
make_FunctionInterface(P... parameters);
#endif //VTKM_DOXYGEN_ONLY

// The following code uses the Boost preprocessor utilities to create
// definitions of make_FunctionInterface for all supported number of arguments.
// The created functions are conceptually defined as follows:
//
// template<typename P0, // Return type
//          typename P1,
//          typename P2, ...>
// VTKM_EXEC_CONT_EXPORT
// FunctionInterface<P0(P1,P2,...)>
// make_FunctionInterface(P1 p1, P2 p2,...) {
//   FunctionInterface<P0(P1,P2,...)> fi;
//   fi.template SetParameters<1>(p1);
//   fi.template SetParameters<2>(p2);
//   ...
//   return fi;
// }

#define VTK_M_SET_PARAMETER(z, ParamIndex, data) \
  BOOST_PP_IF( \
      ParamIndex, \
      fi.template SetParameter<ParamIndex>(BOOST_PP_CAT(p, ParamIndex));,)

#define VTK_M_MAKE_FUNCTION_INTERFACE(NumParamsPlusOne) \
  template<BOOST_PP_ENUM_PARAMS(NumParamsPlusOne, typename P)> \
  VTKM_EXEC_CONT_EXPORT \
  FunctionInterface<P0(BOOST_PP_ENUM_SHIFTED_PARAMS(NumParamsPlusOne, P))> \
  make_FunctionInterface( \
      BOOST_PP_ENUM_SHIFTED_BINARY_PARAMS(NumParamsPlusOne, P, p)) \
  { \
    FunctionInterface<P0(BOOST_PP_ENUM_SHIFTED_PARAMS(NumParamsPlusOne, P))> fi; \
    BOOST_PP_REPEAT(NumParamsPlusOne, VTK_M_SET_PARAMETER,) \
    return fi; \
  }

#define VTK_M_MAKE_FUNCITON_INTERFACE_REPEAT(z, NumParams, data) \
  VTK_M_MAKE_FUNCTION_INTERFACE(BOOST_PP_INC(NumParams))
BOOST_PP_REPEAT(BOOST_PP_INC(VTKM_MAX_FUNCTION_PARAMETERS),
                VTK_M_MAKE_FUNCITON_INTERFACE_REPEAT,)

#undef VTK_M_MAKE_FUNCITON_INTERFACE_REPEAT
#undef VTK_M_MAKE_FUNCTION_INTERFACE
#undef VTK_M_SET_PARAMETER

}
} // namespace vtkm::internal

#endif //vtk_m_cont_internal_FunctionInterface_h
