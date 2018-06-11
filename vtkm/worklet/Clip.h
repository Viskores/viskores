//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_m_worklet_Clip_h
#define vtkm_m_worklet_Clip_h

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/internal/ClipTables.h>

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ImplicitFunctionHandle.h>
#include <vtkm/cont/Timer.h>

#include <utility>
#include <vtkm/exec/FunctorBase.h>

#if defined(THRUST_MAJOR_VERSION) && THRUST_MAJOR_VERSION == 1 && THRUST_MINOR_VERSION == 8 &&     \
  THRUST_SUBMINOR_VERSION < 3
// Workaround a bug in thrust 1.8.0 - 1.8.2 scan implementations which produces
// wrong results
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/detail/type_traits.h>
VTKM_THIRDPARTY_POST_INCLUDE
#define THRUST_SCAN_WORKAROUND
#endif

namespace vtkm
{
namespace worklet
{
struct ClipStats
{
  vtkm::Id NumberOfCells = 0;
  vtkm::Id NumberOfIndices = 0;
  vtkm::Id NumberOfNewPoints = 0;

  struct SumOp
  {
    VTKM_EXEC_CONT
    ClipStats operator()(const ClipStats& cs1, const ClipStats& cs2) const
    {
      ClipStats sum = cs1;
      sum.NumberOfCells += cs2.NumberOfCells;
      sum.NumberOfIndices += cs2.NumberOfIndices;
      sum.NumberOfNewPoints += cs2.NumberOfNewPoints;
      return sum;
    }
  };
};
namespace internal
{


template <typename T>
VTKM_EXEC_CONT T Scale(const T& val, vtkm::Float64 scale)
{
  return static_cast<T>(scale * static_cast<vtkm::Float64>(val));
}

template <typename T, vtkm::IdComponent NumComponents>
VTKM_EXEC_CONT vtkm::Vec<T, NumComponents> Scale(const vtkm::Vec<T, NumComponents>& val,
                                                 vtkm::Float64 scale)
{
  return val * scale;
}

template <typename Device>
class ExecutionObject
{
private:
  using UInt8Portal =
    typename vtkm::cont::ArrayHandle<vtkm::UInt8>::template ExecutionTypes<Device>::Portal;

  using IdComponentPortal =
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::template ExecutionTypes<Device>::Portal;

  using IdPortal =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<Device>::Portal;

public:
  VTKM_CONT
  ExecutionObject()
    : Shapes()
    , NumIndices()
    , Connectivity()
    , IndexOffsets()
  {
  }

  VTKM_CONT
  ExecutionObject(vtkm::cont::ArrayHandle<vtkm::UInt8> shapes,
                  vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices,
                  vtkm::cont::ArrayHandle<vtkm::Id> connectivity,
                  vtkm::cont::ArrayHandle<vtkm::Id> cellToConnectivityMap,
                  vtkm::worklet::ClipStats total)
  {
    this->Shapes = shapes.PrepareForOutput(total.NumberOfCells, Device());
    this->NumIndices = numIndices.PrepareForOutput(total.NumberOfCells, Device());
    this->Connectivity = connectivity.PrepareForOutput(total.NumberOfIndices, Device());
    this->IndexOffsets = cellToConnectivityMap.PrepareForOutput(total.NumberOfCells, Device());
  }
  VTKM_EXEC
  void SetCellShape(vtkm::Id cellIndex, vtkm::UInt8 shape) { this->Shapes.Set(cellIndex, shape); }

  VTKM_EXEC
  void SetNumberOfIndices(vtkm::Id cellIndex, vtkm::IdComponent numIndices)
  {
    this->NumIndices.Set(cellIndex, numIndices);
  }

  VTKM_EXEC
  void SetIndexOffset(vtkm::Id cellIndex, vtkm::Id indexOffset)
  {
    this->IndexOffsets.Set(cellIndex, indexOffset);
  }

