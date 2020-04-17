
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_contour_flyingedges_h
#define vtk_m_worklet_contour_flyingedges_h

#include <vtkm/worklet/contour/FlyingEdgesHelpers.h>
#include <vtkm/worklet/contour/FlyingEdgesPass1.h>
#include <vtkm/worklet/contour/FlyingEdgesPass2.h>
#include <vtkm/worklet/contour/FlyingEdgesPass4.h>
#include <vtkm/worklet/contour/FlyingEdgesPass5.h>

#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/Invoker.h>

namespace vtkm
{
namespace worklet
{
namespace flying_edges
{

namespace detail
{
inline vtkm::cont::CellSetStructured<3> make_metaDataMesh3D(SumXAxis, const vtkm::Id3& pdims)
{
  vtkm::cont::CellSetStructured<3> metaDataMesh;
  metaDataMesh.SetPointDimensions(vtkm::Id3{ pdims[1], pdims[2], 1 });
  return metaDataMesh;
}
inline vtkm::cont::CellSetStructured<2> make_metaDataMesh2D(SumXAxis, const vtkm::Id3& pdims)
{
  vtkm::cont::CellSetStructured<2> metaDataMesh;
  metaDataMesh.SetPointDimensions(vtkm::Id2{ pdims[1], pdims[2] });
  return metaDataMesh;
}

inline vtkm::cont::CellSetStructured<3> make_metaDataMesh3D(SumYAxis, const vtkm::Id3& pdims)
{
  vtkm::cont::CellSetStructured<3> metaDataMesh;
  metaDataMesh.SetPointDimensions(vtkm::Id3{ pdims[0], pdims[2], 1 });
  return metaDataMesh;
}
inline vtkm::cont::CellSetStructured<2> make_metaDataMesh2D(SumYAxis, const vtkm::Id3& pdims)
{
  vtkm::cont::CellSetStructured<2> metaDataMesh;
  metaDataMesh.SetPointDimensions(vtkm::Id2{ pdims[0], pdims[2] });
  return metaDataMesh;
}

template <typename T, typename S>
vtkm::Id extend_by(vtkm::cont::ArrayHandle<T, S>& handle, vtkm::Id size)
{
  vtkm::Id oldLen = handle.GetNumberOfValues();
  if (oldLen == 0)
  {
    handle.Allocate(size);
  }
  else
  {
    vtkm::cont::ArrayHandle<T, S> tempHandle;
    tempHandle.Allocate(oldLen + size);
    vtkm::cont::Algorithm::CopySubRange(handle, 0, oldLen, tempHandle);
    handle = tempHandle;
  }
  return oldLen;
}
}


//----------------------------------------------------------------------------
template <typename ValueType,
          typename StorageTagField,
          typename StorageTagVertices,
          typename StorageTagNormals,
          typename CoordinateType,
          typename NormalType>
vtkm::cont::CellSetSingleType<> execute(
  const vtkm::cont::CellSetStructured<3>& cells,
  const vtkm::cont::ArrayHandleUniformPointCoordinates& coordinateSystem,
  const std::vector<ValueType>& isovalues,
  const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
  vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices>& points,
  vtkm::cont::ArrayHandle<vtkm::Vec<NormalType, 3>, StorageTagNormals>& normals,
  vtkm::worklet::contour::CommonState& sharedState)
{
  //Tasks:
  //2. Refactor how we map fields.
  //   We need the ability unload everything in SharedState after
  //   we have mapped all fields
  //3. Support switching AxisToSum by running this whole thing in a TryExecute
  //   Passes 5 can ignore this

  using AxisToSum = SumXAxis;

  vtkm::cont::Invoker invoke;

  auto pdims = cells.GetPointDimensions();

  vtkm::cont::ArrayHandle<vtkm::UInt8> edgeCases;
  edgeCases.Allocate(coordinateSystem.GetNumberOfValues());

  vtkm::cont::CellSetStructured<3> metaDataMesh3D = detail::make_metaDataMesh3D(AxisToSum{}, pdims);
  vtkm::cont::CellSetStructured<2> metaDataMesh2D = detail::make_metaDataMesh2D(AxisToSum{}, pdims);

  vtkm::cont::ArrayHandle<vtkm::Id> metaDataLinearSums; //per point of metaDataMesh
  vtkm::cont::ArrayHandle<vtkm::Id> metaDataMin;        //per point of metaDataMesh
  vtkm::cont::ArrayHandle<vtkm::Id> metaDataMax;        //per point of metaDataMesh
  vtkm::cont::ArrayHandle<vtkm::Int32> metaDataNumTris; //per cell of metaDataMesh

  auto metaDataSums = vtkm::cont::make_ArrayHandleGroupVec<3>(metaDataLinearSums);

  // Since sharedState can be re-used between invocations of contour,
  // we need to make sure we reset the size of the Interpolation
  // arrays so we don't execute Pass5 over an array that is too large
  sharedState.InterpolationEdgeIds.Shrink(0);
  sharedState.InterpolationWeights.Shrink(0);
  sharedState.CellIdMap.Shrink(0);

  vtkm::cont::ArrayHandle<vtkm::Id> triangle_topology;
  for (std::size_t i = 0; i < isovalues.size(); ++i)
  {
    auto multiContourCellOffset = sharedState.CellIdMap.GetNumberOfValues();
    auto multiContourPointOffset = sharedState.InterpolationWeights.GetNumberOfValues();
    ValueType isoval = isovalues[i];

    //----------------------------------------------------------------------------
    // PASS 1: Process all of the voxel edges that compose each row. Determine the
    // edges case classification, count the number of edge intersections, and
    // figure out where intersections along the row begins and ends
    // (i.e., gather information for computational trimming).
    //
    // We mark everything as below as it is faster than having the worklet to it
    vtkm::cont::Algorithm::Fill(edgeCases, static_cast<vtkm::UInt8>(FlyingEdges3D::Below));
    ComputePass1<ValueType, AxisToSum> worklet1(isoval, pdims);
    invoke(worklet1, metaDataMesh3D, metaDataSums, metaDataMin, metaDataMax, edgeCases, inputField);

    //----------------------------------------------------------------------------
    // PASS 2: Process a single row of voxels/cells. Count the number of other
    // axis intersections by topological reasoning from previous edge cases.
    // Determine the number of primitives (i.e., triangles) generated from this
    // row. Use computational trimming to reduce work.
    ComputePass2<AxisToSum> worklet2(pdims);
    invoke(
      worklet2, metaDataMesh2D, metaDataSums, metaDataMin, metaDataMax, metaDataNumTris, edgeCases);

    //----------------------------------------------------------------------------
    // PASS 3: Compute the number of points and triangles that each edge
    // row needs to generate by using exclusive scans.
    vtkm::cont::Algorithm::ScanExtended(metaDataNumTris, metaDataNumTris);
    auto sumTris =
      vtkm::cont::ArrayGetValue(metaDataNumTris.GetNumberOfValues() - 1, metaDataNumTris);
    if (sumTris > 0)
    {
      detail::extend_by(triangle_topology, 3 * sumTris);
      detail::extend_by(sharedState.CellIdMap, sumTris);


      auto newPointSize =
        vtkm::cont::Algorithm::ScanExclusive(metaDataLinearSums, metaDataLinearSums);
      detail::extend_by(sharedState.InterpolationEdgeIds, newPointSize);
      detail::extend_by(sharedState.InterpolationWeights, newPointSize);

      //----------------------------------------------------------------------------
      // PASS 4: Process voxel rows and generate topology, and interpolation state
      ComputePass4<ValueType, AxisToSum> worklet4(
        isoval, pdims, multiContourCellOffset, multiContourPointOffset);
      invoke(worklet4,
             metaDataMesh2D,
             metaDataSums,
             metaDataMin,
             metaDataMax,
             metaDataNumTris,
             edgeCases,
             inputField,
             triangle_topology,
             sharedState.InterpolationEdgeIds,
             sharedState.InterpolationWeights,
             sharedState.CellIdMap);
    }

    //----------------------------------------------------------------------------
    // PASS 5: Convert the edge interpolation information to point and normals
    vtkm::Vec3f origin, spacing;
    { //extract out the origin and spacing as these are needed for Pass5 to properly
      //interpolate the new points
      auto portal = coordinateSystem.ReadPortal();
      origin = portal.GetOrigin();
      spacing = portal.GetSpacing();
    }
    if (sharedState.GenerateNormals)
    {
      normals.Allocate(sharedState.InterpolationEdgeIds.GetNumberOfValues());
    }

    ComputePass5<ValueType> worklet5(pdims, origin, spacing, sharedState.GenerateNormals);
    invoke(worklet5,
           sharedState.InterpolationEdgeIds,
           sharedState.InterpolationWeights,
           points,
           inputField,
           normals);
  }

  vtkm::cont::CellSetSingleType<> outputCells;
  outputCells.Fill(points.GetNumberOfValues(), vtkm::CELL_SHAPE_TRIANGLE, 3, triangle_topology);
  return outputCells;
}

} //namespace flying_edges
}
}

#endif
