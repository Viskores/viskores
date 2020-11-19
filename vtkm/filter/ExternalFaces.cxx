
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtkm_filter_ExternalFaces_cxx

#include <vtkm/filter/ExternalFaces.h>
#include <vtkm/filter/ExternalFaces.hxx>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
ExternalFaces::ExternalFaces()
  : vtkm::filter::FilterDataSet<ExternalFaces>()
  , CompactPoints(false)
  , Worklet()
{
  this->SetPassPolyData(true);
}

//-----------------------------------------------------------------------------
vtkm::cont::DataSet ExternalFaces::GenerateOutput(const vtkm::cont::DataSet& input,
                                                  vtkm::cont::CellSetExplicit<>& outCellSet)
{
  //This section of ExternalFaces is independent of any input so we can build it
  //into the vtkm_filter library

  //3. Check the fields of the dataset to see what kinds of fields are present so
  //   we can free the cell mapping array if it won't be needed.
  const vtkm::Id numFields = input.GetNumberOfFields();
  bool hasCellFields = false;
  for (vtkm::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    auto f = input.GetField(fieldIdx);
    hasCellFields = f.IsFieldCell();
  }

  if (!hasCellFields)
  {
    this->Worklet.ReleaseCellMapArrays();
  }

  //4. create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    this->Compactor.SetMergePoints(false);
    return this->Compactor.Execute(output);
  }
  else
  {
    return output;
  }
}

bool ExternalFaces::MapFieldOntoOutput(vtkm::cont::DataSet& result, const vtkm::cont::Field& field)
{
  if (field.IsFieldPoint())
  {
    if (this->CompactPoints)
    {
      return this->Compactor.MapFieldOntoOutput(result, field);
    }
    else
    {
      result.AddField(field);
      return true;
    }
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, this->Worklet.GetCellIdMap(), result);
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
template VTKM_FILTER_EXTRA_TEMPLATE_EXPORT vtkm::cont::DataSet ExternalFaces::DoExecute(
  const vtkm::cont::DataSet& inData,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault> policy);
}
}
