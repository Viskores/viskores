//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_arg_TypeCheckTagExecObject_h
#define viskores_cont_arg_TypeCheckTagExecObject_h

#include <viskores/internal/ExportMacros.h>

#include <viskores/cont/arg/TypeCheck.h>

#include <viskores/cont/ExecutionObjectBase.h>

#include <type_traits>

namespace viskores
{
namespace cont
{
namespace arg
{

/// The ExecObject type check passes for any object that inherits from \c
/// ExecutionObjectBase and follows the conventions of that class.
///
struct TypeCheckTagExecObject
{
};

template <typename Type>
struct TypeCheck<TypeCheckTagExecObject, Type>
{
  static constexpr bool value = viskores::cont::internal::IsExecutionObjectBase<Type>::value;
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TypeCheckTagExecObject_h
