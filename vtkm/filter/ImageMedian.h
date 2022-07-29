//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ImageMedian_h
#define vtk_m_filter_ImageMedian_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/image_processing/ImageMedian.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/image_processing/ImageMedian.h instead of vtkm/filter/ImageMedian.h.")
inline void ImageMedian_deprecated() {}

inline void ImageMedian_deprecated_warning()
{
  ImageMedian_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ImageMedian_h