  VTKM_EXEC
  void SetConnectivity(vtkm::Id connectivityIndex, vtkm::Id pointIndex)
  {
    this->Connectivity.Set(connectivityIndex, pointIndex);
  }

private:
  UInt8Portal Shapes;
  IdComponentPortal NumIndices;
  IdPortal Connectivity;
  IdPortal IndexOffsets;
};

class ExecutionConnectivityExplicit : vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_CONT
  ExecutionConnectivityExplicit()
    : Shapes()
    , NumIndices()
    , Connectivity()
    , CellToConnectivityMap()
    , Total()
  {
  }

  VTKM_CONT
  ExecutionConnectivityExplicit(const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                                const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
                                const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
                                const vtkm::cont::ArrayHandle<vtkm::Id>& cellToConnectivityMap,
                                const vtkm::worklet::ClipStats& total)
    : Shapes(shapes)
    , NumIndices(numIndices)
    , Connectivity(connectivity)
    , CellToConnectivityMap(cellToConnectivityMap)
    , Total(total)
  {
  }

  template <typename Device>
  VTKM_CONT ExecutionObject<Device> PrepareForExecution(Device) const
  {
    ExecutionObject<Device> object(Shapes, NumIndices, Connectivity, CellToConnectivityMap, Total);
    return object;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::UInt8> Shapes;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumIndices;
  vtkm::cont::ArrayHandle<vtkm::Id> Connectivity;
  vtkm::cont::ArrayHandle<vtkm::Id> CellToConnectivityMap;
  vtkm::worklet::ClipStats Total;
};

} // namespace internal

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

class Clip
{
public:
  struct TypeClipStats : vtkm::ListTagBase<ClipStats>
  {
  };

  template <typename DeviceAdapter>
  class ComputeStats : public vtkm::worklet::WorkletMapPointToCell
  {
    using ClipTablesPortal = internal::ClipTables::DevicePortal<DeviceAdapter>;

  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<ScalarAll> scalars,
                                  FieldOutCell<IdType> clipTableIdxs,
                                  FieldOutCell<TypeClipStats> stats);
    using ExecutionSignature = void(_2, CellShape, PointCount, _3, _4);

    VTKM_CONT
    ComputeStats(vtkm::Float64 value, const ClipTablesPortal& clipTables, bool invert)
      : Value(value)
      , ClipTables(clipTables)
      , Invert(invert)
    {
    }

    template <typename ScalarsVecType, typename CellShapeTag>
    VTKM_EXEC void operator()(const ScalarsVecType& scalars,
                              CellShapeTag shape,
                              vtkm::Id count,
                              vtkm::Id& clipTableIdx,
                              ClipStats& stats) const
    {
      (void)shape; // C4100 false positive workaround
      const vtkm::Id mask[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

      vtkm::Id caseId = 0;
      for (vtkm::IdComponent i = 0; i < count; ++i)
      {
        if (this->Invert)
        {
          caseId |= (static_cast<vtkm::Float64>(scalars[i]) <= this->Value) ? mask[i] : 0;
        }
        else
        {
          caseId |= (static_cast<vtkm::Float64>(scalars[i]) > this->Value) ? mask[i] : 0;
        }
      }

      vtkm::Id idx = this->ClipTables.GetCaseIndex(shape.Id, caseId);
      clipTableIdx = idx;

      vtkm::Id numberOfCells = this->ClipTables.ValueAt(idx++);
      vtkm::Id numberOfIndices = 0;
      vtkm::Id numberOfNewPoints = 0;
      for (vtkm::Id cell = 0; cell < numberOfCells; ++cell)
      {
        ++idx; // skip shape-id
        vtkm::Id npts = this->ClipTables.ValueAt(idx++);
        numberOfIndices += npts;
        while (npts--)
        {
          // value < 100 means a new point needs to be generated by clipping an edge
          numberOfNewPoints += (this->ClipTables.ValueAt(idx++) < 100) ? 1 : 0;
        }
      }

      stats.NumberOfCells = numberOfCells;
      stats.NumberOfIndices = numberOfIndices;
      stats.NumberOfNewPoints = numberOfNewPoints;
    }

  private:
    vtkm::Float64 Value;
    ClipTablesPortal ClipTables;
    bool Invert;
  };

