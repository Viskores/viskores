//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtkm_filter_Threshold_cxx
#include <vtkm/filter/Threshold.h>

#include <vtkm/filter/MapFieldPermutation.h>

namespace vtkm
{
namespace filter
{

bool Threshold::MapFieldOntoOutput(vtkm::cont::DataSet& result, const vtkm::cont::Field& field)
{
  if (field.IsFieldPoint() || field.IsFieldGlobal())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(field);
    return true;
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, this->Worklet.GetValidCellIds(), result);
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
VTKM_FILTER_COMMON_INSTANTIATE_EXECUTE_METHOD(Threshold);
}
}
