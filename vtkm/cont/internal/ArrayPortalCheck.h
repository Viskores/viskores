//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ArrayPortalCheck_h
#define vtk_m_cont_internal_ArrayPortalCheck_h

#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/internal/ArrayPortalHelpers.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// \brief A wrapper around an `ArrayPortal` that checks to status from the originating array.
///
/// After you get an `ArrayPortal` from an `ArrayHandle`, there is a possibility that the
/// `ArrayHandle` could change in a way that would invalidate the `ArrayPortal` (for example
/// freeing the memory). You could lock the `ArrayHandle`, but that encourages deadlocks.
/// Instead, this `ArrayPortal` keeps a reference to a flag from the `ArrayHandle` that tells
/// whether the memory referenced by the `ArrayPortal is still valid.
///
template <typename PortalType_>
class VTKM_ALWAYS_EXPORT ArrayPortalCheck : public PortalType_
{
  std::shared_ptr<bool> Valid;

  using Superclass = PortalType_;

public:
  template <typename... PortalArgs>
  VTKM_CONT ArrayPortalCheck(const std::shared_ptr<bool>& valid, PortalArgs&&... args)
    : Superclass(std::forward<PortalArgs>(args)...)
    , Valid(valid)
  {
  }

  VTKM_CONT ArrayPortalCheck()
    : Valid(new bool)
  {
    *this->Valid = false;
  }

  // Even though these do not do anything the default implementation does not, they are defined so
  // that the CUDA compiler does not try to compile it for devices and then fail because the
  // std::shared_ptr does not work on CUDA devices.
  VTKM_CONT ArrayPortalCheck(const ArrayPortalCheck& src)
    : Superclass(src)
    , Valid(src.Valid)
  {
  }
  VTKM_CONT ArrayPortalCheck(ArrayPortalCheck&& rhs)
    : Superclass(std::move(static_cast<Superclass&&>(rhs)))
    , Valid(std::move(rhs.Valid))
  {
  }

  VTKM_CONT ArrayPortalCheck& operator=(const ArrayPortalCheck& src)
  {
    this->Superclass::operator=(src);
    this->Valid = src.Valid;
    return *this;
  }

  VTKM_CONT ArrayPortalCheck& operator=(ArrayPortalCheck&& rhs)
  {
    this->Superclass::operator=(std::move(static_cast<Superclass&&>(rhs)));
    this->Valid = std::move(rhs.Valid);
    return *this;
  }

  VTKM_CONT ~ArrayPortalCheck() {}

  // The Get and Set methods are marked for execution environment even though they won't
  // work there. This is so that this class can be used in classes that work in both
  // control and execution environments without having to suppress warnings in them all.

  template <typename PT = Superclass,
            typename std::enable_if<vtkm::internal::PortalSupportsGets<PT>::value, int>::type = 0>
  VTKM_CONT typename Superclass::ValueType Get(vtkm::Id index) const
  {
    if (!(*this->Valid))
    {
      VTKM_LOG_F(vtkm::cont::LogLevel::Fatal,
                 "Attempted to read from an ArrayPortal whose data has been deleted.");
      return typename Superclass::ValueType{};
    }
    // Technically, this is not perfectly thread safe. It is possible that the check above
    // passed and then another thread has deleted the underlying array before we got here.
    // However, any case in which this portal is used where such a condition can possible
    // happen is a grevious error. In which case a different run is likely to give the
    // correct error and the problem can be fixed.
    return this->Superclass::Get(index);
  }

  template <typename PT = Superclass,
            typename std::enable_if<vtkm::internal::PortalSupportsGets3D<PT>::value, int>::type = 0>
  VTKM_CONT typename Superclass::ValueType Get(vtkm::Id3 index) const
  {
    if (!(*this->Valid))
    {
      VTKM_LOG_F(vtkm::cont::LogLevel::Fatal,
                 "Attempted to read from an ArrayPortal whose data has been deleted.");
      return typename Superclass::ValueType{};
    }
    // Technically, this is not perfectly thread safe. It is possible that the check above
    // passed and then another thread has deleted the underlying array before we got here.
    // However, any case in which this portal is used where such a condition can possible
    // happen is a grevious error. In which case a different run is likely to give the
    // correct error and the problem can be fixed.
    return this->Superclass::Get(index);
  }

  template <typename PT = Superclass,
            typename std::enable_if<vtkm::internal::PortalSupportsSets<PT>::value, int>::type = 0>
  VTKM_CONT void Set(vtkm::Id index, typename Superclass::ValueType value) const
  {
    if (!(*this->Valid))
    {
      VTKM_LOG_F(vtkm::cont::LogLevel::Fatal,
                 "Attempted to write to an ArrayPortal whose data has been deleted.");
      return;
    }
    // Technically, this is not perfectly thread safe. It is possible that the check above
    // passed and then another thread has deleted the underlying array before we got here.
    // However, any case in which this portal is used where such a condition can possible
    // happen is a grevious error. In which case a different run is likely to give the
    // correct error and the problem can be fixed.
    this->Superclass::Set(index, value);
  }

  template <typename PT = Superclass,
            typename std::enable_if<vtkm::internal::PortalSupportsSets3D<PT>::value, int>::type = 0>
  VTKM_CONT void Set(vtkm::Id3 index, typename Superclass::ValueType value) const
  {
    if (!(*this->Valid))
    {
      VTKM_LOG_F(vtkm::cont::LogLevel::Fatal,
                 "Attempted to write to an ArrayPortal whose data has been deleted.");
      return;
    }
    // Technically, this is not perfectly thread safe. It is possible that the check above
    // passed and then another thread has deleted the underlying array before we got here.
    // However, any case in which this portal is used where such a condition can possible
    // happen is a grevious error. In which case a different run is likely to give the
    // correct error and the problem can be fixed.
    this->Superclass::Set(index, value);
  }

  VTKM_CONT std::shared_ptr<bool> GetValidPointer() const { return this->Valid; }
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayPortalCheck_h
