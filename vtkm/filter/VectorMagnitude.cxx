//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtkm_filter_VectorMagnitude_cxx

#include <vtkm/filter/VectorMagnitude.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
VectorMagnitude::VectorMagnitude()
  : vtkm::filter::FilterField<VectorMagnitude>()
  , Worklet()
{
  this->SetOutputFieldName("magnitude");
}

//-----------------------------------------------------------------------------
VTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(VectorMagnitude);
}
}
