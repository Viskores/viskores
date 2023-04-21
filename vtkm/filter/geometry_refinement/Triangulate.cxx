//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Algorithm.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/geometry_refinement/Triangulate.h>
#include <vtkm/filter/geometry_refinement/worklet/Triangulate.h>

namespace
{
//-----------------------------------------------------------------------------
VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          const vtkm::worklet::Triangulate& worklet)
{
  if (field.IsPointField())
  {
    // point data is copied as is because it was not collapsed
    result.AddField(field);
    return true;
  }
  else if (field.IsCellField())
  {
    // cell data must be scattered to the cells created per input cell
    vtkm::cont::ArrayHandle<vtkm::Id> permutation =
      worklet.GetOutCellScatter().GetOutputToInputMap();
    return vtkm::filter::MapFieldPermutation(field, permutation, result);
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

struct IsShapeTriangle
{
  VTKM_EXEC_CONT
  bool operator()(vtkm::UInt8 shape) const { return shape == vtkm::CELL_SHAPE_TRIANGLE; }
};

struct BinaryAnd
{
  VTKM_EXEC_CONT
  bool operator()(bool u, bool v) const { return u && v; }
};
} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{
VTKM_CONT vtkm::cont::DataSet Triangulate::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::UnknownCellSet& inCellSet = input.GetCellSet();

  // In case we already have a CellSetSingleType of tetras,
  // don't call the worklet and return the input DataSet directly
  if (inCellSet.CanConvert<vtkm::cont::CellSetSingleType<>>() &&
      inCellSet.AsCellSet<vtkm::cont::CellSetSingleType<>>().GetCellShapeAsId() ==
        vtkm::CellShapeTagTriangle::Id)
  {
    return input;
  }

  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::cont::DataSet output;

  // Optimization in case we only have triangles in the CellSet
  bool allTriangles = false;
  if (inCellSet.CanConvert<vtkm::cont::CellSetExplicit<>>())
  {
    vtkm::cont::CellSetExplicit<> inCellSetExplicit =
      inCellSet.AsCellSet<vtkm::cont::CellSetExplicit<>>();

    auto shapeArray = inCellSetExplicit.GetShapesArray(vtkm::TopologyElementTagCell(),
                                                       vtkm::TopologyElementTagPoint());
    auto isCellTriangleArray = vtkm::cont::make_ArrayHandleTransform(shapeArray, IsShapeTriangle{});
    allTriangles = vtkm::cont::Algorithm::Reduce(isCellTriangleArray, true, BinaryAnd{});

    if (allTriangles)
    {
      // Reuse the input's connectivity array
      outCellSet.Fill(inCellSet.GetNumberOfPoints(),
                      vtkm::CellShapeTagTriangle::Id,
                      3,
                      inCellSetExplicit.GetConnectivityArray(vtkm::TopologyElementTagCell(),
                                                             vtkm::TopologyElementTagPoint()));

      // Copy all fields from the input
      output = this->CreateResult(input, outCellSet, [&](auto& result, const auto& f) {
        result.AddField(f);
        return true;
      });
    }
  }

  if (!allTriangles)
  {
    vtkm::worklet::Triangulate worklet;
    vtkm::cont::CastAndCall(inCellSet,
                            [&](const auto& concrete) { outCellSet = worklet.Run(concrete); });

    auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
    // create the output dataset (without a CoordinateSystem).
    output = this->CreateResult(input, outCellSet, mapper);
  }

  // We did not change the geometry of the input dataset at all. Just attach coordinate system
  // of input dataset to output dataset.
  for (vtkm::IdComponent coordSystemId = 0; coordSystemId < input.GetNumberOfCoordinateSystems();
       ++coordSystemId)
  {
    output.AddCoordinateSystem(input.GetCoordinateSystem(coordSystemId));
  }
  return output;
}
} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm
