//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_KdTree3DConstruction_h
#define vtk_m_worklet_KdTree3DConstruction_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleReverse.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/internal/DispatcherBase.h>
#include <vtkm/worklet/internal/WorkletBase.h>

namespace vtkm
{
namespace worklet
{
namespace spatialstructure
{

class KdTree3DConstruction
{
public:
  ////////// General WORKLET for Kd-tree  //////
  class ComputeFlag : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn rank, FieldIn pointCountInSeg, FieldOut flag);
    using ExecutionSignature = void(_1, _2, _3);

    VTKM_CONT
    ComputeFlag() {}

    template <typename T>
    VTKM_EXEC void operator()(const T& rank, const T& pointCountInSeg, T& flag) const
    {
      if (static_cast<float>(rank) >= static_cast<float>(pointCountInSeg) / 2.0f)
        flag = 1; //right subtree
      else
        flag = 0; //left subtree
    }
  };

  class InverseArray : public vtkm::worklet::WorkletMapField
  { //only for 0/1 array
  public:
    using ControlSignature = void(FieldIn in, FieldOut out);
    using ExecutionSignature = void(_1, _2);

    VTKM_CONT
    InverseArray() {}

    template <typename T>
    VTKM_EXEC void operator()(const T& in, T& out) const
    {
      if (in == 0)
        out = 1;
      else
        out = 0;
    }
  };

  class SegmentedSplitTransform : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature =
      void(FieldIn B, FieldIn D, FieldIn F, FieldIn G, FieldIn H, FieldOut I);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

    VTKM_CONT
    SegmentedSplitTransform() {}

    template <typename T>
    VTKM_EXEC void operator()(const T& B, const T& D, const T& F, const T& G, const T& H, T& I)
      const
    {
      if (B == 1)
      {
        I = F + H + D;
      }
      else
      {
        I = F + G - 1;
      }
    }
  };

