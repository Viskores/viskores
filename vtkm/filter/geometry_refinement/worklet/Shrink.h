//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_Shrink_h
#define vtk_m_worklet_Shrink_h

#include <vtkm/worklet/CellDeepCopy.h>
#include <vtkm/worklet/ScatterCounting.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/exec/ParametricCoordinates.h>


namespace vtkm
{
namespace worklet
{
class Shrink
{
public:
  struct PrepareCellsForShrink : vtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn,
                                  FieldOutCell numPoints,
                                  FieldOutCell centroids,
                                  FieldOutCell shapes,
                                  FieldInPoint coords);
    using ExecutionSignature =
      void(PointCount, _2 numPoints, _3 centroids, _4 shapes, _5 coords, CellShape);

    using InputDomain = _1;

    template <typename CoordsArrayType, typename ShapeIdType, typename ShapeTagType>
    VTKM_EXEC void operator()(vtkm::IdComponent numPointsInCell,
                              vtkm::IdComponent& numPoints,
                              vtkm::Vec3f& centroids,
                              ShapeIdType& shapes,
                              const CoordsArrayType& coords,
                              ShapeTagType cellShape) const
    {
      numPoints = numPointsInCell;
      shapes = cellShape.Id;

      vtkm::Vec3f cellCenter;
      vtkm::exec::ParametricCoordinatesCenter(numPoints, cellShape, cellCenter);
      vtkm::exec::CellInterpolate(coords, cellCenter, cellShape, centroids);
    }
  };

  struct ComputeNewPoints : vtkm::worklet::WorkletVisitCellsWithPoints
  {
    ComputeNewPoints(vtkm::FloatDefault shrinkFactor)
      : ShrinkFactor(shrinkFactor)
    {
    }
    using ControlSignature = void(CellSetIn,
                                  FieldInCell offsets,
                                  FieldInCell centroids,
                                  FieldOutCell oldPointsMapping,
                                  FieldOutCell newPoints,
                                  FieldOutCell newCoords,
                                  FieldInPoint coords);
    using ExecutionSignature = void(_2 offsets,
                                    _3 centroids,
                                    _4 oldPointsMapping,
                                    _5 newPoints,
                                    _6 newCoords,
                                    _7 coords,
                                    VisitIndex localPointNum,
                                    PointIndices globalPointIndex);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template <typename PointIndicesVecType, typename CoordsArrayTypeIn, typename CoordsArrayTypeOut>
    VTKM_EXEC void operator()(const vtkm::Id& offsets,
                              const vtkm::Vec3f& centroids,
                              vtkm::Id& oldPointsMapping,
                              vtkm::Id& newPoints,
                              CoordsArrayTypeOut& newCoords,
                              const CoordsArrayTypeIn& coords,
                              vtkm::IdComponent localPtIndex,
                              const PointIndicesVecType& globalPointIndex) const
    {
      newPoints = offsets + localPtIndex;
      oldPointsMapping = globalPointIndex[localPtIndex];
      newCoords = centroids + this->ShrinkFactor * (coords[localPtIndex] - centroids);
    }

  private:
    vtkm::FloatDefault ShrinkFactor;
  };

  template <typename CellSetType,
            typename CoordsComType,
            typename CoordsInStorageType,
            typename CoordsOutStorageType,
            typename OldPointsMappingType,
            typename NewCellSetType>
  void Run(
    const CellSetType& oldCellset,
    const vtkm::FloatDefault shinkFactor,
    const vtkm::cont::ArrayHandle<vtkm::Vec<CoordsComType, 3>, CoordsInStorageType>& oldCoords,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordsComType, 3>, CoordsOutStorageType>& newCoords,
    vtkm::cont::ArrayHandle<vtkm::Id, OldPointsMappingType>& oldPointsMapping,
    NewCellSetType& newCellset)
  {
    vtkm::cont::Invoker invoke;

    // First pass : count the new number of points per cell, shapes and compute centroids
    vtkm::cont::ArrayHandle<vtkm::IdComponent> cellPointCount;
    vtkm::cont::ArrayHandle<vtkm::Vec3f> centroids;
    vtkm::cont::CellSetExplicit<>::ShapesArrayType shapeArray;
    invoke(PrepareCellsForShrink{}, oldCellset, cellPointCount, centroids, shapeArray, oldCoords);


    // Second pass : compute new point positions and mappings to input points
    vtkm::cont::ArrayHandle<vtkm::Id> newPoints;
    vtkm::worklet::ScatterCounting scatter(cellPointCount, true);
    vtkm::cont::ArrayHandle<vtkm::Id> offsets = scatter.GetInputToOutputMap();
    vtkm::Id totalPoints = scatter.GetOutputRange(cellPointCount.GetNumberOfValues());

    ComputeNewPoints worklet = ComputeNewPoints(shinkFactor);
    invoke(worklet,
           scatter,
           oldCellset,
           offsets,
           centroids,
           oldPointsMapping,
           newPoints,
           newCoords,
           oldCoords);

    newCellset.Fill(totalPoints,
                    shapeArray,
                    newPoints,
                    vtkm::cont::ConvertNumComponentsToOffsets(cellPointCount));
  }
};
} // namespace vtkm::worklet
} // namespace vtkm

#endif // vtk_m_worklet_Shrink_h
