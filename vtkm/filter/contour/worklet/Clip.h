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
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArraySetValues.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/filter/contour/worklet/clip/ClipTables.h>

#include <vtkm/worklet/MaskSelect.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>


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
  struct ClipStats
  {
    vtkm::Id NumberOfCells = 0;
    vtkm::Id NumberOfCellIndices = 0;
    vtkm::Id NumberOfEdges = 0;
    vtkm::Id NumberOfCentroids = 0;
    vtkm::Id NumberOfCentroidIndices = 0;

    struct SumOp
    {
      VTKM_EXEC_CONT
      ClipStats operator()(const ClipStats& stat1, const ClipStats& stat2) const
      {
        ClipStats sum = stat1;
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
    using ControlSignature = void(FieldIn scalar, FieldOut keptPointMask);
    using ExecutionSignature = _2(_1);

    VTKM_CONT
    explicit MarkKeptPoints(vtkm::Float64 isoValue)
      : IsoValue(isoValue)
    {
    }

    template <typename T>
    VTKM_EXEC vtkm::UInt8 operator()(const T& scalar) const
    {
      return Invert ? scalar < this->IsoValue : scalar >= this->IsoValue;
    }

  private:
    vtkm::Float64 IsoValue;
  };

  template <bool Invert>
  class ComputeClipStats : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellSet,
                                  FieldInPoint pointMask,
                                  FieldOutCell clipStat,
                                  FieldOutCell clippedMask,
                                  FieldOutCell keptOrClippedMask,
                                  FieldOutCell caseIndex);

    using ExecutionSignature = void(CellShape, PointCount, _2, _3, _4, _5, _6);

    using CT = internal::ClipTables<Invert>;

    template <typename CellShapeTag, typename KeptPointsMask>
    VTKM_EXEC void operator()(const CellShapeTag& shape,
                              const vtkm::IdComponent pointCount,
                              const KeptPointsMask& keptPointsMask,
                              ClipStats& clipStat,
                              vtkm::UInt8& clippedMask,
                              vtkm::UInt8& keptOrClippedMask,
                              vtkm::UInt8& caseIndex) const
    {
      namespace CTI = vtkm::worklet::internal::ClipTablesInformation;
      // compute case index
      caseIndex = 0;
      for (vtkm::IdComponent ptId = pointCount - 1; ptId >= 0; --ptId)
      {
        static constexpr auto InvertUint8 = static_cast<vtkm::UInt8>(Invert);
        caseIndex |= (InvertUint8 != keptPointsMask[ptId]) << ptId;
      }

      if (CT::IsCellDiscarded(pointCount, caseIndex)) // discarded cell
      {
        // we do that to determine if a cell is discarded using only the caseIndex
        caseIndex = CT::GetDiscardedCellCase();
      }
      else if (CT::IsCellKept(pointCount, caseIndex)) // kept cell
      {
        // we do that to determine if a cell is kept using only the caseIndex
        caseIndex = CT::GetKeptCellCase();
        clipStat.NumberOfCells = 1;
        clipStat.NumberOfCellIndices = pointCount;
      }
      else // clipped cell
      {
        vtkm::Id index = CT::GetCaseIndex(shape.Id, caseIndex);
        const vtkm::UInt8 numberOfShapes = CT::ValueAt(index++);

        clipStat.NumberOfCells = numberOfShapes;
        for (vtkm::IdComponent shapeId = 0; shapeId < numberOfShapes; ++shapeId)
        {
          const vtkm::UInt8 cellShape = CT::ValueAt(index++);
          const vtkm::UInt8 numberOfCellIndices = CT::ValueAt(index++);

          for (vtkm::IdComponent pointId = 0; pointId < numberOfCellIndices; pointId++, index++)
          {
            // Find how many points need to be calculated using edge interpolation.
            const vtkm::UInt8 pointIndex = CT::ValueAt(index);
            clipStat.NumberOfEdges += (pointIndex >= CTI::E00 && pointIndex <= CTI::E11);
          }
          if (cellShape != CTI::ST_PNT) // normal cell
          {
            // Collect number of indices required for storing current shape
            clipStat.NumberOfCellIndices += numberOfCellIndices;
          }
          else // cellShape == ST_PNT
          {
            --clipStat.NumberOfCells; // decrement since this is a centroid shape
            clipStat.NumberOfCentroids++;
            clipStat.NumberOfCentroidIndices += numberOfCellIndices;
          }
        }
      }
      keptOrClippedMask = caseIndex != CT::GetDiscardedCellCase();
      clippedMask = keptOrClippedMask && caseIndex != CT::GetKeptCellCase();
    }
  };

  template <bool Invert>
  class ExtractEdges : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VTKM_CONT
    explicit ExtractEdges(vtkm::Float64 isoValue)
      : IsoValue(isoValue)
    {
    }

    using ControlSignature = void(CellSetIn cellSet,
                                  FieldInPoint scalars,
                                  FieldInCell clipStatOffsets,
                                  FieldInCell caseIndex,
                                  WholeArrayOut edges);

    using ExecutionSignature = void(CellShape, PointIndices, _2, _3, _4, _5);

    using MaskType = vtkm::worklet::MaskSelect;

    using CT = internal::ClipTables<Invert>;

    template <typename CellShapeTag,
              typename PointIndicesVec,
              typename PointScalars,
              typename EdgesArray>
    VTKM_EXEC void operator()(const CellShapeTag& shape,
                              const PointIndicesVec& points,
                              const PointScalars& scalars,
                              const ClipStats& clipStat,
                              const vtkm::UInt8& caseIndex,
                              EdgesArray& edges) const
    {
      namespace CTI = vtkm::worklet::internal::ClipTablesInformation;
      vtkm::Id edgeOffset = clipStat.NumberOfEdges;

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
              vtkm::Swap(edge[0], edge[1]);
            }
            ei.Weight = (static_cast<vtkm::Float64>(scalars[edge[0]]) - this->IsoValue) /
              static_cast<vtkm::Float64>(scalars[edge[1]] - scalars[edge[0]]);
            // Add edge to the list of edges.
            edges.Set(edgeOffset++, ei);
          }
        }
      }
    }

  private:
    vtkm::Float64 IsoValue;
  };

  template <bool Invert>
  class GenerateCellSet : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VTKM_CONT
    GenerateCellSet(vtkm::Id edgePointsOffset, vtkm::Id centroidPointsOffset)
      : EdgePointsOffset(edgePointsOffset)
      , CentroidPointsOffset(centroidPointsOffset)
    {
    }

    using ControlSignature = void(CellSetIn cellSet,
                                  FieldInCell caseIndex,
                                  FieldInCell clipStatOffsets,
                                  WholeArrayIn pointMapOutputToInput,
                                  WholeArrayIn edgeIndexToUnique,
                                  WholeArrayOut centroidOffsets,
                                  WholeArrayOut centroidConnectivity,
                                  WholeArrayOut cellMapOutputToInput,
                                  WholeArrayOut shapes,
                                  WholeArrayOut offsets,
                                  WholeArrayOut connectivity);
    using ExecutionSignature =
      void(InputIndex, CellShape, PointIndices, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11);

    using MaskType = vtkm::worklet::MaskSelect;

    using CT = internal::ClipTables<Invert>;

    template <typename CellShapeTag,
              typename PointVecType,
              typename PointMapInputToOutput,
              typename EdgeIndexToUnique,
              typename CentroidOffsets,
              typename CentroidConnectivity,
              typename CellMapOutputToInput,
              typename Shapes,
              typename Offsets,
              typename Connectivity>
    VTKM_EXEC void operator()(vtkm::Id cellId,
                              const CellShapeTag& shape,
                              const PointVecType& points,
                              const vtkm::UInt8& caseIndex,
                              const ClipStats& clipStatOffsets,
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
      vtkm::Id cellsOffset = clipStatOffsets.NumberOfCells;
      vtkm::Id cellIndicesOffset = clipStatOffsets.NumberOfCellIndices;
      vtkm::Id edgeOffset = clipStatOffsets.NumberOfEdges;
      vtkm::Id centroidOffset = clipStatOffsets.NumberOfCentroids;
      vtkm::Id centroidIndicesOffset = clipStatOffsets.NumberOfCentroidIndices;

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

            for (vtkm::IdComponent pointId = 0; pointId < numberOfCellIndices; pointId++, index++)
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

            for (vtkm::IdComponent pointId = 0; pointId < numberOfCellIndices; pointId++, index++)
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

    // Create an array to store the mask of kept points.
    vtkm::cont::ArrayHandle<vtkm::UInt8> keptPointsMask;
    keptPointsMask.Allocate(numberOfInputPoints);

    // Mark the points that are kept.
    invoke(MarkKeptPoints<Invert>(value), scalars, keptPointsMask);

    // Create an array to save the caseIndex for each cell.
    vtkm::cont::ArrayHandle<vtkm::UInt8> caseIndices;
    caseIndices.Allocate(numberOfInputCells);

    // Create an array to store the statistics of the clip operation.
    vtkm::cont::ArrayHandle<ClipStats> clipStats;
    clipStats.Allocate(numberOfInputCells);

    // Create a mask to only process the cells that are clipped, to extract the edges.
    vtkm::cont::ArrayHandle<vtkm::UInt8> clippedMask;

    // Create a mask to only process the kept or clipped cells.
    vtkm::cont::ArrayHandle<vtkm::UInt8> keptOrClippedMask;

    // Compute the statistics of the clip operation.
    invoke(ComputeClipStats<Invert>(),
           cellSet,
           keptPointsMask,
           clipStats,
           clippedMask,
           keptOrClippedMask,
           caseIndices);

    // Create ScatterCounting on the keptPointsMask.
    vtkm::worklet::ScatterCounting scatterCullDiscardedPoints(keptPointsMask, true);
    auto pointMapInputToOutput = scatterCullDiscardedPoints.GetInputToOutputMap();
    this->PointMapOutputToInput = scatterCullDiscardedPoints.GetOutputToInputMap();
    keptPointsMask.ReleaseResources(); // Release keptPointsMask since it's no longer needed.

    // Compute the total of clipStats, and convert clipStats to offsets in-place.
    const ClipStats total =
      vtkm::cont::Algorithm::ScanExclusive(clipStats, clipStats, ClipStats::SumOp(), ClipStats{});

    // Create an array to store the edge interpolations.
    vtkm::cont::ArrayHandle<EdgeInterpolation> edgeInterpolation;
    edgeInterpolation.Allocate(total.NumberOfEdges);

    // Extract the edges.
    invoke(ExtractEdges<Invert>(value),
           vtkm::worklet::MaskSelect(clippedMask),
           cellSet,
           scalars,
           clipStats, // clipStatOffsets
           caseIndices,
           edgeInterpolation);
    clippedMask.ReleaseResources(); // Release clippedMask since it's no longer needed.

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
    const vtkm::Id numberOfCentroids = total.NumberOfCentroids;
    const vtkm::Id numberOfOutputPoints =
      numberOfKeptPoints + numberOfUniqueEdgePoints + numberOfCentroids;
    // Create the offsets to write the point indices.
    this->EdgePointsOffset = numberOfKeptPoints;
    this->CentroidPointsOffset = this->EdgePointsOffset + numberOfUniqueEdgePoints;

    // Allocate the centroids.
    vtkm::cont::ArrayHandle<vtkm::Id> centroidOffsets;
    centroidOffsets.Allocate(numberOfCentroids + 1);
    vtkm::cont::ArrayHandle<vtkm::Id> centroidConnectivity;
    centroidConnectivity.Allocate(total.NumberOfCentroidIndices);
    this->CentroidPointsInterpolation =
      vtkm::cont::make_ArrayHandleGroupVecVariable(centroidConnectivity, centroidOffsets);

    // Allocate the output cell set.
    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
    shapes.Allocate(total.NumberOfCells);
    vtkm::cont::ArrayHandle<vtkm::Id> offsets;
    offsets.Allocate(total.NumberOfCells + 1);
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    connectivity.Allocate(total.NumberOfCellIndices);

    // Allocate Cell Map output to Input.
    this->CellMapOutputToInput.Allocate(total.NumberOfCells);

    // Generate the output cell set.
    invoke(GenerateCellSet<Invert>(this->EdgePointsOffset, this->CentroidPointsOffset),
           vtkm::worklet::MaskSelect(keptOrClippedMask),
           cellSet,
           caseIndices,
           clipStats, // clipStatOffsets
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
    vtkm::cont::ArraySetValue(total.NumberOfCells, total.NumberOfCellIndices, offsets);
    vtkm::cont::ArraySetValue(numberOfCentroids, total.NumberOfCentroidIndices, centroidOffsets);

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
struct is_integral<vtkm::worklet::ClipStats> : public true_type
{
};
}
} // namespace thrust::detail
#endif

#endif // vtkm_m_worklet_Clip_h
