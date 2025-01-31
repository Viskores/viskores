//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_m_worklet_Clip_h
#define vtkm_m_worklet_Clip_h

#include <vtkm/ImplicitFunction.h>
#include <vtkm/Swap.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArraySetValues.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/filter/contour/worklet/clip/ClipTables.h>

#include <vtkm/worklet/MaskSelect.h>
#include <vtkm/worklet/WorkletMapField.h>


#if defined(THRUST_MAJOR_VERSION) && THRUST_MAJOR_VERSION == 1 && THRUST_MINOR_VERSION == 8 && \
  THRUST_SUBMINOR_VERSION < 3
// Workaround a bug in thrust 1.8.0 - 1.8.2 scan implementations which produces
// wrong results
#include <vtkm/exec/cuda/internal/ThrustPatches.h>
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/detail/type_traits.h>
VTKM_THIRDPARTY_POST_INCLUDE
#define THRUST_SCAN_WORKAROUND
#endif

namespace vtkm
{
namespace worklet
{
class Clip
{
public:
  using BatchesHandle = vtkm::cont::ArrayHandleGroupVecVariable<
    vtkm::cont::ArrayHandleIndex,
    vtkm::cont::ArrayHandleConcatenate<vtkm::cont::ArrayHandleCounting<vtkm::Id>,
                                       vtkm::cont::ArrayHandleConstant<vtkm::Id>>>;
  static BatchesHandle CreateBatches(const vtkm::Id& numberOfElements, const vtkm::Id& batchSize)
  {
    const vtkm::Id numberOfBatches = ((numberOfElements - 1) / batchSize) + 1;
    // create the offsets array
    const vtkm::cont::ArrayHandleCounting<vtkm::Id> offsetsExceptLast(
      0, batchSize, numberOfBatches);
    const vtkm::cont::ArrayHandleConstant<vtkm::Id> lastOffset(numberOfElements, 1);
    const auto offsets = vtkm::cont::make_ArrayHandleConcatenate(offsetsExceptLast, lastOffset);
    // create the indices array
    const auto indices = vtkm::cont::ArrayHandleIndex(numberOfElements);
    return vtkm::cont::make_ArrayHandleGroupVecVariable(indices, offsets);
  }

