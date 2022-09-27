//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_AmrArrays_h
#define vtk_m_filter_AmrArrays_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/multi_block/AmrArrays.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8, "Use vtkm/filter/multi_block/AmrArrays.h instead of vtkm/filter/AmrArrays.h.")
inline void AmrArrays_deprecated() {}

inline void AmrArrays_deprecated_warning()
{
  AmrArrays_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_AmrArrays_h