  class ScatterArray : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn in, FieldIn index, WholeArrayOut out);
    using ExecutionSignature = void(_1, _2, _3);

    VTKM_CONT
    ScatterArray() {}

    template <typename T, typename OutputArrayPortalType>
    VTKM_EXEC void operator()(const T& in,
                              const T& index,
                              const OutputArrayPortalType& outputPortal) const
    {
      outputPortal.Set(index, in);
    }
  };

  class NewSegmentId : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn inSegmentId, FieldIn flag, FieldOut outSegmentId);
    using ExecutionSignature = void(_1, _2, _3);

    VTKM_CONT
    NewSegmentId() {}

    template <typename T>
    VTKM_EXEC void operator()(const T& oldSegId, const T& flag, T& newSegId) const
    {
      if (flag == 0)
        newSegId = oldSegId * 2;
      else
        newSegId = oldSegId * 2 + 1;
    }
  };

  class SaveSplitPointId : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn pointId,
                                  FieldIn flag,
                                  FieldIn oldSplitPointId,
                                  FieldOut newSplitPointId);
    using ExecutionSignature = void(_1, _2, _3, _4);

    VTKM_CONT
    SaveSplitPointId() {}

    template <typename T>
    VTKM_EXEC void operator()(const T& pointId,
                              const T& flag,
                              const T& oldSplitPointId,
                              T& newSplitPointId) const
    {
      if (flag == 0) //do not change
        newSplitPointId = oldSplitPointId;
      else //split point id
        newSplitPointId = pointId;
    }
  };

  class FindSplitPointId : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn pointId, FieldIn rank, FieldOut splitIdInsegment);
    using ExecutionSignature = void(_1, _2, _3);

    VTKM_CONT
    FindSplitPointId() {}

    template <typename T>
    VTKM_EXEC void operator()(const T& pointId, const T& rank, T& splitIdInsegment) const
    {
      if (rank == 0) //do not change
        splitIdInsegment = pointId;
      else                     //split point id
        splitIdInsegment = -1; //indicate this is not split point
    }
  };

  class ArrayAdd : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn inArray0, FieldIn inArray1, FieldOut outArray);
    using ExecutionSignature = void(_1, _2, _3);

    VTKM_CONT
    ArrayAdd() {}

    template <typename T>
    VTKM_EXEC void operator()(const T& in0, const T& in1, T& out) const
    {
      out = in0 + in1;
    }
  };

  class SeprateVec3AryHandle : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn inVec3, FieldOut out0, FieldOut out1, FieldOut out2);
    using ExecutionSignature = void(_1, _2, _3, _4);

    VTKM_CONT
    SeprateVec3AryHandle() {}

    template <typename T>
    VTKM_EXEC void operator()(const Vec<T, 3>& inVec3, T& out0, T& out1, T& out2) const
    {
      out0 = inVec3[0];
      out1 = inVec3[1];
      out2 = inVec3[2];
    }
  };

  ////////// General worklet WRAPPER for Kd-tree //////
  template <typename T, class BinaryFunctor>
  vtkm::cont::ArrayHandle<T> ReverseScanInclusiveByKey(vtkm::cont::ArrayHandle<T>& keyHandle,
                                                       vtkm::cont::ArrayHandle<T>& dataHandle,
                                                       BinaryFunctor binary_functor)
  {
    using Algorithm = vtkm::cont::Algorithm;

    vtkm::cont::ArrayHandle<T> resultHandle;

    auto reversedResultHandle = vtkm::cont::make_ArrayHandleReverse(resultHandle);

    Algorithm::ScanInclusiveByKey(vtkm::cont::make_ArrayHandleReverse(keyHandle),
                                  vtkm::cont::make_ArrayHandleReverse(dataHandle),
                                  reversedResultHandle,
                                  binary_functor);

    return resultHandle;
  }

  template <typename T>
  vtkm::cont::ArrayHandle<T> Inverse01ArrayWrapper(vtkm::cont::ArrayHandle<T>& inputHandle)
  {
    vtkm::cont::ArrayHandle<T> InverseHandle;
    InverseArray invWorklet;
    vtkm::worklet::DispatcherMapField<InverseArray> inverseArrayDispatcher(invWorklet);
    inverseArrayDispatcher.Invoke(inputHandle, InverseHandle);
    return InverseHandle;
  }

  template <typename T>
  vtkm::cont::ArrayHandle<T> ScatterArrayWrapper(vtkm::cont::ArrayHandle<T>& inputHandle,
                                                 vtkm::cont::ArrayHandle<T>& indexHandle)
  {
    vtkm::cont::ArrayHandle<T> outputHandle;
    outputHandle.Allocate(inputHandle.GetNumberOfValues());
    ScatterArray scatterWorklet;
    vtkm::worklet::DispatcherMapField<ScatterArray> scatterArrayDispatcher(scatterWorklet);
    scatterArrayDispatcher.Invoke(inputHandle, indexHandle, outputHandle);
    return outputHandle;
  }

  template <typename T>
  vtkm::cont::ArrayHandle<T> NewKeyWrapper(vtkm::cont::ArrayHandle<T>& oldSegIdHandle,
                                           vtkm::cont::ArrayHandle<T>& flagHandle)
  {
    vtkm::cont::ArrayHandle<T> newSegIdHandle;
    NewSegmentId newsegidWorklet;
    vtkm::worklet::DispatcherMapField<NewSegmentId> newSegIdDispatcher(newsegidWorklet);
    newSegIdDispatcher.Invoke(oldSegIdHandle, flagHandle, newSegIdHandle);
    return newSegIdHandle;
  }

  template <typename T>
  vtkm::cont::ArrayHandle<T> SaveSplitPointIdWrapper(vtkm::cont::ArrayHandle<T>& pointIdHandle,
                                                     vtkm::cont::ArrayHandle<T>& flagHandle,
                                                     vtkm::cont::ArrayHandle<T>& rankHandle,
                                                     vtkm::cont::ArrayHandle<T>& oldSplitIdHandle)
  {
    vtkm::cont::ArrayHandle<T> splitIdInSegmentHandle;
    FindSplitPointId findSplitPointIdWorklet;
    vtkm::worklet::DispatcherMapField<FindSplitPointId> findSplitPointIdWorkletDispatcher(
      findSplitPointIdWorklet);
    findSplitPointIdWorkletDispatcher.Invoke(pointIdHandle, rankHandle, splitIdInSegmentHandle);

    vtkm::cont::ArrayHandle<T> splitIdInSegmentByScanHandle =
      ReverseScanInclusiveByKey(flagHandle, splitIdInSegmentHandle, vtkm::Maximum());

    vtkm::cont::ArrayHandle<T> splitIdHandle;
    SaveSplitPointId saveSplitPointIdWorklet;
    vtkm::worklet::DispatcherMapField<SaveSplitPointId> saveSplitPointIdWorkletDispatcher(
      saveSplitPointIdWorklet);
    saveSplitPointIdWorkletDispatcher.Invoke(
      splitIdInSegmentByScanHandle, flagHandle, oldSplitIdHandle, splitIdHandle);

    return splitIdHandle;
  }

  template <typename T>
  vtkm::cont::ArrayHandle<T> ArrayAddWrapper(vtkm::cont::ArrayHandle<T>& array0Handle,
                                             vtkm::cont::ArrayHandle<T>& array1Handle)
  {
    vtkm::cont::ArrayHandle<T> resultHandle;
    ArrayAdd arrayAddWorklet;
    vtkm::worklet::DispatcherMapField<ArrayAdd> arrayAddDispatcher(arrayAddWorklet);
    arrayAddDispatcher.Invoke(array0Handle, array1Handle, resultHandle);
    return resultHandle;
  }

  ///////////////////////////////////////////////////
  ////////General Kd tree function //////////////////
  ///////////////////////////////////////////////////
  template <typename T>
  vtkm::cont::ArrayHandle<T> ComputeFlagProcedure(vtkm::cont::ArrayHandle<T>& rankHandle,
                                                  vtkm::cont::ArrayHandle<T>& segIdHandle)
  {
    using Algorithm = vtkm::cont::Algorithm;

    vtkm::cont::ArrayHandle<T> segCountAryHandle;
    {
      vtkm::cont::ArrayHandle<T> tmpAryHandle;
      vtkm::cont::ArrayHandleConstant<T> constHandle(1, rankHandle.GetNumberOfValues());
      Algorithm::ScanInclusiveByKey(
        segIdHandle, constHandle, tmpAryHandle, vtkm::Add()); //compute ttl segs in segment

      segCountAryHandle = ReverseScanInclusiveByKey(segIdHandle, tmpAryHandle, vtkm::Maximum());
    }

    vtkm::cont::ArrayHandle<T> flagHandle;
    vtkm::worklet::DispatcherMapField<ComputeFlag> computeFlagDispatcher;
    computeFlagDispatcher.Invoke(rankHandle, segCountAryHandle, flagHandle);

    return flagHandle;
  }

  template <typename T>
  vtkm::cont::ArrayHandle<T> SegmentedSplitProcedure(vtkm::cont::ArrayHandle<T>& A_Handle,
                                                     vtkm::cont::ArrayHandle<T>& B_Handle,
                                                     vtkm::cont::ArrayHandle<T>& C_Handle)
  {
    using Algorithm = vtkm::cont::Algorithm;

    vtkm::cont::ArrayHandle<T> D_Handle;
    T initValue = 0;
    Algorithm::ScanExclusiveByKey(C_Handle, B_Handle, D_Handle, initValue, vtkm::Add());

    vtkm::cont::ArrayHandleCounting<T> Ecouting_Handle(0, 1, A_Handle.GetNumberOfValues());
    vtkm::cont::ArrayHandle<T> E_Handle;
    Algorithm::Copy(Ecouting_Handle, E_Handle);

    vtkm::cont::ArrayHandle<T> F_Handle;
    Algorithm::ScanInclusiveByKey(C_Handle, E_Handle, F_Handle, vtkm::Minimum());

    vtkm::cont::ArrayHandle<T> InvB_Handle = Inverse01ArrayWrapper(B_Handle);
    vtkm::cont::ArrayHandle<T> G_Handle;
    Algorithm::ScanInclusiveByKey(C_Handle, InvB_Handle, G_Handle, vtkm::Add());

    vtkm::cont::ArrayHandle<T> H_Handle =
      ReverseScanInclusiveByKey(C_Handle, G_Handle, vtkm::Maximum());

    vtkm::cont::ArrayHandle<T> I_Handle;
    SegmentedSplitTransform sstWorklet;
    vtkm::worklet::DispatcherMapField<SegmentedSplitTransform> segmentedSplitTransformDispatcher(
      sstWorklet);
    segmentedSplitTransformDispatcher.Invoke(
      B_Handle, D_Handle, F_Handle, G_Handle, H_Handle, I_Handle);

    return ScatterArrayWrapper(A_Handle, I_Handle);
  }

  template <typename T>
  void RenumberRanksProcedure(vtkm::cont::ArrayHandle<T>& A_Handle,
                              vtkm::cont::ArrayHandle<T>& B_Handle,
                              vtkm::cont::ArrayHandle<T>& C_Handle,
                              vtkm::cont::ArrayHandle<T>& D_Handle)
  {
    using Algorithm = vtkm::cont::Algorithm;

    vtkm::Id nPoints = A_Handle.GetNumberOfValues();

    vtkm::cont::ArrayHandleCounting<T> Ecouting_Handle(0, 1, nPoints);
    vtkm::cont::ArrayHandle<T> E_Handle;
    Algorithm::Copy(Ecouting_Handle, E_Handle);

    vtkm::cont::ArrayHandle<T> F_Handle;
    Algorithm::ScanInclusiveByKey(D_Handle, E_Handle, F_Handle, vtkm::Minimum());

    vtkm::cont::ArrayHandle<T> G_Handle;
    G_Handle = ArrayAddWrapper(A_Handle, F_Handle);

    vtkm::cont::ArrayHandleConstant<T> HConstant_Handle(1, nPoints);
    vtkm::cont::ArrayHandle<T> H_Handle;
    Algorithm::Copy(HConstant_Handle, H_Handle);

    vtkm::cont::ArrayHandle<T> I_Handle;
    T initValue = 0;
    Algorithm::ScanExclusiveByKey(C_Handle, H_Handle, I_Handle, initValue, vtkm::Add());

    vtkm::cont::ArrayHandle<T> J_Handle;
    J_Handle = ScatterArrayWrapper(I_Handle, G_Handle);

    vtkm::cont::ArrayHandle<T> K_Handle;
    K_Handle = ScatterArrayWrapper(B_Handle, G_Handle);

    vtkm::cont::ArrayHandle<T> L_Handle;
    L_Handle = SegmentedSplitProcedure(J_Handle, K_Handle, D_Handle);

    vtkm::cont::ArrayHandle<T> M_Handle;
    Algorithm::ScanInclusiveByKey(C_Handle, E_Handle, M_Handle, vtkm::Minimum());

    vtkm::cont::ArrayHandle<T> N_Handle;
    N_Handle = ArrayAddWrapper(L_Handle, M_Handle);

    A_Handle = ScatterArrayWrapper(I_Handle, N_Handle);
  }

  /////////////3D construction      /////////////////////
  /// \brief Segmented split for 3D x, y, z coordinates
  ///
  /// Split \c pointId_Handle, \c X_Handle, \c Y_Handle and \c Z_Handle within each segment
  /// as indicated by \c segId_Handle according to flags in \c flag_Handle.
  ///
  /// \tparam T
  /// \param pointId_Handle
  /// \param flag_Handle
  /// \param segId_Handle
  /// \param X_Handle
  /// \param Y_Handle
  /// \param Z_Handle
  template <typename T>
  void SegmentedSplitProcedure3D(vtkm::cont::ArrayHandle<T>& pointId_Handle,
                                 vtkm::cont::ArrayHandle<T>& flag_Handle,
                                 vtkm::cont::ArrayHandle<T>& segId_Handle,
                                 vtkm::cont::ArrayHandle<T>& X_Handle,
                                 vtkm::cont::ArrayHandle<T>& Y_Handle,
                                 vtkm::cont::ArrayHandle<T>& Z_Handle)
  {
    using Algorithm = vtkm::cont::Algorithm;

    vtkm::cont::ArrayHandle<T> D_Handle;
    T initValue = 0;
    Algorithm::ScanExclusiveByKey(segId_Handle, flag_Handle, D_Handle, initValue, vtkm::Add());

    vtkm::cont::ArrayHandleCounting<T> Ecouting_Handle(0, 1, pointId_Handle.GetNumberOfValues());
    vtkm::cont::ArrayHandle<T> E_Handle;
    Algorithm::Copy(Ecouting_Handle, E_Handle);

    vtkm::cont::ArrayHandle<T> F_Handle;
    Algorithm::ScanInclusiveByKey(segId_Handle, E_Handle, F_Handle, vtkm::Minimum());

    vtkm::cont::ArrayHandle<T> InvB_Handle = Inverse01ArrayWrapper(flag_Handle);
    vtkm::cont::ArrayHandle<T> G_Handle;
    Algorithm::ScanInclusiveByKey(segId_Handle, InvB_Handle, G_Handle, vtkm::Add());

    vtkm::cont::ArrayHandle<T> H_Handle =
      ReverseScanInclusiveByKey(segId_Handle, G_Handle, vtkm::Maximum());

    vtkm::cont::ArrayHandle<T> I_Handle;
    SegmentedSplitTransform sstWorklet;
    vtkm::worklet::DispatcherMapField<SegmentedSplitTransform> segmentedSplitTransformDispatcher(
      sstWorklet);
    segmentedSplitTransformDispatcher.Invoke(
      flag_Handle, D_Handle, F_Handle, G_Handle, H_Handle, I_Handle);

    pointId_Handle = ScatterArrayWrapper(pointId_Handle, I_Handle);

    flag_Handle = ScatterArrayWrapper(flag_Handle, I_Handle);

    X_Handle = ScatterArrayWrapper(X_Handle, I_Handle);

    Y_Handle = ScatterArrayWrapper(Y_Handle, I_Handle);

    Z_Handle = ScatterArrayWrapper(Z_Handle, I_Handle);
  }

  /// \brief Perform one level of KD-Tree construction
  ///
  /// Construct a level of KD-Tree by segemeted splits (partitioning) of \c pointId_Handle,
  /// \c xrank_Handle, \c yrank_Handle and \c zrank_Handle according to the medium element
  /// in each segment as indicated by \c segId_Handle alone the axis determined by \c level.
  /// The split point of each segment will be updated in \c splitId_Handle.
  template <typename T>
  void OneLevelSplit3D(vtkm::cont::ArrayHandle<T>& pointId_Handle,
                       vtkm::cont::ArrayHandle<T>& xrank_Handle,
                       vtkm::cont::ArrayHandle<T>& yrank_Handle,
                       vtkm::cont::ArrayHandle<T>& zrank_Handle,
                       vtkm::cont::ArrayHandle<T>& segId_Handle,
                       vtkm::cont::ArrayHandle<T>& splitId_Handle,
                       vtkm::Int32 level)
  {
    using Algorithm = vtkm::cont::Algorithm;

    vtkm::cont::ArrayHandle<T> flag_Handle;
    if (level % 3 == 0)
    {
      flag_Handle = ComputeFlagProcedure(xrank_Handle, segId_Handle);
    }
    else if (level % 3 == 1)
    {
      flag_Handle = ComputeFlagProcedure(yrank_Handle, segId_Handle);
    }
    else
    {
      flag_Handle = ComputeFlagProcedure(zrank_Handle, segId_Handle);
    }

    SegmentedSplitProcedure3D(
      pointId_Handle, flag_Handle, segId_Handle, xrank_Handle, yrank_Handle, zrank_Handle);

    vtkm::cont::ArrayHandle<T> segIdOld_Handle;
    Algorithm::Copy(segId_Handle, segIdOld_Handle);
    segId_Handle = NewKeyWrapper(segIdOld_Handle, flag_Handle);

    RenumberRanksProcedure(xrank_Handle, flag_Handle, segId_Handle, segIdOld_Handle);
    RenumberRanksProcedure(yrank_Handle, flag_Handle, segId_Handle, segIdOld_Handle);
    RenumberRanksProcedure(zrank_Handle, flag_Handle, segId_Handle, segIdOld_Handle);

    if (level % 3 == 0)
    {
      splitId_Handle =
        SaveSplitPointIdWrapper(pointId_Handle, flag_Handle, xrank_Handle, splitId_Handle);
    }
    else if (level % 3 == 1)
    {
      splitId_Handle =
        SaveSplitPointIdWrapper(pointId_Handle, flag_Handle, yrank_Handle, splitId_Handle);
    }
    else
    {
      splitId_Handle =
        SaveSplitPointIdWrapper(pointId_Handle, flag_Handle, zrank_Handle, splitId_Handle);
    }
  }

  /// \brief Construct KdTree from x y z coordinate vector.
  ///
  /// This method constructs an array based KD-Tree from x, y, z coordinates of points in \c
  /// coordi_Handle. The method rotates between x, y and z axis and splits input points into
  /// equal halves with respect to the split axis at each level of construction. The indices to
  /// the leaf nodes are returned in \c pointId_Handle and indices to internal nodes (splits)
  /// are returned in splitId_handle.
  ///
  /// \param coordi_Handle (in) x, y, z coordinates of input points
  /// \param pointId_Handle (out) returns indices to leaf nodes of the KD-tree
  /// \param splitId_Handle (out) returns indices to internal nodes of the KD-tree
  // Leaf Node vector and internal node (split) vectpr
  template <typename CoordType, typename CoordStorageTag>
  void Run(const vtkm::cont::ArrayHandle<vtkm::Vec<CoordType, 3>, CoordStorageTag>& coordi_Handle,
           vtkm::cont::ArrayHandle<vtkm::Id>& pointId_Handle,
           vtkm::cont::ArrayHandle<vtkm::Id>& splitId_Handle)
  {
    using Algorithm = vtkm::cont::Algorithm;

    vtkm::Id nTrainingPoints = coordi_Handle.GetNumberOfValues();
    vtkm::cont::ArrayHandleCounting<vtkm::Id> counting_Handle(0, 1, nTrainingPoints);
    Algorithm::Copy(counting_Handle, pointId_Handle);
    vtkm::cont::ArrayHandle<vtkm::Id> xorder_Handle;
    Algorithm::Copy(counting_Handle, xorder_Handle);
    vtkm::cont::ArrayHandle<vtkm::Id> yorder_Handle;
    Algorithm::Copy(counting_Handle, yorder_Handle);
    vtkm::cont::ArrayHandle<vtkm::Id> zorder_Handle;
    Algorithm::Copy(counting_Handle, zorder_Handle);

    splitId_Handle.Allocate(nTrainingPoints);

    vtkm::cont::ArrayHandle<CoordType> xcoordi_Handle;
    vtkm::cont::ArrayHandle<CoordType> ycoordi_Handle;
    vtkm::cont::ArrayHandle<CoordType> zcoordi_Handle;

    SeprateVec3AryHandle sepVec3Worklet;
    vtkm::worklet::DispatcherMapField<SeprateVec3AryHandle> sepVec3Dispatcher(sepVec3Worklet);
    sepVec3Dispatcher.Invoke(coordi_Handle, xcoordi_Handle, ycoordi_Handle, zcoordi_Handle);

    Algorithm::SortByKey(xcoordi_Handle, xorder_Handle);
    vtkm::cont::ArrayHandle<vtkm::Id> xrank_Handle =
      ScatterArrayWrapper(pointId_Handle, xorder_Handle);

    Algorithm::SortByKey(ycoordi_Handle, yorder_Handle);
    vtkm::cont::ArrayHandle<vtkm::Id> yrank_Handle =
      ScatterArrayWrapper(pointId_Handle, yorder_Handle);

    Algorithm::SortByKey(zcoordi_Handle, zorder_Handle);
    vtkm::cont::ArrayHandle<vtkm::Id> zrank_Handle =
      ScatterArrayWrapper(pointId_Handle, zorder_Handle);

    vtkm::cont::ArrayHandle<vtkm::Id> segId_Handle;
    vtkm::cont::ArrayHandleConstant<vtkm::Id> constHandle(0, nTrainingPoints);
    Algorithm::Copy(constHandle, segId_Handle);

    ///// build kd tree /////
    vtkm::Int32 maxLevel = static_cast<vtkm::Int32>(ceil(vtkm::Log2(nTrainingPoints) + 1));
    for (vtkm::Int32 i = 0; i < maxLevel - 1; i++)
    {
      OneLevelSplit3D(
        pointId_Handle, xrank_Handle, yrank_Handle, zrank_Handle, segId_Handle, splitId_Handle, i);
    }
  }
};
}
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_KdTree3DConstruction_h
