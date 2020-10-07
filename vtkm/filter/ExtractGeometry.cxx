//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtkm_filter_ExtractGeometry_cxx
#include <vtkm/filter/ExtractGeometry.h>

#include <vtkm/filter/MapFieldPermutation.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
VTKM_FILTER_COMMON_EXPORT ExtractGeometry::ExtractGeometry()
  : vtkm::filter::FilterDataSet<ExtractGeometry>()
  , ExtractInside(true)
  , ExtractBoundaryCells(false)
  , ExtractOnlyBoundaryCells(false)
{
}

VTKM_FILTER_COMMON_EXPORT bool ExtractGeometry::MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                                                   const vtkm::cont::Field& field)
{
  if (field.IsFieldPoint())
  {
    result.AddField(field);
    return true;
  }
  else if (field.IsFieldCell())
  {
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = this->Worklet.GetValidCellIds();
    return vtkm::filter::MapFieldPermutation(field, permutation, result);
  }
  else if (field.IsFieldGlobal())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
VTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(ExtractGeometry);
}
} // namespace vtkm::filter
