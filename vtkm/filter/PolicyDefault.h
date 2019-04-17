//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_PolicyDefault_h
#define vtk_m_filter_PolicyDefault_h

#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{

struct PolicyDefault : vtkm::filter::PolicyBase<PolicyDefault>
{
  // Inherit defaults from PolicyBase
};
}
}

#endif //vtk_m_filter_PolicyDefault_h
