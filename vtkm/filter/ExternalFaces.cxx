
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
    return this->Compactor.Execute(output, PolicyDefault{});
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
VTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(ExternalFaces);
}
}