  template <typename DeviceAdapter>
  class GenerateCellSet : public vtkm::worklet::WorkletMapPointToCell
  {
    using ClipTablesPortal = internal::ClipTables::DevicePortal<DeviceAdapter>;

  public:
    struct EdgeInterp : vtkm::ListTagBase<EdgeInterpolation>
    {
    };

    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<ScalarAll> scalars,
                                  FieldInCell<IdType> clipTableIdxs,
                                  FieldInCell<TypeClipStats> cellSetIdxs,
                                  ExecObject connectivityExplicit,
                                  WholeArrayInOut<EdgeInterp> interpolation,
                                  WholeArrayInOut<IdType> newPointsConnectivityReverseMap,
                                  WholeArrayOut<IdType> cellMapOutputToInput);
    using ExecutionSignature = void(CellShape, InputIndex, _2, FromIndices, _3, _4, _5, _6, _7, _8);

    VTKM_CONT
    GenerateCellSet(vtkm::Float64 value, const ClipTablesPortal clipTables)
      : Value(value)
      , ClipTables(clipTables)
    {
    }

    template <typename CellShapeTag,
              typename ScalarsVecType,
              typename IndicesVecType,
              typename ExecutinoObjectType,
              typename InterpolationWholeArrayType,
              typename ReverseMapWholeArrayType,
              typename CellMapType>
    VTKM_EXEC void operator()(CellShapeTag shape,
                              vtkm::Id inputCellIdx,
                              const ScalarsVecType& scalars,
                              const IndicesVecType& indices,
                              vtkm::Id clipTableIdx,
                              ClipStats cellSetIndices,
                              ExecutinoObjectType& connectivityExplicit,
                              InterpolationWholeArrayType& interpolation,
                              ReverseMapWholeArrayType& newPointsConnectivityReverseMap,
                              CellMapType& cellMap) const
    {
      (void)shape; //C4100 false positive workaround
      vtkm::Id idx = clipTableIdx;

      // index of first cell
      vtkm::Id cellIdx = cellSetIndices.NumberOfCells;
      // index of first cell in connectivity array
      vtkm::Id connectivityIdx = cellSetIndices.NumberOfIndices;
      // index of new points generated by first cell
      vtkm::Id newPtsIdx = cellSetIndices.NumberOfNewPoints;

      vtkm::Id numberOfCells = this->ClipTables.ValueAt(idx++);
      for (vtkm::Id cell = 0; cell < numberOfCells; ++cell, ++cellIdx)
      {
        cellMap.Set(cellIdx, inputCellIdx);
        connectivityExplicit.SetCellShape(cellIdx, this->ClipTables.ValueAt(idx++));
        vtkm::IdComponent numPoints = this->ClipTables.ValueAt(idx++);
        connectivityExplicit.SetNumberOfIndices(cellIdx, numPoints);
        connectivityExplicit.SetIndexOffset(cellIdx, connectivityIdx);

        for (vtkm::Id pt = 0; pt < numPoints; ++pt, ++idx)
        {
          vtkm::IdComponent entry = static_cast<vtkm::IdComponent>(this->ClipTables.ValueAt(idx));
          if (entry >= 100) // existing point
          {
            connectivityExplicit.SetConnectivity(connectivityIdx++, indices[entry - 100]);
          }
          else // edge, new point to be generated by cutting the edge
          {
            internal::ClipTables::EdgeVec edge = this->ClipTables.GetEdge(shape.Id, entry);
            // Sanity check to make sure the edge is valid.
            VTKM_ASSERT(edge[0] != 255);
            VTKM_ASSERT(edge[1] != 255);

            EdgeInterpolation ei;
            ei.Vertex1 = indices[edge[0]];
            ei.Vertex2 = indices[edge[1]];
            if (ei.Vertex1 > ei.Vertex2)
            {
              this->swap(ei.Vertex1, ei.Vertex2);
              this->swap(edge[0], edge[1]);
            }
            ei.Weight = (static_cast<vtkm::Float64>(scalars[edge[0]]) - this->Value) /
              static_cast<vtkm::Float64>(scalars[edge[0]] - scalars[edge[1]]);

            interpolation.Set(newPtsIdx, ei);
            newPointsConnectivityReverseMap.Set(newPtsIdx, connectivityIdx++);
            ++newPtsIdx;
          }
        }
      }
    }

