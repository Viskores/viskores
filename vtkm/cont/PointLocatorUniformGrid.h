//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_PointLocatorUniformGrid_h
#define vtk_m_cont_PointLocatorUniformGrid_h

#include <vtkm/Deprecated.h>

#include <vtkm/cont/PointLocatorSparseGrid.h>

namespace vtkm
{
namespace cont
{

struct VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(1.6, "Replaced with PointLocatorSparseGrid.")
  PointLocatorUniformGrid : vtkm::cont::PointLocatorSparseGrid
{
};

}
}

#endif //vtk_m_cont_PointLocatorUniformGrid_h
