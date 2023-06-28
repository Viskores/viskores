//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/geometry_refinement/ConvertToPointCloud.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetSingleType.h>

#include <vtkm/CellShape.h>

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{

vtkm::cont::DataSet ConvertToPointCloud::DoExecute(const vtkm::cont::DataSet& input)
{
  vtkm::Id numPoints = input.GetNumberOfPoints();

  // A connectivity array for a point cloud is easy. All the cells are a vertex with exactly
  // one point. So, it can be represented a simple index array (i.e., 0, 1, 2, 3, ...).
  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex{ numPoints }, connectivity);

  vtkm::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, connectivity);

  auto fieldMapper = [&](vtkm::cont::DataSet& outData, vtkm::cont::Field& field) {
    if (field.IsCellField())
    {
      // Cell fields are dropped.
      return;
    }
    else if (this->AssociateFieldsWithCells && field.IsPointField() &&
             !input.HasCoordinateSystem(field.GetName()))
    {
      // The user asked to convert point fields to cell fields. (They are interchangable in
      // point clouds.)
      outData.AddCellField(field.GetName(), field.GetData());
    }
    else
    {
      outData.AddField(field);
    }
  };
  return this->CreateResult(input, cellSet, fieldMapper);
}

}
}
} // namespace vtkm::filter::geometry_refinement
