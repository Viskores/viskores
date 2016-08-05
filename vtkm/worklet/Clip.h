//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/ImplicitFunctions.h>

#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/exec/FunctorBase.h>

#if defined(THRUST_MAJOR_VERSION) && THRUST_MAJOR_VERSION == 1 && \
    THRUST_MINOR_VERSION == 8 && THRUST_SUBMINOR_VERSION < 3
// Workaround a bug in thrust 1.8.0 - 1.8.2 scan implementations which produces
// wrong results
#include <thrust/detail/type_traits.h>
#define THRUST_SCAN_WORKAROUND
#endif


namespace vtkm {
namespace worklet {

namespace internal {

// TODO: The following code is meant to be temporary until similar functionality is
// implemented elsewhere.
// The current DeviceAdapterAlgorithm::Copy resizes the destination array, and
// therefore, is not usable for our purpose here.
template <typename T, typename StorageIn, typename StorageOut, typename DeviceAdapter>
class CopyArray : public vtkm::exec::FunctorBase
{
public:
  typedef typename vtkm::cont::ArrayHandle<T, StorageIn>
      ::template ExecutionTypes<DeviceAdapter>::PortalConst SourcePortalType;
  typedef typename vtkm::cont::ArrayHandle<T, StorageOut>
      ::template ExecutionTypes<DeviceAdapter>::Portal DestPortalType;

  VTKM_CONT_EXPORT
  CopyArray(SourcePortalType input, DestPortalType output)
    : Input(input), Output(output)
  {
  }

  VTKM_EXEC_EXPORT
  void operator()(vtkm::Id idx) const
  {
    this->Output.Set(idx, this->Input.Get(idx));
  }

private:
  SourcePortalType Input;
  DestPortalType Output;
};

template <typename T, typename StorageIn, typename StorageOut, typename DeviceAdapter>
VTKM_CONT_EXPORT
void ResizeArrayHandle(const vtkm::cont::ArrayHandle<T, StorageIn> &input,
                       vtkm::Id size,
                       vtkm::cont::ArrayHandle<T, StorageOut> &output,
                       DeviceAdapter)
{
  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;
  output.Allocate(size);
  CopyArray<T, StorageIn, StorageOut, DeviceAdapter>
      copyArray(input.PrepareForInput(DeviceAdapter()),
                output.PrepareForInPlace(DeviceAdapter()));
  Algorithm::Schedule(copyArray, input.GetNumberOfValues());
}

template<typename T>
VTKM_EXEC_EXPORT
void swap(T &v1, T &v2)
{
  T temp = v1;
  v1 = v2;
  v2 = temp;
}

template <typename T>
VTKM_EXEC_CONT_EXPORT
T Scale(const T &val, vtkm::Float64 scale)
{
  return static_cast<T>(scale * static_cast<vtkm::Float64>(val));
}

template <typename T, vtkm::IdComponent NumComponents>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<T, NumComponents> Scale(const vtkm::Vec<T, NumComponents> &val,
                                  vtkm::Float64 scale)
{
  return val * scale;
}

template <typename DeviceAdapter>
class ExecutionConnectivityExplicit : vtkm::exec::ExecutionObjectBase
{
private:
  typedef typename vtkm::cont::ArrayHandle<vtkm::UInt8>
      ::template ExecutionTypes<DeviceAdapter>::Portal UInt8Portal;

  typedef typename vtkm::cont::ArrayHandle<vtkm::IdComponent>
      ::template ExecutionTypes<DeviceAdapter>::Portal IdComponentPortal;

  typedef typename vtkm::cont::ArrayHandle<vtkm::Id>
      ::template ExecutionTypes<DeviceAdapter>::Portal IdPortal;

public:
  VTKM_CONT_EXPORT
  ExecutionConnectivityExplicit()
    : Shapes(), NumIndices(), Connectivity(), IndexOffsets()
  {
  }

  VTKM_CONT_EXPORT
  ExecutionConnectivityExplicit(const UInt8Portal &shapes,
                                const IdComponentPortal &numIndices,
                                const IdPortal &connectivity,
                                const IdPortal &indexOffsets)
    : Shapes(shapes),
      NumIndices(numIndices),
      Connectivity(connectivity),
      IndexOffsets(indexOffsets)
  {
  }

  VTKM_EXEC_EXPORT
  void SetCellShape(vtkm::Id cellIndex, vtkm::UInt8 shape)
  {
    this->Shapes.Set(cellIndex, shape);
  }

