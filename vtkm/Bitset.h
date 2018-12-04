//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
  VTKM_EXEC_CONT Bitset()
    : Mask(0)
  {
  }

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
  MaskType Mask;
};

} // namespace vtkm

#endif //vtk_m_Bitset_h
