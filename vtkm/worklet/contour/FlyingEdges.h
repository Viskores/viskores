
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
  vtkm::cont::Invoker invoke;

  vtkm::Vec3f origin, spacing;
  { //extract out the origin and spacing as these are needed for Pass4 to properly
    //interpolate the new points
    auto portal = coordinateSystem.ReadPortal();
    origin = portal.GetOrigin();
    spacing = portal.GetSpacing();
  }
  auto pdims = cells.GetPointDimensions();

  vtkm::cont::ArrayHandle<vtkm::UInt8> edgeCases;
  edgeCases.Allocate(coordinateSystem.GetNumberOfValues());

  vtkm::cont::CellSetStructured<2> metaDataMesh2D;
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
    {
      VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "FlyingEdges Pass1");

      // We have different logic for CUDA compared to Shared memory systems
      // since this is the first touch of lots of the arrays, and will effect
      // NUMA perf.
      //
      // Additionally CUDA does significantly better when you do an initial fill
      // and write only non-below values
      //
      ComputePass1<ValueType> worklet1(isoval, pdims);
      vtkm::cont::TryExecuteOnDevice(invoke.GetDevice(),
                                     launchComputePass1{},
                                     worklet1,
                                     inputField,
                                     edgeCases,
                                     metaDataMesh2D,
                                     metaDataSums,
                                     metaDataMin,
                                     metaDataMax);
    }

    //----------------------------------------------------------------------------
    // PASS 2: Process a single row of voxels/cells. Count the number of other
    // axis intersections by topological reasoning from previous edge cases.
    // Determine the number of primitives (i.e., triangles) generated from this
    // row. Use computational trimming to reduce work.
    {
      VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "FlyingEdges Pass2");
      ComputePass2 worklet2(pdims);
      invoke(worklet2,
             metaDataMesh2D,
             metaDataSums,
             metaDataMin,
             metaDataMax,
             metaDataNumTris,
             edgeCases);
    }

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


      vtkm::Id newPointSize =
        vtkm::cont::Algorithm::ScanExclusive(metaDataLinearSums, metaDataLinearSums);
      detail::extend_by(sharedState.InterpolationEdgeIds, newPointSize);
      detail::extend_by(sharedState.InterpolationWeights, newPointSize);

      //----------------------------------------------------------------------------
      // PASS 4: Process voxel rows and generate topology, and interpolation state
      {
        VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "FlyingEdges Pass4");

        launchComputePass4 pass4(
          pdims, origin, spacing, multiContourCellOffset, multiContourPointOffset);

        detail::extend_by(points, newPointSize);
        if (sharedState.GenerateNormals)
        {
          detail::extend_by(normals, newPointSize);
        }

        vtkm::cont::TryExecuteOnDevice(invoke.GetDevice(),
                                       pass4,
                                       newPointSize,
                                       isoval,
                                       inputField,
                                       edgeCases,
                                       metaDataMesh2D,
                                       metaDataSums,
                                       metaDataMin,
                                       metaDataMax,
                                       metaDataNumTris,
                                       sharedState,
                                       triangle_topology,
                                       points,
                                       normals);
      }
    }
  }

  vtkm::cont::CellSetSingleType<> outputCells;
  outputCells.Fill(points.GetNumberOfValues(), vtkm::CELL_SHAPE_TRIANGLE, 3, triangle_topology);
  return outputCells;
}

} //namespace flying_edges
}
}

#endif
