//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_ArrayPortalDummy
#define vtk_m_internal_ArrayPortalDummy

#include <vtkm/Assert.h>
#include <vtkm/Types.h>

namespace vtkm
{
namespace internal
{

/// A class that can be used in place of an `ArrayPortal` when the `ArrayPortal` is
/// not actually supported. It allows templates to be compiled, but will cause undefined
/// behavior if actually used.
template <typename T>
struct ArrayPortalDummy
{
  using ValueType = T;

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const { return 0; }

  VTKM_EXEC_CONT ValueType Get(vtkm::Id) const
  {
    VTKM_ASSERT(false && "Tried to use a dummy portal.");
    return ValueType{};
  }
};

}
} // namespace vtkm::internal

#endif //vtk_m_internal_ArrayPortalDummy
