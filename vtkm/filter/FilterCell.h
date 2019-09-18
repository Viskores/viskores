//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_CellFilter_h
#define vtk_m_filter_CellFilter_h

#include <vtkm/filter/FilterField.h>

namespace vtkm
{
namespace filter
{

template <class Derived>
using FilterCell = vtkm::filter::FilterField<Derived>;
}
}
#endif // vtk_m_filter_CellFilter_h
