//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_Bitset_h
#define vtk_m_Bitset_h

#include <assert.h>
#include <limits>
#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{

/// \brief A bitmap to serve different needs.
/// Ex. Editing particular bits in a byte(s), checkint if particular bit values
/// are present or not. Once Cuda supports std::bitset, we should use the
/// standard one if possible
template <typename MaskType>
struct Bitset
{
  VTKM_EXEC_CONT void set(vtkm::Id bitIndex)
  {
    this->Mask = this->Mask | (static_cast<MaskType>(1) << bitIndex);
  }

  VTKM_EXEC_CONT void reset(vtkm::Id bitIndex)
  {
    this->Mask = this->Mask & ~(static_cast<MaskType>(1) << bitIndex);
  }

  VTKM_EXEC_CONT void toggle(vtkm::Id bitIndex)
  {
    this->Mask = this->Mask ^ (static_cast<MaskType>(0) << bitIndex);
  }

  VTKM_EXEC_CONT bool test(vtkm::Id bitIndex)
  {
    return ((this->Mask & (static_cast<MaskType>(1) << bitIndex)) != 0);
  }

private:
  MaskType Mask = 0;
};

} // namespace vtkm

#endif //vtk_m_Bitset_h
