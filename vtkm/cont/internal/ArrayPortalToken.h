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
template <typename PortalType_>
class VTKM_ALWAYS_EXPORT ArrayPortalToken : public PortalType_
{
  std::shared_ptr<vtkm::cont::Token> Token;

  using Superclass = PortalType_;

public:
  template <typename... PortalArgs>
  VTKM_CONT ArrayPortalToken(vtkm::cont::Token&& token, PortalArgs&&... args)
    : Superclass(std::forward<PortalArgs>(args)...)
    , Token(new vtkm::cont::Token(std::move(token)))
  {
  }

  template <typename... PortalArgs>
  VTKM_CONT ArrayPortalToken(std::shared_ptr<vtkm::cont::Token> token, PortalArgs&&... args)
    : Superclass(std::forward<PortalArgs>(args)...)
    , Token(token)
  {
  }

  template <typename... PortalArgs>
  VTKM_CONT ArrayPortalToken(PortalArgs&&... args)
    : Superclass(std::forward<PortalArgs>(args)...)
    , Token(new vtkm::cont::Token)
  {
  }

  // Even though these do not do anything the default implementation does not, they are defined so
  // that the CUDA compiler does not try to compile it for devices and then fail because the
  // std::shared_ptr does not work on CUDA devices.
  VTKM_CONT ArrayPortalToken(const ArrayPortalToken& src)
    : Superclass(src)
    , Token(src.Token)
  {
  }
  VTKM_CONT ArrayPortalToken(ArrayPortalToken&& rhs)
    : Superclass(std::move(static_cast<Superclass&&>(rhs)))
    , Token(std::move(rhs.Token))
  {
  }
  VTKM_CONT ArrayPortalToken& operator=(const ArrayPortalToken& src)
  {
    this->Superclass::operator=(src);
    this->Token = src.Token;
    return *this;
  }
  VTKM_CONT ArrayPortalToken& operator=(ArrayPortalToken&& rhs)
  {
    this->Superclass::operator=(std::move(static_cast<Superclass&&>(rhs)));
    this->Token = std::move(rhs.Token);
    return *this;
  }
  VTKM_CONT ~ArrayPortalToken() {}

  /// \brief Detach this portal from the `ArrayHandle`.
  ///
  /// This will open up the `ArrayHandle` for reading and/or writing.
  ///
  VTKM_CONT void Detach()
  {
    this->Token->DetachFromAll();

    // Reset this portal in case the superclass is holding other array portals with their own
    // tokens. Detach is supposed to invalidate the array portal, so it is OK to do this.
    *this = ArrayPortalToken<PortalType_>();
  }

  /// \brief Get the `Token` of the `ArrayPortal`.
  ///
  /// You can keep a copy of this shared pointer to keep the scope around longer than this
  /// object exists (unless, of course, the `Token` is explicitly detached).
  ///
  VTKM_CONT std::shared_ptr<vtkm::cont::Token> GetToken() const { return this->Token; }
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayPortalToken_h
