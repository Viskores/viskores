//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_internal_Placeholders_h
#define vtk_m_worklet_internal_Placeholders_h

#include <vtkmtaotuple/include/Tuple.h>
#include <vtkmtaotuple/include/tao/seq/make_integer_sequence.hpp>

#include <type_traits>

#include <vtkm/exec/arg/BasicArg.h>

namespace vtkm
{
namespace placeholders
{

//============================================================================
template <int ControlSignatureIndex>
struct Arg : vtkm::exec::arg::BasicArg<ControlSignatureIndex>
{
};

//============================================================================
/**
* Type that computes the number of parameters to the given function signature
*/
template <typename>
struct FunctionSigArity;
template <typename R, typename... ArgTypes>
struct FunctionSigArity<R(ArgTypes...)>
{
  static constexpr std::size_t value = sizeof...(ArgTypes);
};

//============================================================================
template <int... Args>
auto DefaultSigGenerator(tao::seq::integer_sequence<int, 0, Args...>) -> void (*)(Arg<Args>...);

/**
* Given a desired length will generate the default/assumed ExecutionSignature.
*
* So if you want the ExecutionSignature for a function that has 2 parameters this
* would generate a `type` that is comparable to the user writing:
*
* using ExecutionSignature = void(_1, _2);
*
*/
template <int Length>
struct DefaultExecSig
{
  using seq = tao::seq::make_integer_sequence<int, Length + 1>;
  using type = typename std::remove_pointer<decltype(DefaultSigGenerator(seq{}))>::type;
};
template <>
struct DefaultExecSig<1>
{
  using type = void(Arg<1>);
};
template <>
struct DefaultExecSig<2>
{
  using type = void(Arg<1>, Arg<2>);
};
template <>
struct DefaultExecSig<3>
{
  using type = void(Arg<1>, Arg<2>, Arg<3>);
};
template <>
struct DefaultExecSig<4>
{
  using type = void(Arg<1>, Arg<2>, Arg<3>, Arg<4>);
};

//============================================================================
/**
* Given a worklet this will produce a typedef `ExecutionSignature` that is
* the ExecutionSignature of the worklet, even if the worklet itself doesn't
* have said typedef.
*
* Logic this class uses:
*
* 1. If the `WorkletType` has a typedef named `ExecutionSignature` use that
* 2. If no typedef exists, generate one!
*   - Presume the Worklet has a `void` return type, and each ControlSignature
*    argument is passed to the worklet in the same listed order.
*   - Generate this assumed `ExecutionSignature` by using  `DefaultExecSig`
*
*/
template <typename WorkletType>
struct GetExecSig
{
  template <typename U, typename S = decltype(std::declval<typename U::ExecutionSignature>())>
  static vtkmstd::tuple<std::true_type, typename U::ExecutionSignature> get_exec_sig(int);

  template <typename U>
  static vtkmstd::tuple<std::false_type, std::false_type> get_exec_sig(...);

  using cont_sig = typename WorkletType::ControlSignature;
  using cont_sig_info = vtkm::placeholders::FunctionSigArity<cont_sig>;

  using result = decltype(get_exec_sig<WorkletType>(0));
  using has_explicit_exec_sig = typename vtkmstd::tuple_element<0, result>::type;

  using ExecutionSignature = typename std::conditional<
    has_explicit_exec_sig::value,
    typename vtkmstd::tuple_element<1, result>::type,
    typename vtkm::placeholders::DefaultExecSig<cont_sig_info::value>::type>::type;
};
}
}

#endif
