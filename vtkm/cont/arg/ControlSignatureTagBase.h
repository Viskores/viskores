//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_ControlSignatureTagBase_h
#define vtk_m_cont_arg_ControlSignatureTagBase_h

#include <vtkm/StaticAssert.h>
#include <vtkm/internal/ExportMacros.h>

#include <type_traits>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// \brief The base class for all tags used in a \c ControlSignature.
///
/// If a new \c ControlSignature tag is created, it must be derived from this
/// class in some way. This helps identify \c ControlSignature tags in the \c
/// VTKM_IS_CONTROL_SIGNATURE_TAG macro and allows checking the validity of a
/// \c ControlSignature.
///
/// In addition to inheriting from this base class, a \c ControlSignature tag
/// must define the following three typedefs: \c TypeCheckTag, \c TransportTag
/// and \c FetchTag.
///
struct ControlSignatureTagBase
{
};

namespace internal
{

template <typename ControlSignatureTag>
struct ControlSignatureTagCheck
{
  static constexpr bool Valid =
    std::is_base_of<vtkm::cont::arg::ControlSignatureTagBase, ControlSignatureTag>::value;
};

} // namespace internal

/// Checks that the argument is a proper tag for an \c ControlSignature. This
/// is a handy concept check when modifying tags or dispatching to make sure
/// that a template argument is actually an \c ControlSignature tag. (You can
/// get weird errors elsewhere in the code when a mistake is made.)
///
#define VTKM_IS_CONTROL_SIGNATURE_TAG(tag)                                                         \
  VTKM_STATIC_ASSERT_MSG(::vtkm::cont::arg::internal::ControlSignatureTagCheck<tag>::Valid,        \
                         "Provided a type that is not a valid ControlSignature tag.")
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_ControlSignatureTagBase_h
