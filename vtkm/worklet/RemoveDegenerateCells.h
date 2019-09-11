//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_RemoveDegeneratePolygons_h
#define vtk_m_worklet_RemoveDegeneratePolygons_h

#include <vtkm/worklet/DispatcherMapTopology.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetPermutation.h>

#include <vtkm/worklet/CellDeepCopy.h>

#include <vtkm/CellTraits.h>

#include <vtkm/exec/CellFace.h>

namespace vtkm
{
namespace worklet
{

struct RemoveDegenerateCells
{
  struct IdentifyDegenerates : vtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell);
    using ExecutionSignature = _2(CellShape, PointIndices);
    using InputDomain = _1;

    template <vtkm::IdComponent dimensionality, typename CellShapeTag, typename PointVecType>
    VTKM_EXEC bool CheckForDimensionality(vtkm::CellTopologicalDimensionsTag<dimensionality>,
                                          CellShapeTag,
                                          PointVecType&& pointIds) const
    {
      const vtkm::IdComponent numPoints = pointIds.GetNumberOfComponents();
      vtkm::IdComponent numUnduplicatedPoints = 0;
      for (vtkm::IdComponent localPointId = 0; localPointId < numPoints; ++localPointId)
      {
        ++numUnduplicatedPoints;
        if (numUnduplicatedPoints >= dimensionality + 1)
        {
          return true;
        }
        while (((localPointId < numPoints - 1) &&
                (pointIds[localPointId] == pointIds[localPointId + 1])) ||
               ((localPointId == numPoints - 1) && (pointIds[localPointId] == pointIds[0])))
        {
          // Skip over any repeated points. Assume any repeated points are adjacent.
          ++localPointId;
        }
      }
      return false;
    }

    template <typename CellShapeTag, typename PointVecType>
    VTKM_EXEC bool CheckForDimensionality(vtkm::CellTopologicalDimensionsTag<0>,
                                          CellShapeTag,
                                          PointVecType&&)
    {
      return true;
    }

    template <typename CellShapeTag, typename PointVecType>
    VTKM_EXEC bool CheckForDimensionality(vtkm::CellTopologicalDimensionsTag<3>,
                                          CellShapeTag shape,
                                          PointVecType&& pointIds)
    {
      const vtkm::IdComponent numFaces = vtkm::exec::CellFaceNumberOfFaces(shape, *this);
      vtkm::Id numValidFaces = 0;
      for (vtkm::IdComponent faceId = 0; faceId < numFaces; ++faceId)
      {
        if (this->CheckForDimensionality(
              vtkm::CellTopologicalDimensionsTag<2>(), vtkm::CellShapeTagPolygon(), pointIds))
        {
          ++numValidFaces;
          if (numValidFaces > 2)
          {
            return true;
          }
        }
      }
      return false;
    }

    template <typename CellShapeTag, typename PointIdVec>
    VTKM_EXEC bool operator()(CellShapeTag shape, const PointIdVec& pointIds) const
    {
      using Traits = vtkm::CellTraits<CellShapeTag>;
      return this->CheckForDimensionality(
        typename Traits::TopologicalDimensionsTag(), shape, pointIds);
    }

    template <typename PointIdVec>
    VTKM_EXEC bool operator()(vtkm::CellShapeTagGeneric shape, PointIdVec&& pointIds) const
    {
      bool passCell = true;
      switch (shape.Id)
      {
        vtkmGenericCellShapeMacro(passCell = (*this)(CellShapeTag(), pointIds));
        default:
          // Raise an error for unknown cell type? Pass if we don't know.
          passCell = true;
      }
      return passCell;
    }
  };

  template <typename CellSetType>
  vtkm::cont::CellSetExplicit<> Run(const CellSetType& cellSet)
  {
    vtkm::cont::ArrayHandle<bool> passFlags;
    DispatcherMapTopology<IdentifyDegenerates> dispatcher;
    dispatcher.Invoke(cellSet, passFlags);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
    vtkm::cont::Algorithm::CopyIf(
      vtkm::cont::ArrayHandleIndex(passFlags.GetNumberOfValues()), passFlags, this->ValidCellIds);

    vtkm::cont::CellSetPermutation<CellSetType> permutation(this->ValidCellIds, cellSet);
    vtkm::cont::CellSetExplicit<> output;
    vtkm::worklet::CellDeepCopy::Run(permutation, output);
    return output;
  }

  struct CallWorklet
  {
    template <typename CellSetType>
    void operator()(const CellSetType& cellSet,
                    RemoveDegenerateCells& self,
                    vtkm::cont::CellSetExplicit<>& output) const
    {
      output = self.Run(cellSet);
    }
  };

  template <typename CellSetList>
  vtkm::cont::CellSetExplicit<> Run(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellSet)
  {
    vtkm::cont::CellSetExplicit<> output;
    cellSet.CastAndCall(CallWorklet(), *this, output);

    return output;
  }

  template <typename ValueType, typename StorageTag>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageTag> in) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, in);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    vtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};
}
}

#endif //vtk_m_worklet_RemoveDegeneratePolygons_h
