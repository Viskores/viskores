//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_CrossProduct_h
#define vtk_m_filter_CrossProduct_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/vector_calculus/CrossProduct.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/vector_calculus/CrossProduct.h instead of vtkm/filter/CrossProduct.h.")
inline void CrossProduct_deprecated() {}

inline void CrossProduct_deprecated_warning()
{
  CrossProduct_deprecated();
}

class VTKM_DEPRECATED(1.8, "Use vtkm::filter::vector_calculus::CrossProduct.") CrossProduct
  : public vtkm::filter::vector_calculus::CrossProduct
{
  using vector_calculus::CrossProduct::CrossProduct;
};

}
} // namespace vtkm::filter

#endif // vtk_m_filter_CrossProduct_h
