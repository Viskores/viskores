//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_FunctionInterface_h
#define vtk_m_internal_FunctionInterface_h

#include <vtkm/Types.h>

#include <vtkm/internal/FunctionInterfaceDetailPre.h>
#include <vtkm/internal/IndexTag.h>

#include <utility>

namespace vtkm
{
namespace internal
{

namespace detail
{

struct IdentityFunctor
{
  template <typename T>
  VTKM_EXEC_CONT T& operator()(T& x) const
  {
    return x;
  }

  template <typename T>
  VTKM_EXEC_CONT const T& operator()(const T& x) const
  {
    return x;
  }
};

// These functions exist to help copy components of a FunctionInterface.

template <vtkm::IdComponent NumToMove, vtkm::IdComponent ParameterIndex = 1>
struct FunctionInterfaceMoveParameters
{
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename DestSignature, typename SrcSignature>
  static VTKM_EXEC_CONT void Move(
    vtkm::internal::detail::ParameterContainer<DestSignature>& dest,
    const vtkm::internal::detail::ParameterContainer<SrcSignature>& src)
  {
    ParameterContainerAccess<ParameterIndex> pca;

    // using forwarding_type = typename AtType<ParameterIndex, SrcSignature>::type;
    pca.Move(dest, src);
    // std::forward<forwarding_type>(pca.Get(src)) );
    // pca.Get(src));
    FunctionInterfaceMoveParameters<NumToMove - 1, ParameterIndex + 1>::Move(dest, src);
  }
};

template <vtkm::IdComponent ParameterIndex>
struct FunctionInterfaceMoveParameters<0, ParameterIndex>
{
  template <typename DestSignature, typename SrcSignature>
  static VTKM_EXEC_CONT void Move(vtkm::internal::detail::ParameterContainer<DestSignature>&,
                                  const vtkm::internal::detail::ParameterContainer<SrcSignature>&)
  {
    // Nothing left to move.
  }
};

template <typename OriginalSignature, typename Transform>
struct FunctionInterfaceStaticTransformType;

template <typename OriginalFunction,
          typename NewFunction,
          typename TransformFunctor,
          typename FinishFunctor>
class FunctionInterfaceDynamicTransformContContinue;

} // namespace detail

/// \brief Holds parameters and result of a function.
///
/// To make VTK-m easier for the end user developer, the
/// \c Invoke method of dispatchers takes an arbitrary amount of
/// arguments that get transformed and swizzled into arguments and return value
/// for a worklet operator. In between these two invocations a complicated
/// series of transformations and operations can occur.
///
/// Supporting arbitrary function and template arguments is difficult and
/// really requires separate implementations for ANSI and C++11 versions of
/// compilers. Thus, variatic template arguments are, at this point in time,
/// something to be avoided when possible. The intention of \c
/// FunctionInterface is to collect most of the variatic template code into one
/// place. The \c FunctionInterface template class takes a function signature,
/// which can have a variable number of arguments. The \c FunctionInterface
/// will hold in its state a copy of all input parameters (regardless of number
/// or type) and the return value if it exists (i.e. non-nullptr) and the function
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
/// parameters match those of the function interface. This allows you to
/// complete the transition of calling an arbitrary function (like a worklet).
///
/// The following is a rundown of how a \c FunctionInterface is created and
/// used. See the independent documentation for more details.
///
/// Use the \c make_FunctionInterface function to create a \c FunctionInterface
/// and initialize the state of all the parameters. \c make_FunctionInterface
/// takes a variable number of arguments, one for each parameter. Since the
/// return type is not specified as an argument, you must always specify it as
/// a template parameter.
///
/// \code{.cpp}
/// vtkm::internal::FunctionInterface<void(vtkm::IdComponent,double,char)> functionInterface =
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
template <typename FunctionSignature>
class FunctionInterface
{
  template <typename OtherSignature>
  friend class FunctionInterface;

public:
  using Signature = FunctionSignature;

  VTKM_SUPPRESS_EXEC_WARNINGS
  FunctionInterface()
    : Result()
    , Parameters()
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  explicit FunctionInterface(const detail::ParameterContainer<FunctionSignature>& p)
    : Result()
    , Parameters(p)
  {
  }

