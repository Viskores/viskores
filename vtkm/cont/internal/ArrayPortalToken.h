//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ArrayPortalToken_h
#define vtk_m_cont_internal_ArrayPortalToken_h

#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/Token.h>

#include <vtkm/internal/ArrayPortalHelpers.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// \brief A wrapper around an `ArrayPortal` that also holds its state with a token.
///
/// Usually when you get an `ArrayPortal`, you have to associate it with a `Token`
/// object to define its scope. This class wraps around an `ArrayPortal` and also
/// holds its own `Token` that should be attached appropriately to the source
/// `ArrayHandle`. When this class goes out of scope, so will its `Token`.
///
/// Because `Token`s only work in the control environment, so it is for this class.
///
template <typename PortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalToken : public PortalType
{
  std::shared_ptr<vtkm::cont::Token> Token;

public:
  template <typename... PortalArgs>
  VTKM_CONT ArrayPortalToken(vtkm::cont::Token&& token, PortalArgs&... args)
    : PortalType(std::forward<PortalArgs>(args)...)
    , Token(new vtkm::cont::Token(std::move(token)))
  {
  }

  template <typename... PortalArgs>
  VTKM_CONT ArrayPortalToken(PortalArgs&... args)
    : PortalType(std::forward<PortalArgs>(args)...)
    , Token(new vtkm::cont::Token)
  {
  }

  VTKM_CONT void Detach() const
  {
    this->Token.DetachFromAll();
    this->Portal = PortalType();
  }
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayPortalToken_h