  static BatchesHandle CreateBatches(const vtkm::Id& numberOfElements)
  {
    auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
    if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagCuda{}) ||
        tracker.CanRunOn(vtkm::cont::DeviceAdapterTagKokkos{}))
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Creating batches with batch size 6 for GPUs.");
      return CreateBatches(numberOfElements, 6);
    }
    else
    {
      const vtkm::Int32 batchSize =
        vtkm::Min(1000, vtkm::Max(1, static_cast<vtkm::Int32>(numberOfElements / 250000)));
      VTKM_LOG_F(
        vtkm::cont::LogLevel::Info, "Creating batches with batch size %d for CPUs.", batchSize);
      return CreateBatches(numberOfElements, batchSize);
    }
  }

  struct PointBatchData
  {
    vtkm::Id NumberOfKeptPoints = 0;

    struct SumOp
    {
      VTKM_EXEC_CONT
      PointBatchData operator()(const PointBatchData& stat1, const PointBatchData& stat2) const
      {
        PointBatchData sum = stat1;
        sum.NumberOfKeptPoints += stat2.NumberOfKeptPoints;
        return sum;
      }
    };
  };

  struct CellBatchData
  {
    vtkm::Id NumberOfCells = 0;
    vtkm::Id NumberOfCellIndices = 0;
    vtkm::Id NumberOfEdges = 0;
    vtkm::Id NumberOfCentroids = 0;
    vtkm::Id NumberOfCentroidIndices = 0;

    struct SumOp
    {
      VTKM_EXEC_CONT
      CellBatchData operator()(const CellBatchData& stat1, const CellBatchData& stat2) const
      {
        CellBatchData sum = stat1;
        sum.NumberOfCells += stat2.NumberOfCells;
        sum.NumberOfCellIndices += stat2.NumberOfCellIndices;
        sum.NumberOfEdges += stat2.NumberOfEdges;
        sum.NumberOfCentroids += stat2.NumberOfCentroids;
        sum.NumberOfCentroidIndices += stat2.NumberOfCentroidIndices;
        return sum;
      }
    };
  };

  struct EdgeInterpolation
  {
    vtkm::Id Vertex1 = -1;
    vtkm::Id Vertex2 = -1;
    vtkm::Float64 Weight = 0;

    struct LessThanOp
    {
      VTKM_EXEC
      bool operator()(const EdgeInterpolation& v1, const EdgeInterpolation& v2) const
      {
        return (v1.Vertex1 < v2.Vertex1) || (v1.Vertex1 == v2.Vertex1 && v1.Vertex2 < v2.Vertex2);
      }
    };

    struct EqualToOp
    {
      VTKM_EXEC
      bool operator()(const EdgeInterpolation& v1, const EdgeInterpolation& v2) const
      {
        return v1.Vertex1 == v2.Vertex1 && v1.Vertex2 == v2.Vertex2;
      }
    };
  };

  /**
   * This worklet identifies the input points that are kept, i.e. are inside the implicit function.
   */
  template <bool Invert>
  class MarkKeptPoints : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn pointBatch,
                                  FieldOut pointBatchData,
                                  FieldOut batchWithKeptPointsMask,
                                  WholeArrayIn scalars,
                                  WholeArrayOut keptPointsMask);
    using ExecutionSignature = void(_1, _2, _3, _4, _5);

    VTKM_CONT
    explicit MarkKeptPoints(vtkm::Float64 isoValue)
      : IsoValue(isoValue)
    {
    }

    template <typename BatchType, typename PointScalars, typename KeptPointsMask>
    VTKM_EXEC void operator()(const BatchType& pointBatch,
                              PointBatchData& pointBatchData,
                              vtkm::UInt8& batchWithKeptPointsMask,
                              const PointScalars& scalars,
                              KeptPointsMask& keptPointsMask) const
    {
      for (vtkm::IdComponent id = 0, size = pointBatch.GetNumberOfComponents(); id < size; ++id)
      {
        const vtkm::Id& pointId = pointBatch[id];
        const auto scalar = scalars.Get(pointId);
        const vtkm::UInt8 kept = Invert ? scalar < this->IsoValue : scalar >= this->IsoValue;
        keptPointsMask.Set(pointId, kept);
        pointBatchData.NumberOfKeptPoints += kept;
      }
      batchWithKeptPointsMask = pointBatchData.NumberOfKeptPoints > 0;
    }

  private:
    vtkm::Float64 IsoValue;
  };

  class ComputePointMaps : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn pointBatch,
                                  FieldIn pointBatchDataOffsets,
                                  WholeArrayIn keptPointsMask,
                                  WholeArrayOut pointsInputToOutput,
                                  WholeArrayOut pointsOutputToInput);
    using ExecutionSignature = void(_1, _2, _3, _4, _5);

    using MaskType = vtkm::worklet::MaskSelect;

    template <typename BatchType,
              typename KeptPointsMask,
              typename PointsInputToOutput,
              typename PointsOutputToInput>
    VTKM_EXEC void operator()(const BatchType& pointBatch,
                              const PointBatchData& pointBatchDataOffsets,
                              const KeptPointsMask& keptPointsMask,
                              PointsInputToOutput& pointsInputToOutput,
                              PointsOutputToInput& pointsOutputToInput) const
    {
      vtkm::Id pointOffset = pointBatchDataOffsets.NumberOfKeptPoints;
      for (vtkm::IdComponent id = 0, size = pointBatch.GetNumberOfComponents(); id < size; ++id)
      {
        const vtkm::Id& pointId = pointBatch[id];
        if (keptPointsMask.Get(pointId))
        {
          pointsInputToOutput.Set(pointId, pointOffset);
          pointsOutputToInput.Set(pointOffset, pointId);
          pointOffset++;
        }
      }
    }
  };

  template <bool Invert>
  class ComputeCellStats : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn cellBatch,
                                  FieldOut cellBatchData,
                                  FieldOut batchWithClippedCellsMask,
                                  FieldOut batchWithKeptOrClippedCellsMask,
                                  WholeCellSetIn<> cellSet,
                                  WholeArrayIn keptPointsMask,
                                  WholeArrayOut caseIndices);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);

    using CT = internal::ClipTables<Invert>;

    template <typename BatchType,
              typename CellSetType,
              typename KeptPointsMask,
              typename CaseIndices>
    VTKM_EXEC void operator()(const BatchType& cellBatch,
                              CellBatchData& cellBatchData,
                              vtkm::UInt8& batchWithClippedCellsMask,
                              vtkm::UInt8& batchWithKeptOrClippedCellsMask,
                              const CellSetType& cellSet,
                              const KeptPointsMask& keptPointsMask,
                              CaseIndices& caseIndices) const
    {
      namespace CTI = vtkm::worklet::internal::ClipTablesInformation;
      for (vtkm::IdComponent id = 0, size = cellBatch.GetNumberOfComponents(); id < size; ++id)
      {
        const vtkm::Id& cellId = cellBatch[id];
        const auto shape = cellSet.GetCellShape(cellId);
        const auto points = cellSet.GetIndices(cellId);
        const vtkm::IdComponent pointCount = points.GetNumberOfComponents();

        // compute case index
        vtkm::UInt8 caseIndex = 0;
        for (vtkm::IdComponent ptId = pointCount - 1; ptId >= 0; --ptId)
        {
          static constexpr auto InvertUint8 = static_cast<vtkm::UInt8>(Invert);
          caseIndex |= (InvertUint8 != keptPointsMask.Get(points[ptId])) << ptId;
        }

        if (CT::IsCellDiscarded(pointCount, caseIndex)) // discarded cell
        {
          // we do that to determine if a cell is discarded using only the caseIndex
          caseIndices.Set(cellId, CT::GetDiscardedCellCase());
        }
        else if (CT::IsCellKept(pointCount, caseIndex)) // kept cell
        {
          // we do that to determine if a cell is kept using only the caseIndex
          caseIndices.Set(cellId, CT::GetKeptCellCase());
          cellBatchData.NumberOfCells += 1;
          cellBatchData.NumberOfCellIndices += pointCount;
        }
        else // clipped cell
        {
          caseIndices.Set(cellId, caseIndex);

          vtkm::Id index = CT::GetCaseIndex(shape.Id, caseIndex);
          const vtkm::UInt8 numberOfShapes = CT::ValueAt(index++);

          cellBatchData.NumberOfCells += numberOfShapes;
          for (vtkm::IdComponent shapeId = 0; shapeId < numberOfShapes; ++shapeId)
          {
            const vtkm::UInt8 cellShape = CT::ValueAt(index++);
            const vtkm::UInt8 numberOfCellIndices = CT::ValueAt(index++);

            for (vtkm::IdComponent pointId = 0; pointId < numberOfCellIndices; pointId++, index++)
            {
              // Find how many points need to be calculated using edge interpolation.
              const vtkm::UInt8 pointIndex = CT::ValueAt(index);
              cellBatchData.NumberOfEdges += (pointIndex >= CTI::E00 && pointIndex <= CTI::E11);
            }
            if (cellShape != CTI::ST_PNT) // normal cell
            {
              // Collect number of indices required for storing current shape
              cellBatchData.NumberOfCellIndices += numberOfCellIndices;
            }
            else // cellShape == CTI::ST_PNT
            {
              --cellBatchData.NumberOfCells; // decrement since this is a centroid shape
              cellBatchData.NumberOfCentroids++;
              cellBatchData.NumberOfCentroidIndices += numberOfCellIndices;
            }
          }
        }
      }
      batchWithClippedCellsMask = cellBatchData.NumberOfCells > 0 &&
        (cellBatchData.NumberOfEdges > 0 || cellBatchData.NumberOfCentroids > 0);
      batchWithKeptOrClippedCellsMask = cellBatchData.NumberOfCells > 0;
    }
  };

  template <bool Invert>
  class ExtractEdges : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    explicit ExtractEdges(vtkm::Float64 isoValue)
      : IsoValue(isoValue)
    {
    }

    using ControlSignature = void(FieldIn cellBatch,
                                  FieldIn cellBatchDataOffsets,
                                  WholeCellSetIn<> cellSet,
                                  WholeArrayIn scalars,
                                  WholeArrayIn caseIndices,
                                  WholeArrayOut edges);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

    using MaskType = vtkm::worklet::MaskSelect;

    using CT = internal::ClipTables<Invert>;

    template <typename BatchType,
              typename CellSetType,
              typename PointScalars,
              typename CaseIndices,
              typename EdgesArray>
    VTKM_EXEC void operator()(const BatchType& cellBatch,
                              const CellBatchData& cellBatchDataOffsets,
                              const CellSetType& cellSet,
                              const PointScalars& scalars,
                              const CaseIndices& caseIndices,
                              EdgesArray& edges) const
    {
      namespace CTI = vtkm::worklet::internal::ClipTablesInformation;
      vtkm::Id edgeOffset = cellBatchDataOffsets.NumberOfEdges;

      for (vtkm::IdComponent id = 0, size = cellBatch.GetNumberOfComponents(); id < size; ++id)
      {
        const vtkm::Id& cellId = cellBatch[id];
        const vtkm::UInt8 caseIndex = caseIndices.Get(cellId);

        if (caseIndex != CT::GetDiscardedCellCase() &&
            caseIndex != CT::GetKeptCellCase()) // clipped cell
        {
          const auto shape = cellSet.GetCellShape(cellId);
          const auto points = cellSet.GetIndices(cellId);

          // only clipped cells have edges
          vtkm::Id index = CT::GetCaseIndex(shape.Id, caseIndex);
          const vtkm::UInt8 numberOfShapes = CT::ValueAt(index++);

          for (vtkm::IdComponent shapeId = 0; shapeId < numberOfShapes; shapeId++)
          {
            /*vtkm::UInt8 cellShape = */ CT::ValueAt(index++);
            const vtkm::UInt8 numberOfCellIndices = CT::ValueAt(index++);

            for (vtkm::IdComponent pointId = 0; pointId < numberOfCellIndices; pointId++, index++)
            {
              // Find how many points need to be calculated using edge interpolation.
              const vtkm::UInt8 pointIndex = CT::ValueAt(index);
              if (pointIndex >= CTI::E00 && pointIndex <= CTI::E11)
              {
                typename CT::EdgeVec edge = CT::GetEdge(shape.Id, pointIndex - CTI::E00);
                EdgeInterpolation ei;
                ei.Vertex1 = points[edge[0]];
                ei.Vertex2 = points[edge[1]];
                // For consistency purposes keep the points ordered.
                if (ei.Vertex1 > ei.Vertex2)
                {
                  vtkm::Swap(ei.Vertex1, ei.Vertex2);
                }
                ei.Weight = (static_cast<vtkm::Float64>(scalars.Get(ei.Vertex1)) - this->IsoValue) /
                  static_cast<vtkm::Float64>(scalars.Get(ei.Vertex2) - scalars.Get(ei.Vertex1));
                // Add edge to the list of edges.
                edges.Set(edgeOffset++, ei);
              }
            }
          }
        }
      }
    }

  private:
    vtkm::Float64 IsoValue;
  };

  template <bool Invert>
  class GenerateCellSet : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    GenerateCellSet(vtkm::Id edgePointsOffset, vtkm::Id centroidPointsOffset)
      : EdgePointsOffset(edgePointsOffset)
      , CentroidPointsOffset(centroidPointsOffset)
    {
    }

    using ControlSignature = void(FieldIn cellBatch,
                                  FieldIn cellBatchDataOffsets,
                                  WholeCellSetIn<> cellSet,
                                  WholeArrayIn caseIndices,
                                  WholeArrayIn pointMapOutputToInput,
                                  WholeArrayIn edgeIndexToUnique,
                                  WholeArrayOut centroidOffsets,
                                  WholeArrayOut centroidConnectivity,
                                  WholeArrayOut cellMapOutputToInput,
                                  WholeArrayOut shapes,
                                  WholeArrayOut offsets,
                                  WholeArrayOut connectivity);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12);

    using MaskType = vtkm::worklet::MaskSelect;

    using CT = internal::ClipTables<Invert>;

    template <typename BatchType,
              typename CellSetType,
              typename CaseIndices,
              typename PointMapInputToOutput,
              typename EdgeIndexToUnique,
              typename CentroidOffsets,
              typename CentroidConnectivity,
              typename CellMapOutputToInput,
              typename Shapes,
              typename Offsets,
              typename Connectivity>
    VTKM_EXEC void operator()(const BatchType& cellBatch,
                              const CellBatchData& cellBatchDataOffsets,
                              const CellSetType& cellSet,
                              const CaseIndices& caseIndices,
                              const PointMapInputToOutput pointMapInputToOutput,
                              const EdgeIndexToUnique& edgeIndexToUnique,
                              CentroidOffsets& centroidOffsets,
                              CentroidConnectivity& centroidConnectivity,
                              CellMapOutputToInput& cellMapOutputToInput,
                              Shapes& shapes,
                              Offsets& offsets,
                              Connectivity& connectivity) const
    {
      namespace CTI = vtkm::worklet::internal::ClipTablesInformation;
      vtkm::Id cellsOffset = cellBatchDataOffsets.NumberOfCells;
      vtkm::Id cellIndicesOffset = cellBatchDataOffsets.NumberOfCellIndices;
      vtkm::Id edgeOffset = cellBatchDataOffsets.NumberOfEdges;
      vtkm::Id centroidOffset = cellBatchDataOffsets.NumberOfCentroids;
      vtkm::Id centroidIndicesOffset = cellBatchDataOffsets.NumberOfCentroidIndices;

      for (vtkm::IdComponent id = 0, size = cellBatch.GetNumberOfComponents(); id < size; ++id)
      {
        const vtkm::Id& cellId = cellBatch[id];
        const vtkm::UInt8 caseIndex = caseIndices.Get(cellId);
        if (caseIndex != CT::GetDiscardedCellCase()) // not discarded cell
        {
          const auto shape = cellSet.GetCellShape(cellId);
          const auto points = cellSet.GetIndices(cellId);
          if (caseIndex == CT::GetKeptCellCase()) // kept cell
          {
            cellMapOutputToInput.Set(cellsOffset, cellId);
            shapes.Set(cellsOffset, static_cast<vtkm::UInt8>(shape.Id));
            offsets.Set(cellsOffset, cellIndicesOffset);
            for (vtkm::IdComponent pointId = 0; pointId < points.GetNumberOfComponents(); ++pointId)
            {
              connectivity.Set(cellIndicesOffset++, pointMapInputToOutput.Get(points[pointId]));
            }
          }
          else // clipped cell
          {
            vtkm::Id centroidIndex = 0;

            vtkm::Id index = CT::GetCaseIndex(shape.Id, caseIndex);
            const vtkm::UInt8 numberOfShapes = CT::ValueAt(index++);

            for (vtkm::IdComponent shapeId = 0; shapeId < numberOfShapes; shapeId++)
            {
              const vtkm::UInt8 cellShape = CT::ValueAt(index++);
              const vtkm::UInt8 numberOfCellIndices = CT::ValueAt(index++);

              if (cellShape != CTI::ST_PNT) // normal cell
              {
                // Store the cell data
                cellMapOutputToInput.Set(cellsOffset, cellId);
                shapes.Set(cellsOffset, cellShape);
                offsets.Set(cellsOffset++, cellIndicesOffset);

                for (vtkm::IdComponent pointId = 0; pointId < numberOfCellIndices;
                     pointId++, index++)
                {
                  // Find how many points need to be calculated using edge interpolation.
                  const vtkm::UInt8 pointIndex = CT::ValueAt(index);
                  if (pointIndex <= CTI::P7) // Input Point
                  {
                    // We know pt P0 must be > P0 since we already
                    // assume P0 == 0.  This is why we do not
                    // bother subtracting P0 from pt here.
                    connectivity.Set(cellIndicesOffset++,
                                     pointMapInputToOutput.Get(points[pointIndex]));
                  }
                  else if (/*pointIndex >= CTI::E00 &&*/ pointIndex <= CTI::E11) // Mid-Edge Point
                  {
                    connectivity.Set(cellIndicesOffset++,
                                     this->EdgePointsOffset + edgeIndexToUnique.Get(edgeOffset++));
                  }
                  else // pointIndex == CTI::N0 // Centroid Point
                  {
                    connectivity.Set(cellIndicesOffset++, centroidIndex);
                  }
                }
              }
              else // cellShape == CTI::ST_PNT
              {
                // Store the centroid data
                centroidIndex = this->CentroidPointsOffset + centroidOffset;
                centroidOffsets.Set(centroidOffset++, centroidIndicesOffset);

                for (vtkm::IdComponent pointId = 0; pointId < numberOfCellIndices;
                     pointId++, index++)
                {
                  // Find how many points need to be calculated using edge interpolation.
                  const vtkm::UInt8 pointIndex = CT::ValueAt(index);
                  if (pointIndex <= CTI::P7) // Input Point
                  {
                    // We know pt P0 must be > P0 since we already
                    // assume P0 == 0.  This is why we do not
                    // bother subtracting P0 from pt here.
                    centroidConnectivity.Set(centroidIndicesOffset++,
                                             pointMapInputToOutput.Get(points[pointIndex]));
                  }
                  else /*pointIndex >= CTI::E00 && pointIndex <= CTI::E11*/ // Mid-Edge Point
                  {
                    centroidConnectivity.Set(centroidIndicesOffset++,
                                             this->EdgePointsOffset +
                                               edgeIndexToUnique.Get(edgeOffset++));
                  }
                }
              }
            }
          }
        }
      }
    }

  private:
    vtkm::Id EdgePointsOffset;
    vtkm::Id CentroidPointsOffset;
  };

  Clip() = default;

  template <bool Invert, typename CellSetType, typename ScalarsArrayHandle>
  vtkm::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                    const ScalarsArrayHandle& scalars,
                                    vtkm::Float64 value)
  {
    const vtkm::Id numberOfInputPoints = scalars.GetNumberOfValues();
    const vtkm::Id numberOfInputCells = cellSet.GetNumberOfCells();

    // Create an invoker.
    vtkm::cont::Invoker invoke;

    // Create batches of points to process.
    auto pointBatches = CreateBatches(numberOfInputPoints);

    // Create an array to store the point batch statistics.
    vtkm::cont::ArrayHandle<PointBatchData> pointBatchesData;
    pointBatchesData.Allocate(pointBatches.GetNumberOfValues());

    // Create a mask to only process the batches that have kept points.
    vtkm::cont::ArrayHandle<vtkm::UInt8> batchesWithKeptPointsMask;
    batchesWithKeptPointsMask.Allocate(pointBatches.GetNumberOfValues());

    // Create an array to store the mask of kept points.
    vtkm::cont::ArrayHandle<vtkm::UInt8> keptPointsMask;
    keptPointsMask.Allocate(numberOfInputPoints);

    // Mark the points that are kept.
    invoke(MarkKeptPoints<Invert>(value),
           pointBatches,
           pointBatchesData,
           batchesWithKeptPointsMask,
           scalars,
           keptPointsMask);

    // Compute the total of pointBatchesData, and convert pointBatchesData to offsets in-place.
    const PointBatchData pointBatchTotal = vtkm::cont::Algorithm::ScanExclusive(
      pointBatchesData, pointBatchesData, PointBatchData::SumOp(), PointBatchData{});

    // Create arrays to store the point map from input to output, and output to input.
    vtkm::cont::ArrayHandle<vtkm::Id> pointMapInputToOutput;
    pointMapInputToOutput.Allocate(numberOfInputPoints);
    this->PointMapOutputToInput.Allocate(pointBatchTotal.NumberOfKeptPoints);

    // Compute the point map from input to output, and output to input. (see Scatter Counting)
    invoke(ComputePointMaps(),
           vtkm::worklet::MaskSelect(batchesWithKeptPointsMask),
           pointBatches,
           pointBatchesData, // pointBatchesDataOffsets
           keptPointsMask,
           pointMapInputToOutput,
           this->PointMapOutputToInput);
    // Release pointBatches related arrays since they are no longer needed.
    pointBatches.ReleaseResources();
    pointBatchesData.ReleaseResources();
    batchesWithKeptPointsMask.ReleaseResources();

    // Create batches of cells to process.
    auto cellBatches = CreateBatches(numberOfInputCells);

    // Create an array to store the cell batch statistics.
    vtkm::cont::ArrayHandle<CellBatchData> cellBatchesData;
    cellBatchesData.Allocate(cellBatches.GetNumberOfValues());

    // Create a mask to only process the batches that have clipped cells, to extract the edges.
    vtkm::cont::ArrayHandle<vtkm::UInt8> batchesWithClippedCellsMask;
    batchesWithClippedCellsMask.Allocate(cellBatches.GetNumberOfValues());

    // Create a mask to only process the batches that have kept or clipped cells.
    vtkm::cont::ArrayHandle<vtkm::UInt8> batchesWithKeptOrClippedCellsMask;
    batchesWithKeptOrClippedCellsMask.Allocate(cellBatches.GetNumberOfValues());

    // Create an array to save the caseIndex for each cell.
    vtkm::cont::ArrayHandle<vtkm::UInt8> caseIndices;
    caseIndices.Allocate(numberOfInputCells);

    // Compute the cell statistics of the clip operation.
    invoke(ComputeCellStats<Invert>(),
           cellBatches,
           cellBatchesData,
           batchesWithClippedCellsMask,
           batchesWithKeptOrClippedCellsMask,
           cellSet,
           keptPointsMask,
           caseIndices);
    keptPointsMask.ReleaseResources(); // Release keptPointsMask since it's no longer needed.

    // Compute the total of cellBatchesData, and convert cellBatchesData to offsets in-place.
    const CellBatchData cellBatchTotal = vtkm::cont::Algorithm::ScanExclusive(
      cellBatchesData, cellBatchesData, CellBatchData::SumOp(), CellBatchData{});

    // Create an array to store the edge interpolations.
    vtkm::cont::ArrayHandle<EdgeInterpolation> edgeInterpolation;
    edgeInterpolation.Allocate(cellBatchTotal.NumberOfEdges);

    // Extract the edges.
    invoke(ExtractEdges<Invert>(value),
           vtkm::worklet::MaskSelect(batchesWithClippedCellsMask),
           cellBatches,
           cellBatchesData, // cellBatchesDataOffsets
           cellSet,
           scalars,
           caseIndices,
           edgeInterpolation);
    // Release batchesWithClippedCellsMask since it's no longer needed.
    batchesWithClippedCellsMask.ReleaseResources();

    // Sort the edge interpolations.
    vtkm::cont::Algorithm::Sort(edgeInterpolation, EdgeInterpolation::LessThanOp());
    // Copy the edge interpolations to the output.
    vtkm::cont::Algorithm::Copy(edgeInterpolation, this->EdgePointsInterpolation);
    // Remove duplicates.
    vtkm::cont::Algorithm::Unique(this->EdgePointsInterpolation, EdgeInterpolation::EqualToOp());
    // Get the edge index to unique index.
    vtkm::cont::ArrayHandle<vtkm::Id> edgeInterpolationIndexToUnique;
    vtkm::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
                                       edgeInterpolation,
                                       edgeInterpolationIndexToUnique,
                                       EdgeInterpolation::LessThanOp());
    edgeInterpolation.ReleaseResources(); // Release edgeInterpolation since it's no longer needed.

    // Get the number of kept points, unique edge points, centroids, and output points.
    const vtkm::Id numberOfKeptPoints = this->PointMapOutputToInput.GetNumberOfValues();
    const vtkm::Id numberOfUniqueEdgePoints = this->EdgePointsInterpolation.GetNumberOfValues();
    const vtkm::Id numberOfCentroids = cellBatchTotal.NumberOfCentroids;
    const vtkm::Id numberOfOutputPoints =
      numberOfKeptPoints + numberOfUniqueEdgePoints + numberOfCentroids;
    // Create the offsets to write the point indices.
    this->EdgePointsOffset = numberOfKeptPoints;
    this->CentroidPointsOffset = this->EdgePointsOffset + numberOfUniqueEdgePoints;

    // Allocate the centroids.
    vtkm::cont::ArrayHandle<vtkm::Id> centroidOffsets;
    centroidOffsets.Allocate(numberOfCentroids + 1);
    vtkm::cont::ArrayHandle<vtkm::Id> centroidConnectivity;
    centroidConnectivity.Allocate(cellBatchTotal.NumberOfCentroidIndices);
    this->CentroidPointsInterpolation =
      vtkm::cont::make_ArrayHandleGroupVecVariable(centroidConnectivity, centroidOffsets);

    // Allocate the output cell set.
    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
    shapes.Allocate(cellBatchTotal.NumberOfCells);
    vtkm::cont::ArrayHandle<vtkm::Id> offsets;
    offsets.Allocate(cellBatchTotal.NumberOfCells + 1);
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    connectivity.Allocate(cellBatchTotal.NumberOfCellIndices);

    // Allocate Cell Map output to Input.
    this->CellMapOutputToInput.Allocate(cellBatchTotal.NumberOfCells);

    // Generate the output cell set.
    invoke(GenerateCellSet<Invert>(this->EdgePointsOffset, this->CentroidPointsOffset),
           vtkm::worklet::MaskSelect(batchesWithKeptOrClippedCellsMask),
           cellBatches,
           cellBatchesData, // cellBatchesDataOffsets
           cellSet,
           caseIndices,
           pointMapInputToOutput,
           edgeInterpolationIndexToUnique,
           centroidOffsets,
           centroidConnectivity,
           this->CellMapOutputToInput,
           shapes,
           offsets,
           connectivity);
    // All no longer needed arrays will be released at the end of this function.

    // Set the last offset to the size of the connectivity.
    vtkm::cont::ArraySetValue(
      cellBatchTotal.NumberOfCells, cellBatchTotal.NumberOfCellIndices, offsets);
    vtkm::cont::ArraySetValue(
      numberOfCentroids, cellBatchTotal.NumberOfCentroidIndices, centroidOffsets);

    vtkm::cont::CellSetExplicit<> output;
    output.Fill(numberOfOutputPoints, shapes, connectivity, offsets);
    return output;
  }

  template <bool Invert, typename CellSetType, typename ImplicitFunction>
  class ClipWithImplicitFunction
  {
  public:
    VTKM_CONT
    ClipWithImplicitFunction(Clip* clipper,
                             const CellSetType& cellSet,
                             const ImplicitFunction& function,
                             vtkm::Float64 offset,
                             vtkm::cont::CellSetExplicit<>* result)
      : Clipper(clipper)
      , CellSet(&cellSet)
      , Function(function)
      , Offset(offset)
      , Result(result)
    {
    }

    template <typename ArrayHandleType>
    VTKM_CONT void operator()(const ArrayHandleType& handle) const
    {
      // Evaluate the implicit function on the input coordinates using
      // ArrayHandleTransform
      vtkm::cont::ArrayHandleTransform<ArrayHandleType,
                                       vtkm::ImplicitFunctionValueFunctor<ImplicitFunction>>
        clipScalars(handle, this->Function);

      // Clip at locations where the implicit function evaluates to `Offset`
      *this->Result = Invert
        ? this->Clipper->template Run<true>(*this->CellSet, clipScalars, this->Offset)
        : this->Clipper->template Run<false>(*this->CellSet, clipScalars, this->Offset);
    }

  private:
    Clip* Clipper;
    const CellSetType* CellSet;
    ImplicitFunction Function;
    vtkm::Float64 Offset;
    vtkm::cont::CellSetExplicit<>* Result;
  };

  template <bool Invert, typename CellSetType, typename ImplicitFunction>
  vtkm::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                    const ImplicitFunction& clipFunction,
                                    vtkm::Float64 offset,
                                    const vtkm::cont::CoordinateSystem& coords)
  {
    vtkm::cont::CellSetExplicit<> output;

    ClipWithImplicitFunction<Invert, CellSetType, ImplicitFunction> clip(
      this, cellSet, clipFunction, offset, &output);

    CastAndCall(coords, clip);
    return output;
  }

  template <bool Invert, typename CellSetType, typename ImplicitFunction>
  vtkm::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                    const ImplicitFunction& clipFunction,
                                    const vtkm::cont::CoordinateSystem& coords)
  {
    return this->Run<Invert>(cellSet, clipFunction, 0.0, coords);
  }

  struct PerformEdgeInterpolations : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn edgeInterpolations,
                                  WholeArrayIn originalField,
                                  FieldOut outputField);
    using ExecutionSignature = void(_1, _2, _3);

    template <typename FieldPortal, typename T>
    VTKM_EXEC void operator()(const EdgeInterpolation& edgeInterp,
                              const FieldPortal& originalField,
                              T& output) const
    {
      const T v1 = originalField.Get(edgeInterp.Vertex1);
      const T v2 = originalField.Get(edgeInterp.Vertex2);

      // Interpolate per-vertex because some vec-like objects do not allow intermediate variables
      using VTraits = vtkm::VecTraits<T>;
      using CType = typename VTraits::ComponentType;
      VTKM_ASSERT(VTraits::GetNumberOfComponents(v1) == VTraits::GetNumberOfComponents(output));
      VTKM_ASSERT(VTraits::GetNumberOfComponents(v2) == VTraits::GetNumberOfComponents(output));
      for (vtkm::IdComponent component = 0; component < VTraits::GetNumberOfComponents(output);
           ++component)
      {
        const CType c1 = VTraits::GetComponent(v1, component);
        const CType c2 = VTraits::GetComponent(v2, component);
        const CType o = static_cast<CType>(((c1 - c2) * edgeInterp.Weight) + c1);
        VTraits::SetComponent(output, component, o);
      }
    }
  };

  struct PerformCentroidInterpolations : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn centroidInterpolation,
                                  WholeArrayIn outputField,
                                  FieldOut output);
    using ExecutionSignature = void(_1, _2, _3);

    template <typename CentroidInterpolation, typename OutputFieldArray, typename OutputFieldValue>
    VTKM_EXEC void operator()(const CentroidInterpolation& centroid,
                              const OutputFieldArray& outputField,
                              OutputFieldValue& output) const
    {
      const vtkm::IdComponent numValues = centroid.GetNumberOfComponents();

      // Interpolate per-vertex because some vec-like objects do not allow intermediate variables
      using VTraits = vtkm::VecTraits<OutputFieldValue>;
      using CType = typename VTraits::ComponentType;
      for (vtkm::IdComponent component = 0; component < VTraits::GetNumberOfComponents(output);
           ++component)
      {
        CType sum = VTraits::GetComponent(outputField.Get(centroid[0]), component);
        for (vtkm::IdComponent i = 1; i < numValues; ++i)
        {
          // static_cast is for when OutputFieldValue is a small int that gets promoted to int32.
          sum = static_cast<CType>(sum +
                                   VTraits::GetComponent(outputField.Get(centroid[i]), component));
        }
        VTraits::SetComponent(output, component, static_cast<CType>(sum / numValues));
      }
    }
  };

  template <typename InputType, typename OutputType>
  void ProcessPointField(const InputType& input, OutputType& output)
  {
    const vtkm::Id numberOfKeptPoints = this->PointMapOutputToInput.GetNumberOfValues();
    const vtkm::Id numberOfEdgePoints = this->EdgePointsInterpolation.GetNumberOfValues();
    const vtkm::Id numberOfCentroidPoints = this->CentroidPointsInterpolation.GetNumberOfValues();

    output.Allocate(numberOfKeptPoints + numberOfEdgePoints + numberOfCentroidPoints);

    // Copy over the original values that are still part of the output.
    vtkm::cont::Algorithm::CopySubRange(
      vtkm::cont::make_ArrayHandlePermutation(this->PointMapOutputToInput, input),
      0,
      numberOfKeptPoints,
      output);

    // Interpolate all new points that lie on edges of the input mesh.
    vtkm::cont::Invoker invoke;
    invoke(PerformEdgeInterpolations(),
           this->EdgePointsInterpolation,
           input,
           vtkm::cont::make_ArrayHandleView(output, this->EdgePointsOffset, numberOfEdgePoints));

    // interpolate all new points that lie as centroids of input meshes
    invoke(
      PerformCentroidInterpolations(),
      this->CentroidPointsInterpolation,
      output,
      vtkm::cont::make_ArrayHandleView(output, this->CentroidPointsOffset, numberOfCentroidPoints));
  }

  vtkm::cont::ArrayHandle<vtkm::Id> GetCellMapOutputToInput() const
  {
    return this->CellMapOutputToInput;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> PointMapOutputToInput;
  vtkm::cont::ArrayHandle<EdgeInterpolation> EdgePointsInterpolation;
  vtkm::cont::ArrayHandleGroupVecVariable<vtkm::cont::ArrayHandle<vtkm::Id>,
                                          vtkm::cont::ArrayHandle<vtkm::Id>>
    CentroidPointsInterpolation;
  vtkm::cont::ArrayHandle<vtkm::Id> CellMapOutputToInput;
  vtkm::Id EdgePointsOffset = 0;
  vtkm::Id CentroidPointsOffset = 0;
};
}
} // namespace vtkm::worklet

#if defined(THRUST_SCAN_WORKAROUND)
namespace thrust
{
namespace detail
{

// causes a different code path which does not have the bug
template <>
struct is_integral<vtkm::worklet::CellBatchesData> : public true_type
{
};
}
} // namespace thrust::detail
#endif

#endif // vtkm_m_worklet_Clip_h