  // the number of parameters as an integral constant
  using SigInfo = detail::FunctionSigInfo<FunctionSignature>;
  using SignatureArity = typename SigInfo::ArityType;
  using ResultType = typename SigInfo::ResultType;
  using ComponentSig = typename SigInfo::Components;
  using ParameterSig = typename SigInfo::Parameters;

  template <vtkm::IdComponent ParameterIndex>
  struct ParameterType
  {
    using type = typename detail::AtType<ParameterIndex, FunctionSignature>::type;
  };

  static constexpr bool RETURN_VALID = FunctionInterfaceReturnContainer<ResultType>::VALID;

  /// The number of parameters in this \c Function Interface.
  ///
  static constexpr vtkm::IdComponent ARITY = SigInfo::Arity;

  /// Returns the number of parameters held in this \c FunctionInterface. The
  /// return value is the same as \c ARITY.
  ///
  VTKM_EXEC_CONT
  vtkm::IdComponent GetArity() const { return ARITY; }

  /// Retrieves the return value from the last invocation called. This method
  /// will result in a compiler error if used with a function having a void
  /// return type.
  ///
  VTKM_EXEC_CONT
  ResultType GetReturnValue() const { return this->Result.Value; }

  /// Retrieves the return value from the last invocation wrapped in a \c
  /// FunctionInterfaceReturnContainer object. This call can succeed even if
  /// the return type is void. You still have to somehow check to make sure the
  /// return is non-void before trying to use it, but using this method can
  /// simplify templated programming.
  ///
  VTKM_EXEC_CONT
  const FunctionInterfaceReturnContainer<ResultType>& GetReturnValueSafe() const
  {
    return this->Result;
  }
  VTKM_EXEC_CONT
  FunctionInterfaceReturnContainer<ResultType>& GetReturnValueSafe() { return this->Result; }

  /// Gets the value for the parameter of the given index. Parameters are
  /// indexed starting at 1. To use this method you have to specify a static,
  /// compile time index. There are two ways to specify the index. The first is
  /// to specify a specific template parameter (e.g.
  /// <tt>GetParameter<1>()</tt>). Note that if you are using FunctionInterface
  /// within a template (which is almost always the case), then you will have
  /// to use the template keyword. For example, here is a simple implementation
  /// of a method that grabs the first parameter of FunctionInterface.
  ///
  /// \code{.cpp}
  /// template<FunctionSignature>
  /// void Foo(const vtkm::internal::FunctionInterface<FunctionSignature> &fInterface)
  /// {
  ///   bar(fInterface.template GetParameter<1>());
  /// }
  /// \endcode
  ///
  /// Alternatively the \c GetParameter method also has an optional argument
  /// that can be a \c IndexTag that specifies the parameter index. Here is
  /// a repeat of the previous example using this method.
  ///
  /// \code{.cpp}
  /// template<FunctionSignature>
  /// void Foo(const vtkm::internal::FunctionInterface<FunctionSignature> &fInterface)
  /// {
  ///   using vtkm::internal::IndexTag;
  ///   bar(fInterface.GetParameter(IndexTag<1>()));
  /// }
  /// \endcode
  ///
  template <vtkm::IdComponent ParameterIndex>
  VTKM_EXEC_CONT const typename ParameterType<ParameterIndex>::type& GetParameter(
    vtkm::internal::IndexTag<ParameterIndex> = vtkm::internal::IndexTag<ParameterIndex>()) const
  {
    return (detail::ParameterContainerAccess<ParameterIndex>()).Get(this->Parameters);
  }

