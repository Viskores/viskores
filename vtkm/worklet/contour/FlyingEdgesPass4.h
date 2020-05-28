
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#ifndef vtk_m_worklet_contour_flyingedges_pass4_h
#define vtk_m_worklet_contour_flyingedges_pass4_h

#include <vtkm/worklet/contour/FlyingEdgesPass4Common.h>
#include <vtkm/worklet/contour/FlyingEdgesPass4X.h>
#include <vtkm/worklet/contour/FlyingEdgesPass4XWithNormals.h>
#include <vtkm/worklet/contour/FlyingEdgesPass4Y.h>

namespace vtkm
{
namespace worklet
{
namespace flying_edges
{

struct launchComputePass4
{
  vtkm::Id3 PointDims;
  vtkm::Vec3f Origin;
  vtkm::Vec3f Spacing;

  vtkm::Id CellWriteOffset;
  vtkm::Id PointWriteOffset;

  launchComputePass4(const vtkm::Id3& pdims,
                     const vtkm::Vec3f& origin,
                     const vtkm::Vec3f& spacing,
                     vtkm::Id multiContourCellOffset,
                     vtkm::Id multiContourPointOffset)
    : PointDims(pdims)
    , Origin(origin)
    , Spacing(spacing)
    , CellWriteOffset(multiContourCellOffset)
    , PointWriteOffset(multiContourPointOffset)
  {
  }

  template <typename DeviceAdapterTag,
            typename T,
            typename StorageTagField,
            typename MeshSums,
            typename PointType,
            typename NormalType>
  VTKM_CONT bool operator()(DeviceAdapterTag device,
                            vtkm::Id vtkmNotUsed(newPointSize),
                            T isoval,
                            const vtkm::cont::ArrayHandle<T, StorageTagField>& inputField,
                            vtkm::cont::ArrayHandle<vtkm::UInt8> edgeCases,
                            vtkm::cont::CellSetStructured<2>& metaDataMesh2D,
                            const MeshSums& metaDataSums,
                            const vtkm::cont::ArrayHandle<vtkm::Id>& metaDataMin,
                            const vtkm::cont::ArrayHandle<vtkm::Id>& metaDataMax,
                            const vtkm::cont::ArrayHandle<vtkm::Int32>& metaDataNumTris,
                            vtkm::worklet::contour::CommonState& sharedState,
                            vtkm::cont::ArrayHandle<vtkm::Id>& triangle_topology,
                            PointType& points,
                            NormalType& normals) const
  {
    vtkm::cont::Invoker invoke(device);
    if (sharedState.GenerateNormals)
    {
      ComputePass4XWithNormals<T> worklet4(isoval,
                                           this->PointDims,
                                           this->Origin,
                                           this->Spacing,
                                           this->CellWriteOffset,
                                           this->PointWriteOffset);
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
             sharedState.CellIdMap,
             points,
             normals);
    }
    else
    {
      ComputePass4X<T> worklet4(isoval,
                                this->PointDims,
                                this->Origin,
                                this->Spacing,
                                this->CellWriteOffset,
                                this->PointWriteOffset);
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
             sharedState.CellIdMap,
             points);
    }

    return true;
  }

  template <typename T,
            typename StorageTagField,
            typename MeshSums,
            typename PointType,
            typename NormalType>
  VTKM_CONT bool operator()(vtkm::cont::DeviceAdapterTagCuda device,
                            vtkm::Id newPointSize,
                            T isoval,
                            const vtkm::cont::ArrayHandle<T, StorageTagField>& inputField,
                            vtkm::cont::ArrayHandle<vtkm::UInt8> edgeCases,
                            vtkm::cont::CellSetStructured<2>& metaDataMesh2D,
                            const MeshSums& metaDataSums,
                            const vtkm::cont::ArrayHandle<vtkm::Id>& metaDataMin,
                            const vtkm::cont::ArrayHandle<vtkm::Id>& metaDataMax,
                            const vtkm::cont::ArrayHandle<vtkm::Int32>& metaDataNumTris,
                            vtkm::worklet::contour::CommonState& sharedState,
                            vtkm::cont::ArrayHandle<vtkm::Id>& triangle_topology,
                            PointType& points,
                            NormalType& normals) const
  {
    vtkm::cont::Invoker invoke(device);

    ComputePass4Y<T> worklet4(
      isoval, this->PointDims, this->CellWriteOffset, this->PointWriteOffset);
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

    //This needs to be done on array handle view ( start = this->PointWriteOffset, len = newPointSize)
    ComputePass5Y<T> worklet5(this->PointDims,
                              this->Origin,
                              this->Spacing,
                              this->PointWriteOffset,
                              sharedState.GenerateNormals);
    invoke(worklet5,
           vtkm::cont::make_ArrayHandleView(
             sharedState.InterpolationEdgeIds, this->PointWriteOffset, newPointSize),
           vtkm::cont::make_ArrayHandleView(
             sharedState.InterpolationWeights, this->PointWriteOffset, newPointSize),
           vtkm::cont::make_ArrayHandleView(points, this->PointWriteOffset, newPointSize),
           inputField,
           normals);

    return true;
  }
};
}
}
}
#endif
