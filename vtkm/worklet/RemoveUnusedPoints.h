//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_RemoveUnusedPoints_h
#define vtk_m_worklet_RemoveUnusedPoints_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetExplicit.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{

/// A collection of worklets used to identify which points are used by at least
/// one cell and then remove the points that are not used by any cells. The
/// class containing these worklets can be used to manage running these
/// worklets, building new cell sets, and redefine field arrays.
///
class RemoveUnusedPoints
{
public:
  /// A worklet that creates a mask of used points (the first step in removing
  /// unused points). Given an array of point indices (taken from the
  /// connectivity of a CellSetExplicit) and an array mask initialized to 0,
  /// writes a 1 at the index of every point referenced by a cell.
  ///
  struct GeneratePointMask : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn pointIndices, WholeArrayInOut pointMask);
    using ExecutionSignature = void(_1, _2);

    template <typename PointMaskPortalType>
    VTKM_EXEC void operator()(vtkm::Id pointIndex, const PointMaskPortalType& pointMask) const
    {
      pointMask.Set(pointIndex, 1);
    }
  };

  /// A worklet that takes an array of point indices (taken from the
  /// connectivity of a CellSetExplicit) and an array that functions as a map
  /// from the original indices to new indices, creates a new array with the
  /// new mapped indices.
  ///
  struct TransformPointIndices : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn pointIndex, WholeArrayIn indexMap, FieldOut mappedPoints);
    using ExecutionSignature = _3(_1, _2);

    template <typename IndexMapPortalType>
    VTKM_EXEC vtkm::Id operator()(vtkm::Id pointIndex, const IndexMapPortalType& indexPortal) const
    {
      return indexPortal.Get(pointIndex);
    }
  };

