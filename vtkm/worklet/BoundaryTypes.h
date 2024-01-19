//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_BoundaryTypes_h
#define vtk_m_worklet_BoundaryTypes_h

#include <vtkm/Deprecated.h>

namespace vtkm
{
namespace worklet
{
/// \brief Clamps boundary values to the nearest valid i,j,k value
///
/// BoundaryClamp always returns the nearest valid i,j,k value when at an
/// image boundary. This is a commonly used when solving differential equations.
///
/// For example, when used with WorkletCellNeighborhood3x3x3 when centered
/// on the point 1:
/// \code
///               * * *
///               * 1 2 (where * denotes points that lie outside of the image boundary)
///               * 3 5
/// \endcode
/// returns the following neighborhood of values:
/// \code
///              1 1 2
///              1 1 2
///              3 3 5
/// \endcode
struct VTKM_DEPRECATED(2.2, "Never fully supported, so being removed.") BoundaryClamp
{
};
}
}
#endif //vtk_m_worklet_BoundaryTypes_h
