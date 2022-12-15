//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/UncertainCellSet.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/geometry_refinement/VertexClustering.h>
#include <vtkm/filter/geometry_refinement/worklet/VertexClustering.h>

namespace
{
VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          const vtkm::worklet::VertexClustering& worklet)
{
  if (field.IsPointField())
  {
    return vtkm::filter::MapFieldPermutation(field, worklet.GetPointIdMap(), result);
  }
  else if (field.IsCellField())
  {
    return vtkm::filter::MapFieldPermutation(field, worklet.GetCellIdMap(), result);
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}
} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{
VTKM_CONT vtkm::cont::DataSet VertexClustering::DoExecute(const vtkm::cont::DataSet& input)
{
  //need to compute bounds first
  vtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  auto inCellSet = input.GetCellSet().ResetCellSetList<VTKM_DEFAULT_CELL_SET_LIST_UNSTRUCTURED>();
  vtkm::cont::UnknownCellSet outCellSet;
  vtkm::cont::UnknownArrayHandle outCoords;
  vtkm::worklet::VertexClustering worklet;
  worklet.Run(inCellSet,
              input.GetCoordinateSystem(),
              bounds,
              this->GetNumberOfDivisions(),
              outCellSet,
              outCoords);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResultCoordinateSystem(
    input, outCellSet, input.GetCoordinateSystem().GetName(), outCoords, mapper);
}
} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm
