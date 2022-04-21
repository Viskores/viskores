//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_CellSetConnectivity_h
#define vtk_m_filter_CellSetConnectivity_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/connected_components/CellSetConnectivity.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/connected_components/CellSetConnectivity.h instead of "
                "vtkm/filter/CellSetConnectivity.h.")
inline void CellSetConnectivity_deprecated() {}

inline void CellSetConnectivity_deprecated_warning()
{
  CellSetConnectivity_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_CellSetConnectivity_h