  VTKM_EXEC_EXPORT
  void SetNumberOfIndices(vtkm::Id cellIndex, vtkm::IdComponent numIndices)
  {
    this->NumIndices.Set(cellIndex, numIndices);
  }

  VTKM_EXEC_EXPORT
  void SetIndexOffset(vtkm::Id cellIndex, vtkm::Id indexOffset)
  {
    this->IndexOffsets.Set(cellIndex, indexOffset);
  }

  VTKM_EXEC_EXPORT
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

} // namespace internal



struct ClipStats
{
  vtkm::Id NumberOfCells;
  vtkm::Id NumberOfIndices;
  vtkm::Id NumberOfNewPoints;

  struct SumOp
  {
    VTKM_EXEC_CONT_EXPORT
    ClipStats operator()(const ClipStats &cs1, const ClipStats &cs2) const
    {
      ClipStats sum = cs1;
      sum.NumberOfCells += cs2.NumberOfCells;
      sum.NumberOfIndices += cs2.NumberOfIndices;
      sum.NumberOfNewPoints += cs2.NumberOfNewPoints;
      return sum;
    }
  };
};

struct EdgeInterpolation
{
  vtkm::Id Vertex1, Vertex2;
  vtkm::Float64 Weight;

  struct LessThanOp
  {
    VTKM_EXEC_EXPORT
    bool operator()(const EdgeInterpolation &v1, const EdgeInterpolation &v2) const
    {
      return (v1.Vertex1 < v2.Vertex1) ||
             (v1.Vertex1 == v2.Vertex1 && v1.Vertex2 < v2.Vertex2);
    }
  };

  struct EqualToOp
  {
    VTKM_EXEC_EXPORT
    bool operator()(const EdgeInterpolation &v1, const EdgeInterpolation &v2) const
    {
      return v1.Vertex1 == v2.Vertex1 && v1.Vertex2 == v2.Vertex2;
    }
  };
};


VTKM_EXEC_CONT_EXPORT
std::ostream& operator<<(std::ostream &out, const ClipStats &val)
{
  return out << std::endl << "{ Cells: " << val.NumberOfCells
             << ", Indices: " << val.NumberOfIndices
             << ", NewPoints: " << val.NumberOfNewPoints << " }";
}

VTKM_EXEC_CONT_EXPORT
std::ostream& operator<<(std::ostream &out, const EdgeInterpolation &val)
{
  return out << std::endl << "{ Vertex1: " << val.Vertex1
             << ", Vertex2: " << val.Vertex2
             << ", Weight: " << val.Weight << " }";
}


template<typename DeviceAdapter>
class Clip
{
private:
  typedef internal::ClipTables::DevicePortal<DeviceAdapter> ClipTablesPortal;

  typedef typename vtkm::cont::ArrayHandle<vtkm::Id>
      ::template ExecutionTypes<DeviceAdapter>::Portal IdPortal;
  typedef typename vtkm::cont::ArrayHandle<vtkm::Id>
      ::template ExecutionTypes<DeviceAdapter>::PortalConst IdPortalConst;
  typedef typename vtkm::cont::ArrayHandle<EdgeInterpolation>
      ::template ExecutionTypes<DeviceAdapter>::Portal EdgeInterpolationPortal;
  typedef typename vtkm::cont::ArrayHandle<EdgeInterpolation>
      ::template ExecutionTypes<DeviceAdapter>::PortalConst
        EdgeInterpolationPortalConst;

  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;


public:
  struct TypeClipStats : vtkm::ListTagBase<ClipStats> { };


  class ComputeStats : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldInPoint<ScalarAll> scalars,
                                  FieldOutCell<IdType> clipTableIdxs,
                                  FieldOutCell<TypeClipStats> stats);
    typedef void ExecutionSignature(_2, CellShape, PointCount, _3, _4);

    VTKM_CONT_EXPORT
    ComputeStats(vtkm::Float64 value, const ClipTablesPortal &clipTables)
      : Value(value), ClipTables(clipTables)
    {
    }