    template <typename T>
    VTKM_EXEC void swap(T& v1, T& v2) const
    {
      T temp = v1;
      v1 = v2;
      v2 = temp;
    }

  private:
    vtkm::Float64 Value;
    ClipTablesPortal ClipTables;
  };

  // The following can be done using DeviceAdapterAlgorithm::LowerBounds followed by
  // a worklet for updating connectivity. We are going with a custom worklet, that
  // combines lower-bounds computation and connectivity update, because this is
  // currently faster and uses less memory.
  template <typename DeviceAdapter>
  class AmendConnectivity : public vtkm::exec::FunctorBase
  {
    using IdPortal =
      typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::Portal;
    using IdPortalConst = typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<
      DeviceAdapter>::PortalConst;
    using EdgeInterpolationPortalConst = typename vtkm::cont::ArrayHandle<
      EdgeInterpolation>::template ExecutionTypes<DeviceAdapter>::PortalConst;

  public:
    VTKM_CONT
    AmendConnectivity(EdgeInterpolationPortalConst newPoints,
                      EdgeInterpolationPortalConst uniqueNewPoints,
                      IdPortalConst newPointsConnectivityReverseMap,
                      vtkm::Id newPointsOffset,
                      IdPortal connectivity)
      : NewPoints(newPoints)
      , UniqueNewPoints(uniqueNewPoints)
      , NewPointsConnectivityReverseMap(newPointsConnectivityReverseMap)
      , NewPointsOffset(newPointsOffset)
      , Connectivity(connectivity)
    {
    }

    VTKM_EXEC
    void operator()(vtkm::Id idx) const
    {
      EdgeInterpolation current = this->NewPoints.Get(idx);
      typename EdgeInterpolation::LessThanOp lt;

      // find point index by looking up in the unique points array (binary search)
      vtkm::Id count = UniqueNewPoints.GetNumberOfValues();
      vtkm::Id first = 0;
      while (count > 0)
      {
        vtkm::Id step = count / 2;
        vtkm::Id mid = first + step;
        if (lt(this->UniqueNewPoints.Get(mid), current))
        {
          first = ++mid;
          count -= step + 1;
        }
        else
        {
          count = step;
        }
      }

      this->Connectivity.Set(this->NewPointsConnectivityReverseMap.Get(idx),
                             this->NewPointsOffset + first);
    }

  private:
    EdgeInterpolationPortalConst NewPoints;
    EdgeInterpolationPortalConst UniqueNewPoints;
    IdPortalConst NewPointsConnectivityReverseMap;
    vtkm::Id NewPointsOffset;
    IdPortal Connectivity;
  };

  Clip()
    : ClipTablesInstance()
    , NewPointsInterpolation()
    , NewPointsOffset()
  {
  }

  template <typename CellSetList, typename ScalarsArrayHandle, typename DeviceAdapter>
  vtkm::cont::CellSetExplicit<> Run(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
                                    const ScalarsArrayHandle& scalars,
                                    vtkm::Float64 value,
                                    bool invert,
                                    DeviceAdapter device)
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    using ClipTablesPortal = internal::ClipTables::DevicePortal<DeviceAdapter>;
    ClipTablesPortal clipTablesDevicePortal = this->ClipTablesInstance.GetDevicePortal(device);

    // Step 1. compute counts for the elements of the cell set data structure
    vtkm::cont::ArrayHandle<vtkm::Id> clipTableIdxs;
    vtkm::cont::ArrayHandle<ClipStats> stats;

    ComputeStats<DeviceAdapter> computeStats(value, clipTablesDevicePortal, invert);
    DispatcherMapTopology<ComputeStats<DeviceAdapter>, DeviceAdapter>(computeStats)
      .Invoke(cellSet, scalars, clipTableIdxs, stats);