  /// Sets the value for the parameter of the given index. Parameters are
  /// indexed starting at 1. To use this method you have to specify a static,
  /// compile time index. There are two ways to specify the index. The first is
  /// to specify a specific template parameter (e.g.
  /// <tt>SetParameter<1>(value)</tt>). Note that if you are using
  /// FunctionInterface within a template (which is almost always the case),
  /// then you will have to use the template keyword. For example, here is a
  /// simple implementation of a method that grabs the first parameter of
  /// FunctionInterface.
  ///
  /// \code{.cpp}
  /// template<FunctionSignature>
  /// void Foo(vtkm::internal::FunctionInterface<FunctionSignature> &fInterface)
  /// {
  ///   fInterface.template SetParameter<1>(bar);
  /// }
  /// \endcode
  ///
  /// Alternatively the \c GetParameter method also has an optional argument
  /// that can be a \c IndexTag that specifies the parameter index. Here is
  /// a repeat of the previous example using this method.
  ///
  /// \code{.cpp}
  /// template<FunctionSignature>
  /// void Foo(vtkm::internal::FunctionInterface<FunctionSignature> &fInterface)
  /// {
  ///   using vtkm::internal::IndexTag;
  ///   fInterface.SetParameter(bar, IndexTag<1>());
  /// }
  /// \endcode
  ///
  /// Sets the value for the parameter of the given index. Parameters are
  /// indexed starting at 1. To use this method you have to specify the index
  /// as a template parameter. If you are using FunctionInterface within a
  /// template (which is almost always the case), then you will have to use the
  /// template keyword.
  ///
  template <vtkm::IdComponent ParameterIndex>
  VTKM_EXEC_CONT void SetParameter(
    const typename ParameterType<ParameterIndex>::type& parameter,
    vtkm::internal::IndexTag<ParameterIndex> = vtkm::internal::IndexTag<ParameterIndex>())
  {
    return (detail::ParameterContainerAccess<ParameterIndex>()).Set(this->Parameters, parameter);
  }

  /// Copies the parameters and return values from the given \c
  /// FunctionInterface to this object. The types must be copiable from source
  /// to destination. If the number of parameters in the two objects are not
  /// the same, copies the first N arguments, where N is the smaller arity of
  /// the two function interfaces.
  ///
  template <typename SrcFunctionSignature>
  void Copy(const FunctionInterface<SrcFunctionSignature>& src)
  {
    this->Result = src.GetReturnValueSafe();

    constexpr vtkm::UInt16 minArity = (ARITY < FunctionInterface<SrcFunctionSignature>::ARITY)
      ? ARITY
      : FunctionInterface<SrcFunctionSignature>::ARITY;

    (detail::CopyAllParameters<minArity>()).Copy(this->Parameters, src.Parameters);
  }

  void Copy(const FunctionInterface<FunctionSignature>& src)
  { //optimized version for assignment/copy
    this->Result = src.GetReturnValueSafe();
    this->Parameters = src.Parameters;
  }

