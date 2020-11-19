//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtkm_filter_ExtractStructured_cxx
#include <vtkm/filter/ExtractStructured.h>
#include <vtkm/filter/ExtractStructured.hxx>

#include <vtkm/filter/MapFieldPermutation.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
ExtractStructured::ExtractStructured()
  : vtkm::filter::FilterDataSet<ExtractStructured>()
  , VOI(vtkm::RangeId3(0, -1, 0, -1, 0, -1))
  , SampleRate(vtkm::Id3(1, 1, 1))
  , IncludeBoundary(false)
  , IncludeOffset(false)
  , Worklet()
{
}

//-----------------------------------------------------------------------------
bool ExtractStructured::MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                           const vtkm::cont::Field& field)
{
  if (field.IsFieldPoint())
  {
    return vtkm::filter::MapFieldPermutation(field, this->PointFieldMap, result);
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, this->CellFieldMap, result);
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
void ExtractStructured::PostExecute(const vtkm::cont::PartitionedDataSet&,
                                    vtkm::cont::PartitionedDataSet&)
{
  this->CellFieldMap.ReleaseResources();
  this->PointFieldMap.ReleaseResources();
}

//-----------------------------------------------------------------------------
template VTKM_FILTER_COMMON_TEMPLATE_EXPORT vtkm::cont::DataSet ExtractStructured::DoExecute(
  const vtkm::cont::DataSet& inData,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault> policy);
}
}