    template<typename ScalarsVecType, typename CellShapeTag>
    VTKM_EXEC_EXPORT
    void operator()(const ScalarsVecType &scalars,
                    CellShapeTag shape,
                    vtkm::Id count,
                    vtkm::Id& clipTableIdx,
                    ClipStats& stats) const
    {
      const vtkm::Id mask[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

      vtkm::Id caseId = 0;
      for (vtkm::IdComponent i = 0; i < count; ++i)
      {
        caseId |= (static_cast<vtkm::Float64>(scalars[i]) > this->Value) ?
                  mask[i] : 0;
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
  };


  class GenerateCellSet : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldInPoint<ScalarAll> scalars,
                                  FieldInCell<IdType> clipTableIdxs,
                                  FieldInCell<TypeClipStats> cellSetIdxs,
                                  ExecObject connectivityExplicit,
                                  ExecObject interpolation,
                                  ExecObject newPointsConnectivityReverseMap);
    typedef void ExecutionSignature(CellShape, _2, FromIndices, _3, _4, _5, _6, _7);

    VTKM_CONT_EXPORT
    GenerateCellSet(vtkm::Float64 value, const ClipTablesPortal clipTables)
      : Value(value), ClipTables(clipTables)
    {
    }

    template<typename CellShapeTag,
             typename ScalarsVecType,
             typename IndicesVecType,
             typename Storage>
    VTKM_EXEC_EXPORT
    void operator()(
        CellShapeTag shape,
        const ScalarsVecType &scalars,
        const IndicesVecType &indices,
        vtkm::Id clipTableIdx,
        ClipStats cellSetIndices,
        internal::ExecutionConnectivityExplicit<DeviceAdapter> connectivityExplicit,
        vtkm::exec::ExecutionWholeArray<EdgeInterpolation, Storage, DeviceAdapter>
            interpolation,
        vtkm::exec::ExecutionWholeArray<vtkm::Id, Storage, DeviceAdapter>
            newPointsConnectivityReverseMap) const
    {
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
        connectivityExplicit.SetCellShape(cellIdx, this->ClipTables.ValueAt(idx++));
        vtkm::IdComponent numPoints = this->ClipTables.ValueAt(idx++);
        connectivityExplicit.SetNumberOfIndices(cellIdx, numPoints);
        connectivityExplicit.SetIndexOffset(cellIdx, connectivityIdx);

        for (vtkm::Id pt = 0; pt < numPoints; ++pt, ++idx)
        {
          vtkm::IdComponent entry =
              static_cast<vtkm::IdComponent>(this->ClipTables.ValueAt(idx));
          if (entry >= 100) // existing point
          {
            connectivityExplicit.SetConnectivity(connectivityIdx++,
                                                 indices[entry - 100]);
          }
          else // edge, new point to be generated by cutting the egde
          {
            internal::ClipTables::EdgeVec edge =
                this->ClipTables.GetEdge(shape.Id, entry);

            EdgeInterpolation ei;
            ei.Vertex1 = indices[edge[0]];
            ei.Vertex2 = indices[edge[1]];
            if (ei.Vertex1 > ei.Vertex2)
            {
              internal::swap(ei.Vertex1, ei.Vertex2);
              internal::swap(edge[0], edge[1]);
            }
            ei.Weight = (static_cast<vtkm::Float64>(scalars[edge[0]]) - this->Value) /
                        static_cast<vtkm::Float64>(scalars[edge[0]] - scalars[edge[1]]);

            interpolation.Set(newPtsIdx , ei);
            newPointsConnectivityReverseMap.Set(newPtsIdx, connectivityIdx++);
            ++newPtsIdx;
          }
        }
      }
    }

