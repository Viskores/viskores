//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ImageDifference_h
#define vtk_m_filter_ImageDifference_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/image_processing/ImageDifference.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/image_processing/ImageDifference.h instead of vtkm/filter/ImageDifference.h.")
inline void ImageDifference_deprecated() {}

inline void ImageDifference_deprecated_warning()
{
  ImageDifference_deprecated();
}

class VTKM_DEPRECATED(1.8, "Use vtkm::filter::image_processing::ImageDifference.") ImageDifference
  : public vtkm::filter::image_processing::ImageDifference
{
  using image_processing::ImageDifference::ImageDifference;
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ImageDifference_h
