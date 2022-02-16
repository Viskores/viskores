//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ZFPCompressor1D_h
#define vtk_m_filter_ZFPCompressor1D_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/zfp/ZFPCompressor1D.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/zfp/ZFPCompressor1D.h instead of vtkm/filter/ZFPCompressor1D.h.")
inline void ZFPCompressor1D_deprecated() {}

inline void ZFPCompressor1D_deprecated_warning()
{
  ZFPCompressor1D_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ZFPCompressor1D_h
