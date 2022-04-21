//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ImageConnectivity_h
#define vtk_m_filter_ImageConnectivity_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/connected_components/ImageConnectivity.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/connected_components/ImageConnectivity.h instead of "
                "vtkm/filter/ImageConnectivity.h.")
inline void ImageConnectivity_deprecated() {}

inline void ImageConnectivity_deprecated_warning()
{
  ImageConnectivity_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ImageConnectivity_h
