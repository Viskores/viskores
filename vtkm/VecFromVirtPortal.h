//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_VecFromVirtPortal_h
#define vtk_m_VecFromVirtPortal_h

#include <vtkm/Types.h>

#include <vtkm/VecFromPortal.h>
#include <vtkm/internal/ArrayPortalVirtual.h>

namespace vtkm
{

/// \brief A short variable-length array from a window in an ArrayPortal.
///
/// The \c VecFromPortal class is a Vec-like class that holds an array portal
/// and exposes a small window of that portal as if it were a \c Vec.
///
template <typename T>
class VTKM_ALWAYS_EXPORT VecFromVirtPortal
{
  using RefType = vtkm::internal::ArrayPortalValueReference<vtkm::ArrayPortalRef<T>>;

public:
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  VecFromVirtPortal(const vtkm::ArrayPortalRef<T>* portal,
                    vtkm::IdComponent numComponents,
                    vtkm::Id offset)
    : Portal(portal)
    , NumComponents(numComponents)
    , Offset(offset)
  {
  }

  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfComponents() const { return this->NumComponents; }

  template <vtkm::IdComponent DestSize>
  VTKM_EXEC_CONT void CopyInto(vtkm::Vec<T, DestSize>& dest) const
  {
    vtkm::IdComponent numComponents = vtkm::Min(DestSize, this->NumComponents);
    for (vtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = this->Portal->Get(index + this->Offset);
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  RefType operator[](vtkm::IdComponent index) const
  {
    return RefType(*this->Portal, index + this->Offset);
  }

private:
  const vtkm::ArrayPortalRef<T>* Portal = nullptr;
  vtkm::IdComponent NumComponents = 0;
  vtkm::Id Offset = 0;
};
}
#endif
