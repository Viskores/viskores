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
/// really requires separate implementations for pre-C++11 and C++11 versions of
/// compilers. Thus, variadic template arguments are, at this point in time,
/// something to be avoided when possible. The intention of \c
/// FunctionInterface is to collect most of the variadic template code into one
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
/// Finally, there is a way to replace all of the parameters at
/// once. The \c StaticTransform methods take a transform functor that modifies
/// each of the parameters. See the documentation for this method for
/// details on how it is used.
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
    : Parameters()
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  explicit FunctionInterface(const detail::ParameterContainer<FunctionSignature>& p)
    : Parameters(p)
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

  /// The number of parameters in this \c Function Interface.
  ///
  static constexpr vtkm::IdComponent ARITY = SigInfo::Arity;

  /// Returns the number of parameters held in this \c FunctionInterface. The
  /// return value is the same as \c ARITY.
  ///
  VTKM_EXEC_CONT
  vtkm::IdComponent GetArity() const { return ARITY; }

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
    constexpr vtkm::UInt16 minArity = (ARITY < FunctionInterface<SrcFunctionSignature>::ARITY)
      ? ARITY
      : FunctionInterface<SrcFunctionSignature>::ARITY;

    (detail::CopyAllParameters<minArity>()).Copy(this->Parameters, src.Parameters);
  }

  void Copy(const FunctionInterface<FunctionSignature>& src)
  { //optimized version for assignment/copy
    this->Parameters = src.Parameters;
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

private:
  detail::ParameterContainer<FunctionSignature> Parameters;
};

//============================================================================
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

template <typename R, typename... Args>
FunctionInterface<R(Args...)> make_FunctionInterface(const Args&... args)
{
  detail::ParameterContainer<R(Args...)> container = { args... };
  return FunctionInterface<R(Args...)>{ container };
}
}
} // namespace vtkm::internal

#include <vtkm/internal/FunctionInterfaceDetailPost.h>

#endif //vtk_m_internal_FunctionInterface_h
