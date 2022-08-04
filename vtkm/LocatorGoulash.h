//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_LocatorGoulash_h
#define vtk_m_LocatorGoulash_h

#include <vtkm/exec/internal/Variant.h>

namespace vtkm
{

//Last cell object for each locator.
struct LastCellUniformGrid
{
};

struct LastCellRectilinearGrid
{
};

struct LastCellTwoLevel
{
  vtkm::Id CellId = -1;
  vtkm::Id LeafIdx = -1;
};

struct LastCellBoundingHierarchy
{
  vtkm::Id CellId = -1;
  vtkm::Id NodeIdx = -1;
};

using LastCellType = vtkm::exec::internal::Variant<LastCellUniformGrid,
                                                   LastCellRectilinearGrid,
                                                   LastCellTwoLevel,
                                                   LastCellBoundingHierarchy>;

} //namespace vtkm


#endif // vtk_m_LocatorGoulash_h
