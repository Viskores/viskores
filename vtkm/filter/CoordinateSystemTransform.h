//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_CoordinateSystemTransform_h
#define vtk_m_filter_CoordinateSystemTransform_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/field_transform/CoordinateSystemTransform.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/field_transform/CoordinateSystemTransform.h instead of "
                "vtkm/filter/CoordinateSystemTransform.h.")
inline void CoordinateSystemTransform_deprecated() {}

inline void CoordinateSystemTransform_deprecated_warning()
{
  CoordinateSystemTransform_deprecated();
}

class VTKM_DEPRECATED(1.8, "Use vtkm::filter::field_transform::CoordinateSystemTransform.")
  CoordinateSystemTransform : public vtkm::filter::field_transform::CoordinateSystemTransform
{
  using field_transform::CoordinateSystemTransform::CoordinateSystemTransform;
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_CoordinateSystemTransform_h