  private:
    vtkm::Float64 Value;
    ClipTablesPortal ClipTables;
  };


// The following can be done using DeviceAdapterAlgorithm::LowerBounds followed by
// a worklet for updating connectivity. We are going with a custom worklet, that
// combines lower-bounds computation and connectivity update, because this is
// currently faster and uses less memory.
  class AmendConnectivity : public vtkm::exec::FunctorBase
  {
  public:
    VTKM_CONT_EXPORT
    AmendConnectivity(EdgeInterpolationPortalConst newPoints,
                      EdgeInterpolationPortalConst uniqueNewPoints,
                      IdPortalConst newPointsConnectivityReverseMap,
                      vtkm::Id newPointsOffset,
                      IdPortal connectivity)
      : NewPoints(newPoints),
        UniqueNewPoints(uniqueNewPoints),
        NewPointsConnectivityReverseMap(newPointsConnectivityReverseMap),
        NewPointsOffset(newPointsOffset),
        Connectivity(connectivity)
    {
    }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id idx) const
    {
      EdgeInterpolation current = this->NewPoints.Get(idx);
      typename EdgeInterpolation::LessThanOp lt;

      // find point index by looking up in the unique points array (binary search)
      vtkm::Id count = UniqueNewPoints.GetNumberOfValues();
      vtkm::Id first = 0;
      while (count > 0)
      {
        vtkm::Id step = count/2;
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
    : ClipTablesInstance(), NewPointsInterpolation(), NewPointsOffset()
  {
  }

  template <typename ScalarsArrayHandle>
  vtkm::cont::CellSetExplicit<> Run(const vtkm::cont::DynamicCellSet &cellSet,
                                    const ScalarsArrayHandle &scalars,
                                    vtkm::Float64 value)
  {
    DeviceAdapter device;
    ClipTablesPortal clipTablesDevicePortal =
        this->ClipTablesInstance.GetDevicePortal(device);


    // Step 1. compute counts for the elements of the cell set data structure
    vtkm::cont::ArrayHandle<vtkm::Id> clipTableIdxs;
    vtkm::cont::ArrayHandle<ClipStats> stats;

    ComputeStats computeStats(value, clipTablesDevicePortal);
    DispatcherMapTopology<ComputeStats>(computeStats).Invoke(cellSet, scalars,
       clipTableIdxs, stats);

    // compute offsets for each invocation
    ClipStats zero = { 0, 0, 0 };
    vtkm::cont::ArrayHandle<ClipStats> cellSetIndices;
    ClipStats total = Algorithm::ScanExclusive(stats, cellSetIndices,
        ClipStats::SumOp(), zero);
    stats.ReleaseResources();


    // Step 2. generate the output cell set
    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    vtkm::cont::ArrayHandle<vtkm::Id> cellToConnectivityMap;
    internal::ExecutionConnectivityExplicit<DeviceAdapter> outConnectivity(
        shapes.PrepareForOutput(total.NumberOfCells, device),
        numIndices.PrepareForOutput(total.NumberOfCells, device),
        connectivity.PrepareForOutput(total.NumberOfIndices, device),
        cellToConnectivityMap.PrepareForOutput(total.NumberOfCells, device));

    vtkm::cont::ArrayHandle<EdgeInterpolation> newPoints;
    // reverse map from the new points to connectivity array
    vtkm::cont::ArrayHandle<vtkm::Id> newPointsConnectivityReverseMap;

    GenerateCellSet generateCellSet(value, clipTablesDevicePortal);

    DispatcherMapTopology<GenerateCellSet>(generateCellSet).Invoke(cellSet, scalars,
        clipTableIdxs, cellSetIndices, outConnectivity,
        vtkm::exec::ExecutionWholeArray<
            EdgeInterpolation,
            VTKM_DEFAULT_STORAGE_TAG,
            DeviceAdapter>(newPoints, total.NumberOfNewPoints),
        vtkm::exec::ExecutionWholeArray<
            vtkm::Id,
            VTKM_DEFAULT_STORAGE_TAG,
            DeviceAdapter>(newPointsConnectivityReverseMap, total.NumberOfNewPoints));
    cellSetIndices.ReleaseResources();

    // Step 3. remove duplicates from the list of new points
    vtkm::cont::ArrayHandle<vtkm::worklet::EdgeInterpolation> uniqueNewPoints;

    Algorithm::SortByKey(newPoints, newPointsConnectivityReverseMap,
                         EdgeInterpolation::LessThanOp());
    Algorithm::Copy(newPoints, uniqueNewPoints);
    Algorithm::Unique(uniqueNewPoints, EdgeInterpolation::EqualToOp());

    this->NewPointsInterpolation = uniqueNewPoints;
    this->NewPointsOffset = scalars.GetNumberOfValues();


    // Step 4. update the connectivity array with indexes to the new, unique points
    AmendConnectivity computeNewPointsConnectivity(
        newPoints.PrepareForInput(device),
        uniqueNewPoints.PrepareForInput(device),
        newPointsConnectivityReverseMap.PrepareForInput(device),
        this->NewPointsOffset,
        connectivity.PrepareForInPlace(device));
    Algorithm::Schedule(computeNewPointsConnectivity, total.NumberOfNewPoints);


    vtkm::cont::CellSetExplicit<> output;
    output.Fill(shapes, numIndices, connectivity);

    return output;
  }

  template<typename ImplicitFunction>
  class ClipWithImplicitFunction
  {
  public:
    VTKM_CONT_EXPORT
    ClipWithImplicitFunction(Clip *clipper, const vtkm::cont::DynamicCellSet &cellSet,
      ImplicitFunction function, vtkm::cont::CellSetExplicit<> *result)
      : Clipper(clipper), CellSet(&cellSet), Function(function), Result(result)
    { }

    template <typename ArrayHandleType>
    VTKM_CONT_EXPORT
    void operator()(const ArrayHandleType &handle) const
    {
      vtkm::cont::ArrayHandleTransform<
        vtkm::FloatDefault,
        ArrayHandleType,
        vtkm::ImplicitFunctionValue<ImplicitFunction> > clipScalars(handle,
                                                                    this->Function);

        *this->Result = this->Clipper->Run(*this->CellSet, clipScalars, 0.0);
    }

  private:
    Clip *Clipper;
    const vtkm::cont::DynamicCellSet *CellSet;
    const vtkm::ImplicitFunctionValue<ImplicitFunction> Function;
    vtkm::cont::CellSetExplicit<> *Result;
  };

  template <typename ImplicitFunction>
  vtkm::cont::CellSetExplicit<> Run(const vtkm::cont::DynamicCellSet &cellSet,
                                    const ImplicitFunction &clipFunction,
                                    const vtkm::cont::CoordinateSystem &coords)
  {
    vtkm::cont::CellSetExplicit<> output;
    ClipWithImplicitFunction<ImplicitFunction> clip(this, cellSet, clipFunction,
                                                    &output);
    CastAndCall(coords, clip);

    return output;
  }

  class InterpolateField
  {
  public:
    template <typename T>
    class Kernel : public vtkm::exec::FunctorBase
    {
    public:
      typedef typename vtkm::cont::ArrayHandle<T>
        ::template ExecutionTypes<DeviceAdapter>::Portal FieldPortal;

      VTKM_CONT_EXPORT
      Kernel(EdgeInterpolationPortalConst interpolation,
             vtkm::Id newPointsOffset,
             FieldPortal field)
        : Interpolation(interpolation), NewPointsOffset(newPointsOffset), Field(field)
      {
      }

      VTKM_EXEC_EXPORT
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

    template <typename T, typename Storage>
    VTKM_CONT_EXPORT
    void operator()(const vtkm::cont::ArrayHandle<T, Storage> &field) const
    {
      vtkm::Id count = this->InterpolationArray.GetNumberOfValues();

      vtkm::cont::ArrayHandle<T> result;
      internal::ResizeArrayHandle(field, field.GetNumberOfValues() + count,
                                  result, DeviceAdapter());

      Kernel<T> kernel(
          this->InterpolationArray.PrepareForInput(DeviceAdapter()),
          this->NewPointsOffset,
          result.PrepareForInPlace(DeviceAdapter()));

      Algorithm::Schedule(kernel, count);
      *(this->Output) = vtkm::cont::DynamicArrayHandle(result);
    }

    VTKM_CONT_EXPORT
    InterpolateField(
        vtkm::cont::ArrayHandle<EdgeInterpolation> interpolationArray,
        vtkm::Id newPointsOffset,
        vtkm::cont::DynamicArrayHandle *output)
      : InterpolationArray(interpolationArray),
        NewPointsOffset(newPointsOffset),
        Output(output)
    {
    }

  private:
    vtkm::cont::ArrayHandle<EdgeInterpolation> InterpolationArray;
    vtkm::Id NewPointsOffset;
    vtkm::cont::DynamicArrayHandle *Output;
  };

  template <typename FieldType>
  vtkm::cont::DynamicArrayHandle ProcessField(
      const FieldType &fieldData) const
  {
    vtkm::cont::DynamicArrayHandle output;
    CastAndCall(fieldData, InterpolateField(this->NewPointsInterpolation,
                                            this->NewPointsOffset,
                                            &output));
    return output;
  }

private:
  internal::ClipTables ClipTablesInstance;
  vtkm::cont::ArrayHandle<EdgeInterpolation> NewPointsInterpolation;
  vtkm::Id NewPointsOffset;
};

}
} // namespace vtkm::worklet

#if defined(THRUST_SCAN_WORKAROUND)
namespace thrust {
namespace detail {

// causes a different code path which does not have the bug
template<> struct is_integral<vtkm::worklet::ClipStats> : public true_type {};

}
} // namespace thrust::detail
#endif

#endif // vtkm_m_worklet_Clip_h