  /// Invoke a function \c f using the arguments stored in this
  /// FunctionInterface.
  ///
  /// If this FunctionInterface specifies a non-void return value, then the
  /// result of the function call is stored within this FunctionInterface and
  /// can be retrieved with GetReturnValue().
  ///
  template <typename Function>
  VTKM_CONT void InvokeCont(const Function& f)
  {
    detail::DoInvokeCont(f, this->Parameters, this->Result, detail::IdentityFunctor());
  }
  template <typename Function>
  VTKM_CONT void InvokeCont(Function& f)
  {
    detail::DoInvokeCont(f, this->Parameters, this->Result, detail::IdentityFunctor());
  }
  template <typename Function>
  VTKM_EXEC void InvokeExec(const Function& f)
  {
    detail::DoInvokeExec(f, this->Parameters, this->Result, detail::IdentityFunctor());
  }
  template <typename Function>
  VTKM_EXEC void InvokeExec(Function& f)
  {
    detail::DoInvokeExec(f, this->Parameters, this->Result, detail::IdentityFunctor());
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
  template <typename Function, typename TransformFunctor>
  VTKM_CONT void InvokeCont(const Function& f, const TransformFunctor& transform)
  {
    detail::DoInvokeCont(f, this->Parameters, this->Result, transform);
  }
  template <typename Function, typename TransformFunctor>
  VTKM_CONT void InvokeCont(Function& f, const TransformFunctor& transform)
  {
    detail::DoInvokeCont(f, this->Parameters, this->Result, transform);
  }
  template <typename Function, typename TransformFunctor>
  VTKM_EXEC void InvokeExec(const Function& f, const TransformFunctor& transform)
  {
    detail::DoInvokeExec(f, this->Parameters, this->Result, transform);
  }
  template <typename Function, typename TransformFunctor>
  VTKM_EXEC void InvokeExec(Function& f, const TransformFunctor& transform)
  {
    detail::DoInvokeExec(f, this->Parameters, this->Result, transform);
  }

  template <typename NewType>
  struct AppendType
  {
    using type = FunctionInterface<typename detail::AppendType<ComponentSig, NewType>::type>;
  };

  /// Returns a new \c FunctionInterface with all the parameters of this \c
  /// FunctionInterface and the given method argument appended to these
  /// parameters. The return type can be determined with the \c AppendType
  /// template.
  ///
  template <typename NewType>
  VTKM_CONT typename AppendType<NewType>::type Append(const NewType& newParameter) const
  {
    using AppendSignature = typename detail::AppendType<ComponentSig, NewType>::type;

    FunctionInterface<AppendSignature> appendedFuncInterface;
    appendedFuncInterface.Copy(*this);
    appendedFuncInterface.template SetParameter<ARITY + 1>(newParameter);
    return appendedFuncInterface;
  }

  template <vtkm::IdComponent ParameterIndex, typename NewType>
  struct ReplaceType
  {
    using type =
      FunctionInterface<typename detail::ReplaceType<ComponentSig, ParameterIndex, NewType>::type>;
  };

  /// Returns a new \c FunctionInterface with all the parameters of this \c
  /// FunctionInterface except that the parameter indexed at the template
  /// parameter \c ParameterIndex (also specified with the optional second
  /// argument) is replaced with the given argument. This method can be used in
  /// place of SetParameter when the parameter type changes. The return type
  /// can be determined with the \c ReplaceType template.
  /// Gets the value for the parameter of the given index. Parameters are
  /// indexed starting at 1. To use this method you have to specify a static,
  /// compile time index. There are two ways to specify the index. The first is
  /// to specify a specific template parameter (e.g.
  /// <tt>GetParameter<1>()</tt>). Note that if you are using FunctionInterface
  /// within a template (which is almost always the case), then you will have
  /// to use the template keyword. For example, here is a simple implementation
  /// of a method that grabs the first parameter of FunctionInterface.
  ///
  /// \code{.cpp}
  /// template<FunctionSignature>
  /// void Foo(const vtkm::internal::FunctionInterface<FunctionSignature> &fInterface)
  /// {
  ///   bar(fInterface.template GetParameter<1>());
  /// }
  /// \endcode
  ///
  /// Alternatively the \c GetParameter method also has an optional argument
  /// that can be a \c IndexTag that specifies the parameter index. Here is
  /// a repeat of the previous example using this method.
  ///
  /// \code{.cpp}
  /// template<FunctionSignature>
  /// void Foo(const vtkm::internal::FunctionInterface<FunctionSignature> &fInterface)
  /// {
  ///   using vtkm::internal::IndexTag;
  ///   bar(fInterface.GetParameter(IndexTag<1>()));
  /// }
  /// \endcode
  ///
  ///
  template <vtkm::IdComponent ParameterIndex, typename NewType>
  VTKM_CONT typename ReplaceType<ParameterIndex, NewType>::type Replace(
    const NewType& newParameter,
    vtkm::internal::IndexTag<ParameterIndex> = vtkm::internal::IndexTag<ParameterIndex>()) const
  {

    using ReplaceSigType =
      typename detail::ReplaceType<ComponentSig, ParameterIndex, NewType>::type;
    FunctionInterface<ReplaceSigType> replacedFuncInterface;

    detail::FunctionInterfaceMoveParameters<ParameterIndex - 1>::Move(
      replacedFuncInterface.Parameters, this->Parameters);

    replacedFuncInterface.template SetParameter<ParameterIndex>(newParameter);

    detail::FunctionInterfaceMoveParameters<ARITY - ParameterIndex, ParameterIndex + 1>::Move(
      replacedFuncInterface.Parameters, this->Parameters);
    return replacedFuncInterface;
  }

  template <typename Transform>
  struct StaticTransformType
  {
    using type = FunctionInterface<
      typename detail::FunctionInterfaceStaticTransformType<FunctionSignature, Transform>::type>;
  };

  /// \brief Transforms the \c FunctionInterface based on compile-time
  /// information.
  ///
  /// The \c StaticTransform methods transform all the parameters of this \c
  /// FunctionInterface to different types and values based on compile-time
  /// information. It operates by accepting a functor that two arguments. The
  /// first argument is the parameter to transform and the second argument is
  /// an \c IndexTag specifying the index of the parameter (which can be
  /// ignored in many cases). The functor's return value is the transformed
  /// value. The functor must also contain a templated struct name ReturnType
  /// with an internal type named \c type that defines the return type of the
  /// transform for a given input type and parameter index.
  ///
  /// The transformation is only applied to the parameters of the function. The
  /// return argument is unaffected.
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
  ///   template<typename T, vtkm::IdComponent Index>
  ///   struct ReturnType {
  ///     typedef const T *type;
  ///   };
  ///
  ///   template<typename T, vtkm::IdComponent Index>
  ///   VTKM_CONT
  ///   const T *operator()(const T &x, vtkm::internal::IndexTag<Index>) const {
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
  template <typename Transform>
  VTKM_CONT typename StaticTransformType<Transform>::type StaticTransformCont(
    const Transform& transform)
  {
    typename StaticTransformType<Transform>::type newFuncInterface;
    detail::DoStaticTransformCont(transform, this->Parameters, newFuncInterface.Parameters);
    return newFuncInterface;
  }
  template <typename Transform>
  VTKM_EXEC typename StaticTransformType<Transform>::type StaticTransformExec(
    const Transform& transform)
  {
    typename StaticTransformType<Transform>::type newFuncInterface;
    detail::DoStaticTransformExec(transform, this->Parameters, newFuncInterface.Parameters);
    return newFuncInterface;
  }

  /// \brief Transforms the \c FunctionInterface based on run-time information.
  ///
  /// The \c DynamicTransform method transforms all the parameters of this \c
  /// FunctionInterface to different types and values based on run-time
  /// information. It operates by accepting two functors. The first functor
  /// accepts three arguments. The first argument is a parameter to transform,
  /// the second is a functor to call with the transformed result, and the third
  /// is an instance of \c IndexTag denoting the index parameter..
  ///
  /// The second argument to \c DynamicTransform is another functor that
  /// accepts the transformed \c FunctionInterface and does something. If that
  /// transformed \c FunctionInterface has a return value, that return value
  /// will be passed back to this \c FunctionInterface.
  ///
  /// Here is a contrived but illustrative example. This transformation will
  /// pass all arguments except any string that looks like a number will be
  /// converted to a vtkm::FloatDefault. Note that because the types are not
  /// determined until runtime, this transform cannot be determined at compile
  /// time with meta-template programming.
  ///
  /// \code
  /// struct MyTransformFunctor {
  ///   template<typename InputType,
  ///            typename ContinueFunctor,
  ///            vtkm::IdComponent Index>
  ///   VTKM_CONT
  ///   void operator()(const InputType &input,
  ///                   const ContinueFunctor &continueFunc,
  ///                   vtkm::internal::IndexTag<Index>) const
  ///   {
  ///     continueFunc(input);
  ///   }
  ///
  ///   template<typename ContinueFunctor, vtkm::IdComponent Index>
  ///   VTKM_CONT
  ///   void operator()(const std::string &input,
  ///                   const ContinueFunctor &continueFunc,
  ///                   vtkm::internal::IndexTag<Index>) const
  ///   {
  ///     if ((input[0] >= '0' && (input[0] <= '9'))
  ///     {
  ///       std::stringstream stream(input);
  ///       vtkm::FloatDefault value;
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
  ///   VTKM_CONT
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
  template <typename TransformFunctor, typename FinishFunctor>
  VTKM_CONT void DynamicTransformCont(const TransformFunctor& transform,
                                      const FinishFunctor& finish) const
  {
    using ContinueFunctorType =
      detail::FunctionInterfaceDynamicTransformContContinue<FunctionSignature,
                                                            ResultType(),
                                                            TransformFunctor,
                                                            FinishFunctor>;

    FunctionInterface<ResultType()> emptyInterface;
    ContinueFunctorType continueFunctor =
      ContinueFunctorType(*this, emptyInterface, transform, finish);

    continueFunctor.DoNextTransform(emptyInterface);
    //    this->Result = emptyInterface.GetReturnValueSafe();
  }

  /// \brief Applies a function to all the parameters.
  ///
  /// The \c ForEach methods take a functor and apply that functor to each of
  /// the parameters in the \c FunctionInterface. (Return values are not
  /// effected.) The first argument of the functor is the parameter value and
  /// the second argument is an \c IndexTag, which can be used to identify the
  /// index of the parameter.
  ///
  template <typename Functor>
  VTKM_CONT void ForEachCont(const Functor& f) const
  {
    detail::DoForEachCont(f, this->Parameters);
  }
  template <typename Functor>
  VTKM_CONT void ForEachCont(const Functor& f)
  {
    detail::DoForEachCont(f, this->Parameters);
  }
  template <typename Functor>
  VTKM_EXEC void ForEachExec(const Functor& f) const
  {
    detail::DoForEachExec(f, this->Parameters);
  }
  template <typename Functor>
  VTKM_EXEC void ForEachExec(const Functor& f)
  {
    detail::DoForEachExec(f, this->Parameters);
  }

private:
  vtkm::internal::FunctionInterfaceReturnContainer<ResultType> Result;
  detail::ParameterContainer<FunctionSignature> Parameters;
};

namespace detail
{

template <typename OriginalFunction,
          typename NewFunction,
          typename TransformFunctor,
          typename FinishFunctor>
class FunctionInterfaceDynamicTransformContContinue
{
public:
  FunctionInterfaceDynamicTransformContContinue(
    const vtkm::internal::FunctionInterface<OriginalFunction>& originalInterface,
    vtkm::internal::FunctionInterface<NewFunction>& newInterface,
    const TransformFunctor& transform,
    const FinishFunctor& finish)
    : OriginalInterface(originalInterface)
    , NewInterface(newInterface)
    , Transform(transform)
    , Finish(finish)
  {
  }