    // compute offsets for each invocation
    ClipStats zero;
    vtkm::cont::ArrayHandle<ClipStats> cellSetIndices;
    ClipStats total = Algorithm::ScanExclusive(stats, cellSetIndices, ClipStats::SumOp(), zero);
    stats.ReleaseResources();

    // Step 2. generate the output cell set
    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    vtkm::cont::ArrayHandle<vtkm::Id> cellToConnectivityMap;
    internal::ExecutionConnectivityExplicit outConnectivity(
      shapes, numIndices, connectivity, cellToConnectivityMap, total);

    vtkm::cont::ArrayHandle<EdgeInterpolation> newPoints;
    newPoints.Allocate(total.NumberOfNewPoints);
    // reverse map from the new points to connectivity array
    vtkm::cont::ArrayHandle<vtkm::Id> newPointsConnectivityReverseMap;
    newPointsConnectivityReverseMap.Allocate(total.NumberOfNewPoints);

    this->CellIdMap.Allocate(total.NumberOfCells);

    GenerateCellSet<DeviceAdapter> generateCellSet(value, clipTablesDevicePortal);
    DispatcherMapTopology<GenerateCellSet<DeviceAdapter>, DeviceAdapter>(generateCellSet)
      .Invoke(cellSet,
              scalars,
              clipTableIdxs,
              cellSetIndices,
              outConnectivity,
              newPoints,
              newPointsConnectivityReverseMap,
              this->CellIdMap);
    cellSetIndices.ReleaseResources();

    // Step 3. remove duplicates from the list of new points
    vtkm::cont::ArrayHandle<vtkm::worklet::EdgeInterpolation> uniqueNewPoints;

    Algorithm::SortByKey(
      newPoints, newPointsConnectivityReverseMap, EdgeInterpolation::LessThanOp());
    Algorithm::Copy(newPoints, uniqueNewPoints);
    Algorithm::Unique(uniqueNewPoints, EdgeInterpolation::EqualToOp());

    this->NewPointsInterpolation = uniqueNewPoints;
    this->NewPointsOffset = scalars.GetNumberOfValues();

    // Step 4. update the connectivity array with indexes to the new, unique points
    AmendConnectivity<DeviceAdapter> computeNewPointsConnectivity(
      newPoints.PrepareForInput(device),
      uniqueNewPoints.PrepareForInput(device),
      newPointsConnectivityReverseMap.PrepareForInput(device),
      this->NewPointsOffset,
      connectivity.PrepareForInPlace(device));
    Algorithm::Schedule(computeNewPointsConnectivity, total.NumberOfNewPoints);

    vtkm::cont::CellSetExplicit<> output;
    output.Fill(this->NewPointsOffset + uniqueNewPoints.GetNumberOfValues(),
                shapes,
                numIndices,
                connectivity);

    return output;
  }

  template <typename DynamicCellSet, typename DeviceAdapter>
  class ClipWithImplicitFunction
  {
  public:
    VTKM_CONT
    ClipWithImplicitFunction(Clip* clipper,
                             const DynamicCellSet& cellSet,
                             const vtkm::ImplicitFunction* function,
                             const bool invert,
                             vtkm::cont::CellSetExplicit<>* result)
      : Clipper(clipper)
      , CellSet(&cellSet)
      , Function(function)
      , Invert(invert)
      , Result(result)
    {
    }

    template <typename ArrayHandleType>
    VTKM_CONT void operator()(const ArrayHandleType& handle) const
    {
      // Evaluate the implicit function on the input coordinates using
      // ArrayHandleTransform
      vtkm::cont::ArrayHandleTransform<ArrayHandleType, vtkm::ImplicitFunctionValue> clipScalars(
        handle, this->Function);

      // Clip at locations where the implicit function evaluates to 0
      *this->Result =
        this->Clipper->Run(*this->CellSet, clipScalars, 0.0, this->Invert, DeviceAdapter());
    }

  private:
    Clip* Clipper;
    const DynamicCellSet* CellSet;
    vtkm::ImplicitFunctionValue Function;
    bool Invert;
    vtkm::cont::CellSetExplicit<>* Result;
  };

