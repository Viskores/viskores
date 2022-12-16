//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_CellClassification_h
#define vtk_m_CellClassification_h

#include <vtkm/Types.h>

namespace vtkm
{

/// \brief Bit flags used in ghost arrays to identify what type a cell is.
///
/// `CellClassification` contains several bit flags that determine whether a cell is
/// normal or if it should be treated as duplicated or removed in some way. The flags
/// can be (and should be) treated as `vtkm::UInt8` and or-ed together.
///
class CellClassification
{
  // Implementation note: An enum would be a natural representation for these flags.
  // However, a non-scoped enum leaks the names into the broader namespace and a
  // scoped enum is too difficult to convert to the `vtkm::UInt8` we really want to
  // treat it as. Thus, use constexpr to define the `vtkm::UInt8`s.
  vtkm::UInt8 Flags;

public:
  // Use an unscoped enum here, where it will be properly scoped in the class.
  // Using unscoped enums in this way is sort of obsolete, but prior to C++17
  // a `static constexpr` may require a definition in a .cxx file, and that is
  // not really possible for this particular class (which could be used on a GPU).
  enum : vtkm::UInt8
  {
    Normal = 0,       //Valid cell
    Ghost = 1 << 0,   //Ghost cell
    Invalid = 1 << 1, //Cell is invalid
    Unused0 = 1 << 2,
    Blanked = 1 << 3, //Blanked cell in AMR
    Unused3 = 1 << 4,
    Unused4 = 1 << 5,
    Unused5 = 1 << 6,
  };

  VTKM_EXEC constexpr CellClassification(vtkm::UInt8 flags = vtkm::UInt8{ Normal })
    : Flags(flags)
  {
  }

  VTKM_EXEC constexpr operator vtkm::UInt8() const { return this->Flags; }
};

} // namespace vtkm

#endif // vtk_m_CellClassification_h
