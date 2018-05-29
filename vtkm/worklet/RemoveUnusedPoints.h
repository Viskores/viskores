//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_worklet_RemoveUnusedPoints_h
#define vtk_m_worklet_RemoveUnusedPoints_h

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
    using ControlSignature = void(FieldIn<> pointIndices, WholeArrayInOut<> pointMask);
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
    using ControlSignature = void(FieldIn<IdType> pointIndex,
                                  WholeArrayIn<IdType> indexMap,
                                  FieldOut<IdType> mappedPoints);
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

  template <typename ShapeStorage,
            typename NumIndicesStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage,
            typename Device>
  VTKM_CONT RemoveUnusedPoints(const vtkm::cont::CellSetExplicit<ShapeStorage,
                                                                 NumIndicesStorage,
                                                                 ConnectivityStorage,
                                                                 OffsetsStorage>& inCellSet,
                               Device)
  {
    this->FindPointsStart(Device());
    this->FindPoints(inCellSet, Device());
    this->FindPointsEnd(Device());
  }

  /// Get this class ready for identifying the points used by cell sets.
  ///
  template <typename Device>
  VTKM_CONT void FindPointsStart(Device)
  {
    this->MaskArray.ReleaseResources();
  }

  /// Analyze the given cell set to find all points that are used. Unused
  /// points are those that are not found in any cell sets passed to this
  /// method.
  ///
  template <typename ShapeStorage,
            typename NumIndicesStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage,
            typename Device>
  VTKM_CONT void FindPoints(const vtkm::cont::CellSetExplicit<ShapeStorage,
                                                              NumIndicesStorage,
                                                              ConnectivityStorage,
                                                              OffsetsStorage>& inCellSet,
                            Device)
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    if (this->MaskArray.GetNumberOfValues() < 1)
    {
      // Initialize mask array to 0.
      Algorithm::Copy(
        vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(0, inCellSet.GetNumberOfPoints()),
        this->MaskArray);
    }
    VTKM_ASSERT(this->MaskArray.GetNumberOfValues() == inCellSet.GetNumberOfPoints());

    vtkm::worklet::DispatcherMapField<GeneratePointMask, Device> dispatcher;
    dispatcher.Invoke(inCellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                     vtkm::TopologyElementTagCell()),
                      this->MaskArray);
  }

  /// Compile the information collected from calls to \c FindPointsInCellSet to
  /// ready this class for mapping cell sets and fields.
  ///
  template <typename Device>
  VTKM_CONT void FindPointsEnd(Device)
  {
    this->PointScatter.reset(new vtkm::worklet::ScatterCounting(this->MaskArray, Device(), true));

    this->MaskArray.ReleaseResources();
  }

  /// \brief Map cell indices
  ///
  /// Given a cell set (typically the same one passed to the constructor) and
  /// returns a new cell set with cell points transformed to use the indices of
  /// the new reduced point arrays.
  ///
  template <typename ShapeStorage,
            typename NumIndicesStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage,
            typename Device>
  VTKM_CONT vtkm::cont::CellSetExplicit<ShapeStorage,
                                        NumIndicesStorage,
                                        VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
                                        OffsetsStorage>
  MapCellSet(const vtkm::cont::CellSetExplicit<ShapeStorage,
                                               NumIndicesStorage,
                                               ConnectivityStorage,
                                               OffsetsStorage>& inCellSet,
             Device) const
  {
    using FromTopology = vtkm::TopologyElementTagPoint;
    using ToTopology = vtkm::TopologyElementTagCell;

    using NewConnectivityStorage = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG;

    VTKM_ASSERT(this->PointScatter);

    vtkm::cont::ArrayHandle<vtkm::Id, NewConnectivityStorage> newConnectivityArray;

    vtkm::worklet::DispatcherMapField<TransformPointIndices, Device> dispatcher;
    dispatcher.Invoke(inCellSet.GetConnectivityArray(FromTopology(), ToTopology()),
                      this->PointScatter->GetInputToOutputMap(),
                      newConnectivityArray);

    vtkm::Id numberOfPoints = this->PointScatter->GetOutputToInputMap().GetNumberOfValues();
    vtkm::cont::
      CellSetExplicit<ShapeStorage, NumIndicesStorage, NewConnectivityStorage, OffsetsStorage>
        outCellSet(inCellSet.GetName());
    outCellSet.Fill(numberOfPoints,
                    inCellSet.GetShapesArray(FromTopology(), ToTopology()),
                    inCellSet.GetNumIndicesArray(FromTopology(), ToTopology()),
                    newConnectivityArray,
                    inCellSet.GetIndexOffsetArray(FromTopology(), ToTopology()));

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
  template <typename InArrayHandle, typename OutArrayHandle, typename Device>
  VTKM_CONT void MapPointFieldDeep(const InArrayHandle& inArray,
                                   OutArrayHandle& outArray,
                                   Device) const
  {
    VTKM_IS_ARRAY_HANDLE(InArrayHandle);
    VTKM_IS_ARRAY_HANDLE(OutArrayHandle);
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    Algorithm::Copy(this->MapPointFieldShallow(inArray), outArray);
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
  template <typename InArrayHandle, typename Device>
  vtkm::cont::ArrayHandle<typename InArrayHandle::ValueType> MapPointFieldDeep(
    const InArrayHandle& inArray,
    Device) const
  {
    VTKM_IS_ARRAY_HANDLE(InArrayHandle);
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::cont::ArrayHandle<typename InArrayHandle::ValueType> outArray;
    this->MapPointFieldDeep(inArray, outArray, Device());

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