  template <typename T>
  VTKM_CONT void operator()(const T& newParameter) const
  {
    using NewFSigComp = typename FunctionInterface<NewFunction>::ComponentSig;

    //Determine if we should do the next transform
    using appended = brigand::push_back<NewFSigComp, T>;
    using interfaceSig = typename detail::AsSigType<appended>::type;
    using NextInterfaceType = FunctionInterface<interfaceSig>;

    static constexpr std::size_t newArity = NextInterfaceType::ARITY;
    static constexpr std::size_t oldArity = detail::FunctionSigInfo<OriginalFunction>::Arity;
    using ShouldDoNextTransformType = std::integral_constant<bool, (newArity < oldArity)>;

    NextInterfaceType nextInterface = this->NewInterface.Append(newParameter);

    this->DoNextTransform(nextInterface, ShouldDoNextTransformType());
    this->NewInterface.GetReturnValueSafe() = nextInterface.GetReturnValueSafe();
  }

  template <typename NextFunction>
  void DoNextTransform(vtkm::internal::FunctionInterface<NextFunction>& nextInterface) const
  {
    using NextContinueType = FunctionInterfaceDynamicTransformContContinue<OriginalFunction,
                                                                           NextFunction,
                                                                           TransformFunctor,
                                                                           FinishFunctor>;
    NextContinueType nextContinue =
      NextContinueType(this->OriginalInterface, nextInterface, this->Transform, this->Finish);
    static constexpr vtkm::IdComponent Index =
      vtkm::internal::FunctionInterface<NextFunction>::ARITY + 1;
    vtkm::internal::IndexTag<Index> indexTag;
    this->Transform(this->OriginalInterface.GetParameter(indexTag), nextContinue, indexTag);
  }

private:
  template <typename NextFunction>
  void DoNextTransform(vtkm::internal::FunctionInterface<NextFunction>& nextInterface,
                       std::true_type) const
  {
    using NextContinueType = FunctionInterfaceDynamicTransformContContinue<OriginalFunction,
                                                                           NextFunction,
                                                                           TransformFunctor,
                                                                           FinishFunctor>;
    NextContinueType nextContinue =
      NextContinueType(this->OriginalInterface, nextInterface, this->Transform, this->Finish);
    static constexpr vtkm::IdComponent Index =
      vtkm::internal::FunctionInterface<NextFunction>::ARITY + 1;
    vtkm::internal::IndexTag<Index> indexTag;
    this->Transform(this->OriginalInterface.GetParameter(indexTag), nextContinue, indexTag);
  }

  template <typename NextFunction>
  void DoNextTransform(vtkm::internal::FunctionInterface<NextFunction>& nextInterface,
                       std::false_type) const
  {
    this->Finish(nextInterface);
  }

private:
  const vtkm::internal::FunctionInterface<OriginalFunction>& OriginalInterface;
  vtkm::internal::FunctionInterface<NewFunction>& NewInterface;
  const TransformFunctor& Transform;
  const FinishFunctor& Finish;

  void operator=(const FunctionInterfaceDynamicTransformContContinue<OriginalFunction,
                                                                     NewFunction,
                                                                     TransformFunctor,
                                                                     FinishFunctor>&) = delete;
};

} // namespace detail
}
} // namespace vtkm::internal

#include <vtkm/internal/FunctionInterfaceDetailPost.h>

#endif //vtk_m_internal_FunctionInterface_h
