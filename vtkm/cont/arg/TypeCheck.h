//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TypeCheck_h
#define vtk_m_cont_arg_TypeCheck_h

namespace vtkm
{
namespace cont
{
namespace arg
{

/// \brief Class for checking that a type matches the semantics for an argument.
///
/// The \c TypeCheck class is used in dispatchers to test whether an argument
/// passed to the \c Invoke command matches the corresponding argument in the
/// \c ControlSignature.
///
/// This check happens after casting dynamic classes to static classes, so the
/// check need not worry about querying dynamic types.
///
/// The generic implementation of \c TypeCheck always results in failure. When
/// a new type check tag is defined, along with it should be partial
/// specializations that find valid types.
///
template <typename TypeCheckTag, typename Type>
struct TypeCheck
{
  /// The static constant boolean \c value is set to \c true if the type is
  /// valid for the given check tag and \c false otherwise.
  ///
  static constexpr bool value = false;
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheck_h