  template <typename CellSetList, typename DeviceAdapter>
  vtkm::cont::CellSetExplicit<> Run(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
                                    const vtkm::cont::ImplicitFunctionHandle& clipFunction,
                                    const vtkm::cont::CoordinateSystem& coords,
                                    const bool invert,
                                    DeviceAdapter device)
  {
    vtkm::cont::CellSetExplicit<> output;

    ClipWithImplicitFunction<vtkm::cont::DynamicCellSetBase<CellSetList>, DeviceAdapter> clip(
      this, cellSet, clipFunction.PrepareForExecution(device), invert, &output);

    CastAndCall(coords, clip);
    return output;
  }

  template <typename ArrayHandleType, typename DeviceAdapter>
  class InterpolateField
  {
  public:
    using ValueType = typename ArrayHandleType::ValueType;

    template <typename T>
    class Kernel : public vtkm::exec::FunctorBase
    {
    public:
      using FieldPortal =
        typename vtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapter>::Portal;

      using EdgeInterpolationPortalConst = typename vtkm::cont::ArrayHandle<
        EdgeInterpolation>::template ExecutionTypes<DeviceAdapter>::PortalConst;

      VTKM_CONT
      Kernel(EdgeInterpolationPortalConst interpolation,
             vtkm::Id newPointsOffset,
             FieldPortal field)
        : Interpolation(interpolation)
        , NewPointsOffset(newPointsOffset)
        , Field(field)
      {
      }

      VTKM_EXEC
      void operator()(vtkm::Id idx) const
      {
        EdgeInterpolation ei = this->Interpolation.Get(idx);
        T v1 = Field.Get(ei.Vertex1);
        T v2 = Field.Get(ei.Vertex2);
        Field.Set(this->NewPointsOffset + idx,
                  static_cast<T>(internal::Scale(T(v2 - v1), ei.Weight) + v1));
      }

    private:
      EdgeInterpolationPortalConst Interpolation;
      vtkm::Id NewPointsOffset;
      FieldPortal Field;
    };

    VTKM_CONT
    InterpolateField(vtkm::cont::ArrayHandle<EdgeInterpolation> interpolationArray,
                     vtkm::Id newPointsOffset,
                     ArrayHandleType* output)
      : InterpolationArray(interpolationArray)
      , NewPointsOffset(newPointsOffset)
      , Output(output)
    {
    }

    template <typename Storage>
    VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<ValueType, Storage>& field) const
    {
      using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

      vtkm::Id count = this->InterpolationArray.GetNumberOfValues();

      ArrayHandleType result;
      result.Allocate(field.GetNumberOfValues() + count);
      Algorithm::CopySubRange(field, 0, field.GetNumberOfValues(), result);
      Kernel<ValueType> kernel(this->InterpolationArray.PrepareForInput(DeviceAdapter()),
                               this->NewPointsOffset,
                               result.PrepareForInPlace(DeviceAdapter()));

      Algorithm::Schedule(kernel, count);
      *(this->Output) = result;
    }

  private:
    vtkm::cont::ArrayHandle<EdgeInterpolation> InterpolationArray;
    vtkm::Id NewPointsOffset;
    ArrayHandleType* Output;
  };

  template <typename ValueType, typename StorageType, typename DeviceAdapter>
  vtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& fieldData,
    DeviceAdapter) const
  {
    using ResultType = vtkm::cont::ArrayHandle<ValueType>;
    using Worker = InterpolateField<ResultType, DeviceAdapter>;

    ResultType output;

    Worker worker = Worker(this->NewPointsInterpolation, this->NewPointsOffset, &output);
    worker(fieldData);

    return output;
  }

  template <typename ValueType, typename StorageType, typename DeviceAdapter>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& fieldData,
    DeviceAdapter) const
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->CellIdMap, fieldData);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    Algo::Copy(tmp, result);

    return result;
  }

private:
  internal::ClipTables ClipTablesInstance;
  vtkm::cont::ArrayHandle<EdgeInterpolation> NewPointsInterpolation;
  vtkm::cont::ArrayHandle<vtkm::Id> CellIdMap;
  vtkm::Id NewPointsOffset;
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