public:
  VTKM_CONT
  RemoveUnusedPoints() {}

  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VTKM_CONT RemoveUnusedPoints(
    const vtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& inCellSet)
  {
    this->FindPointsStart();
    this->FindPoints(inCellSet);
    this->FindPointsEnd();
  }

  /// Get this class ready for identifying the points used by cell sets.
  ///
  VTKM_CONT void FindPointsStart() { this->MaskArray.ReleaseResources(); }

  /// Analyze the given cell set to find all points that are used. Unused
  /// points are those that are not found in any cell sets passed to this
  /// method.
  ///
  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VTKM_CONT void FindPoints(
    const vtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& inCellSet)
  {
    if (this->MaskArray.GetNumberOfValues() < 1)
    {
      // Initialize mask array to 0.
      vtkm::cont::ArrayCopy(
        vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(0, inCellSet.GetNumberOfPoints()),
        this->MaskArray);
    }
    VTKM_ASSERT(this->MaskArray.GetNumberOfValues() == inCellSet.GetNumberOfPoints());

    vtkm::worklet::DispatcherMapField<GeneratePointMask> dispatcher;
    dispatcher.Invoke(inCellSet.GetConnectivityArray(vtkm::TopologyElementTagCell(),
                                                     vtkm::TopologyElementTagPoint()),
                      this->MaskArray);
  }

  /// Compile the information collected from calls to \c FindPointsInCellSet to
  /// ready this class for mapping cell sets and fields.
  ///
  VTKM_CONT void FindPointsEnd()
  {
    this->PointScatter.reset(new vtkm::worklet::ScatterCounting(this->MaskArray, true));

    this->MaskArray.ReleaseResources();
  }

  /// \brief Map cell indices
  ///
  /// Given a cell set (typically the same one passed to the constructor)
  /// returns a new cell set with cell points transformed to use the indices of
  /// the new reduced point arrays.
  ///
  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VTKM_CONT
    vtkm::cont::CellSetExplicit<ShapeStorage, VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG, OffsetsStorage>
    MapCellSet(const vtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>&
                 inCellSet) const
  {
    VTKM_ASSERT(this->PointScatter);

    return MapCellSet(inCellSet,
                      this->PointScatter->GetInputToOutputMap(),
                      this->PointScatter->GetOutputToInputMap().GetNumberOfValues());
  }

  /// \brief Map cell indices
  ///
  /// Given a cell set (typically the same one passed to the constructor) and
  /// an array that maps point indices from an old set of indices to a new set,
  /// returns a new cell set with cell points transformed to use the indices of
  /// the new reduced point arrays.
  ///
  /// This helper method can be used by external items that do similar operations
  /// that remove points or otherwise rearange points in a cell set. If points
  /// were removed by calling \c FindPoints, then you should use the other form
  /// of \c MapCellSet.
  ///
  template <typename ShapeStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage,
            typename MapStorage>
  VTKM_CONT static vtkm::cont::CellSetExplicit<ShapeStorage,
                                               VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
                                               OffsetsStorage>
  MapCellSet(
    const vtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& inCellSet,
    const vtkm::cont::ArrayHandle<vtkm::Id, MapStorage>& inputToOutputPointMap,
    vtkm::Id numberOfPoints)
  {
    using VisitTopology = vtkm::TopologyElementTagCell;
    using IncidentTopology = vtkm::TopologyElementTagPoint;

    using NewConnectivityStorage = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG;

    vtkm::cont::ArrayHandle<vtkm::Id, NewConnectivityStorage> newConnectivityArray;

    vtkm::worklet::DispatcherMapField<TransformPointIndices> dispatcher;
    dispatcher.Invoke(inCellSet.GetConnectivityArray(VisitTopology(), IncidentTopology()),
                      inputToOutputPointMap,
                      newConnectivityArray);

    vtkm::cont::CellSetExplicit<ShapeStorage, NewConnectivityStorage, OffsetsStorage> outCellSet;
    outCellSet.Fill(numberOfPoints,
                    inCellSet.GetShapesArray(VisitTopology(), IncidentTopology()),
                    newConnectivityArray,
                    inCellSet.GetOffsetsArray(VisitTopology(), IncidentTopology()));

    return outCellSet;
  }

  /// \brief Maps a point field from the original points to the new reduced points
  ///
  /// Given an array handle that holds the values for a point field of the
  /// original data set, returns a new array handle containing field values
  /// rearranged to the new indices of the reduced point set.
  ///
  /// This version of point mapping performs a shallow copy by using a
  /// permutation array.
  ///
  template <typename InArrayHandle>
  VTKM_CONT vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id>, InArrayHandle>
  MapPointFieldShallow(const InArrayHandle& inArray) const
  {
    VTKM_ASSERT(this->PointScatter);

    return vtkm::cont::make_ArrayHandlePermutation(this->PointScatter->GetOutputToInputMap(),
                                                   inArray);
  }

  /// \brief Maps a point field from the original points to the new reduced points
  ///
  /// Given an array handle that holds the values for a point field of the
  /// original data set, returns a new array handle containing field values
  /// rearranged to the new indices of the reduced point set.
  ///
  /// This version of point mapping performs a deep copy into the destination
  /// array provided.
  ///
  template <typename InArrayHandle, typename OutArrayHandle>
  VTKM_CONT void MapPointFieldDeep(const InArrayHandle& inArray, OutArrayHandle& outArray) const
  {
    VTKM_IS_ARRAY_HANDLE(InArrayHandle);
    VTKM_IS_ARRAY_HANDLE(OutArrayHandle);

    vtkm::cont::ArrayCopy(this->MapPointFieldShallow(inArray), outArray);
  }

  /// \brief Maps a point field from the original points to the new reduced points
  ///
  /// Given an array handle that holds the values for a point field of the
  /// original data set, returns a new array handle containing field values
  /// rearranged to the new indices of the reduced point set.
  ///
  /// This version of point mapping performs a deep copy into an array that is
  /// returned.
  ///
  template <typename InArrayHandle>
  vtkm::cont::ArrayHandle<typename InArrayHandle::ValueType> MapPointFieldDeep(
    const InArrayHandle& inArray) const
  {
    VTKM_IS_ARRAY_HANDLE(InArrayHandle);

    vtkm::cont::ArrayHandle<typename InArrayHandle::ValueType> outArray;
    this->MapPointFieldDeep(inArray, outArray);

    return outArray;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::IdComponent> MaskArray;

  /// Manages how the original point indices map to the new point indices.
  ///
  std::shared_ptr<vtkm::worklet::ScatterCounting> PointScatter;
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_RemoveUnusedPoints_h
