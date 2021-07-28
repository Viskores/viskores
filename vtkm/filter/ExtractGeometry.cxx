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
#include <vtkm/filter/ExtractGeometry.hxx>

#include <vtkm/filter/MapFieldPermutation.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
ExtractGeometry::ExtractGeometry()
  : vtkm::filter::FilterDataSet<ExtractGeometry>()
  , ExtractInside(true)
  , ExtractBoundaryCells(false)
  , ExtractOnlyBoundaryCells(false)
{
}

bool ExtractGeometry::MapFieldOntoOutput(vtkm::cont::DataSet& result,
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
template VTKM_FILTER_COMMON_TEMPLATE_EXPORT vtkm::cont::DataSet ExtractGeometry::DoExecute(
  const vtkm::cont::DataSet& inData,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault> policy);
}
} // namespace vtkm::filter
