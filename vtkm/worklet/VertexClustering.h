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
#ifndef vtk_m_worklet_VertexClustering_h
#define vtk_m_worklet_VertexClustering_h

#include <vtkm/BinaryOperators.h>
#include <vtkm/BinaryPredicates.h>
#include <vtkm/Types.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleDiscard.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

//#define __VTKM_VERTEX_CLUSTERING_BENCHMARK

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
#include <vtkm/cont/Timer.h>
#endif

namespace vtkm
{
namespace worklet
{

namespace internal
{

// Allows Sort to be called on an array that indexes into ValuePortal.
template <typename ValuePortalType>
struct IndirectSortPredicate
{
  using ValueType = typename ValuePortalType::ValueType;

  ValuePortalType ValuePortal;

  IndirectSortPredicate(ValuePortalType valuePortal)
    : ValuePortal(valuePortal)
  {
  }

  template <typename IndexType>
  bool operator()(const IndexType& a, const IndexType& b) const
  {
    // If the values compare equal, compare the indices as well so we get
    // consistent outputs.
    const ValueType valueA = this->ValuePortal.Get(a);
    const ValueType valueB = this->ValuePortal.Get(b);
    if (valueA < valueB)
    {
      return true;
    }
    else if (valueB < valueA)
    {
      return false;
    }
    else
    {
      return a < b;
    }
  }
};

// Allows Unique to be called on an array that indexes into ValuePortal.
template <typename ValuePortalType>
struct IndirectUniquePredicate
{
  ValuePortalType ValuePortal;

  IndirectUniquePredicate(ValuePortalType valuePortal)
    : ValuePortal(valuePortal)
  {
  }

  template <typename IndexType>
  bool operator()(const IndexType& a, const IndexType& b) const
  {
    return this->ValuePortal.Get(a) == this->ValuePortal.Get(b);
  }
};

// Returns an ArrayHandle<vtkm::Id> that contains sorted, unique indices of the data in values.
template <typename ValueType, typename Storage, typename DeviceAdapter>
VTKM_CONT static vtkm::cont::ArrayHandle<vtkm::Id> SortAndUniqueIndices(
  const vtkm::cont::ArrayHandle<ValueType, Storage>& values,
  DeviceAdapter)
{
  using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
  using IndexArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using ValueArrayType = vtkm::cont::ArrayHandle<ValueType, Storage>;
  using ValuePortalType =
    typename ValueArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;

  using SortPredicate = IndirectSortPredicate<ValuePortalType>;
  using UniquePredicate = IndirectUniquePredicate<ValuePortalType>;

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
  vtkm::cont::Timer<DeviceAdapter> timer;
#endif

  // Generate the initial index array
  IndexArrayType indices;
  {
    vtkm::cont::ArrayHandleIndex indicesSrc(values.GetNumberOfValues());
    Algo::Copy(indicesSrc, indices);
  }

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
  std::cout << "Generate index array: " << timer.GetElapsedTime() << "\n";
  timer.Reset();
#endif

  ValuePortalType valuePortal = values.PrepareForInput(DeviceAdapter());

  // Sort the values:
  {
    Algo::Sort(indices, SortPredicate(valuePortal));
  }

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
  std::cout << "Indexed sort: " << timer.GetElapsedTime() << "\n";
  timer.Reset();
#endif

  // Make unique:
  {
    Algo::Unique(indices, UniquePredicate(valuePortal));
  }

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
  std::cout << "Indexed unique: " << timer.GetElapsedTime() << "\n";
  timer.Reset();
#endif

  return indices;
}

template <typename ValueType, typename StorageType, typename DeviceAdapter>
VTKM_CONT vtkm::cont::ArrayHandle<ValueType> ConcretePermutationArray(
  const vtkm::cont::ArrayHandle<vtkm::Id>& indices,
  const vtkm::cont::ArrayHandle<ValueType, StorageType>& values,
  DeviceAdapter)
{
  vtkm::cont::ArrayHandle<ValueType> result;
  auto tmp = vtkm::cont::make_ArrayHandlePermutation(indices, values);
  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(tmp, result);
  return result;
}

// Create a permutation of a dynamic array.
template <typename DeviceAdapter>
struct DynamicPermutationFunctor
{
  using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  const vtkm::cont::ArrayHandle<vtkm::Id>& Indices;
  vtkm::cont::DynamicArrayHandle& Output;

  DynamicPermutationFunctor(const vtkm::cont::ArrayHandle<vtkm::Id>& indices,
                            vtkm::cont::DynamicArrayHandle& output)
    : Indices(indices)
    , Output(output)
  {
  }

  template <typename ArrayType>
  VTKM_CONT void operator()(const ArrayType& input) const
  {
    this->Output = ConcretePermutationArray(this->Indices, input, DeviceAdapter());
  }

  template <typename DynamicArrayHandleType>
  VTKM_CONT static vtkm::cont::DynamicArrayHandle Run(
    const vtkm::cont::ArrayHandle<vtkm::Id>& indices,
    const DynamicArrayHandleType& values)
  {
    vtkm::cont::DynamicArrayHandle result;
    DynamicPermutationFunctor<DeviceAdapter> functor(indices, result);
    CastAndCall(values, functor);
    return result;
  }
};

template <typename T, vtkm::IdComponent N, typename DeviceAdapter>
vtkm::cont::ArrayHandle<T> copyFromVec(vtkm::cont::ArrayHandle<vtkm::Vec<T, N>> const& other,
                                       DeviceAdapter)
{
  const T* vmem = reinterpret_cast<const T*>(&*other.GetPortalConstControl().GetIteratorBegin());
  vtkm::cont::ArrayHandle<T> mem =
    vtkm::cont::make_ArrayHandle(vmem, other.GetNumberOfValues() * N);
  vtkm::cont::ArrayHandle<T> result;
  vtkm::cont::ArrayCopy(mem, result, DeviceAdapter());
  return result;
}

} // namespace internal

struct VertexClustering
{
  struct GridInfo
  {
    vtkm::Vec<vtkm::Id, 3> dim;
    vtkm::Vec<vtkm::Float64, 3> origin;
    vtkm::Vec<vtkm::Float64, 3> bin_size;
    vtkm::Vec<vtkm::Float64, 3> inv_bin_size;
  };

  // input: points  output: cid of the points
  class MapPointsWorklet : public vtkm::worklet::WorkletMapField
  {
  private:
    GridInfo Grid;

  public:
    typedef void ControlSignature(FieldIn<Vec3>, FieldOut<IdType>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT
    MapPointsWorklet(const GridInfo& grid)
      : Grid(grid)
    {
    }

    /// determine grid resolution for clustering
    template <typename PointType>
    VTKM_EXEC vtkm::Id GetClusterId(const PointType& p) const
    {
      typedef typename PointType::ComponentType ComponentType;
      PointType gridOrigin(static_cast<ComponentType>(this->Grid.origin[0]),
                           static_cast<ComponentType>(this->Grid.origin[1]),
                           static_cast<ComponentType>(this->Grid.origin[2]));

      PointType p_rel = (p - gridOrigin) * this->Grid.inv_bin_size;

      vtkm::Id x = vtkm::Min(static_cast<vtkm::Id>(p_rel[0]), this->Grid.dim[0] - 1);
      vtkm::Id y = vtkm::Min(static_cast<vtkm::Id>(p_rel[1]), this->Grid.dim[1] - 1);
      vtkm::Id z = vtkm::Min(static_cast<vtkm::Id>(p_rel[2]), this->Grid.dim[2] - 1);

      return x + this->Grid.dim[0] * (y + this->Grid.dim[1] * z); // get a unique hash value
    }

    template <typename PointType>
    VTKM_EXEC void operator()(const PointType& point, vtkm::Id& cid) const
    {
      cid = this->GetClusterId(point);
      VTKM_ASSERT(cid >= 0); // the id could overflow if too many cells
    }
  };

  class MapCellsWorklet : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldInPoint<IdType> pointClusterIds,
                                  FieldOutCell<Id3Type> cellClusterIds);
    typedef void ExecutionSignature(_2, _3);

    VTKM_CONT
    MapCellsWorklet() {}

    // TODO: Currently only works with Triangle cell types
    template <typename ClusterIdsVecType>
    VTKM_EXEC void operator()(const ClusterIdsVecType& pointClusterIds,
                              vtkm::Id3& cellClusterId) const
    {
      cellClusterId[0] = pointClusterIds[0];
      cellClusterId[1] = pointClusterIds[1];
      cellClusterId[2] = pointClusterIds[2];
    }
  };

  /// pass 3
  class IndexingWorklet : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType>, WholeArrayOut<IdType>);
    typedef void ExecutionSignature(WorkIndex, _1, _2); // WorkIndex: use vtkm indexing

    template <typename OutPortalType>
    VTKM_EXEC void operator()(const vtkm::Id& counter,
                              const vtkm::Id& cid,
                              const OutPortalType& outPortal) const
    {
      outPortal.Set(cid, counter);
    }
  };

  class Cid2PointIdWorklet : public vtkm::worklet::WorkletMapField
  {
    vtkm::Id NPoints;

    VTKM_EXEC
    void rotate(vtkm::Id3& ids) const
    {
      vtkm::Id temp = ids[0];
      ids[0] = ids[1];
      ids[1] = ids[2];
      ids[2] = temp;
    }

  public:
    typedef void ControlSignature(FieldIn<Id3Type>, FieldOut<Id3Type>, WholeArrayIn<IdType>);
    typedef void ExecutionSignature(_1, _2, _3);

    VTKM_CONT
    Cid2PointIdWorklet(vtkm::Id nPoints)
      : NPoints(nPoints)
    {
    }

    template <typename InPortalType>
    VTKM_EXEC void operator()(const vtkm::Id3& cid3,
                              vtkm::Id3& pointId3,
                              const InPortalType& inPortal) const
    {
      if (cid3[0] == cid3[1] || cid3[0] == cid3[2] || cid3[1] == cid3[2])
      {
        pointId3[0] = pointId3[1] = pointId3[2] = this->NPoints; // invalid cell to be removed
      }
      else
      {
        pointId3[0] = inPortal.Get(cid3[0]);
        pointId3[1] = inPortal.Get(cid3[1]);
        pointId3[2] = inPortal.Get(cid3[2]);

        // Sort triangle point ids so that the same triangle will have the same signature
        // Rotate these ids making the first one the smallest
        if (pointId3[0] > pointId3[1] || pointId3[0] > pointId3[2])
        {
          rotate(pointId3);
          if (pointId3[0] > pointId3[1] || pointId3[0] > pointId3[2])
          {
            rotate(pointId3);
          }
        }
      }
    }
  };

  struct TypeInt64 : vtkm::ListTagBase<vtkm::Int64>
  {
  };

  class Cid3HashWorklet : public vtkm::worklet::WorkletMapField
  {
  private:
    vtkm::Int64 NPoints;

  public:
    typedef void ControlSignature(FieldIn<Id3Type>, FieldOut<TypeInt64>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT
    Cid3HashWorklet(vtkm::Id nPoints)
      : NPoints(nPoints)
    {
    }

    VTKM_EXEC
    void operator()(const vtkm::Id3& cid, vtkm::Int64& cidHash) const
    {
      cidHash =
        cid[0] + this->NPoints * (cid[1] + this->NPoints * cid[2]); // get a unique hash value
    }
  };

  class Cid3UnhashWorklet : public vtkm::worklet::WorkletMapField
  {
  private:
    vtkm::Int64 NPoints;

  public:
    typedef void ControlSignature(FieldIn<TypeInt64>, FieldOut<Id3Type>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT
    Cid3UnhashWorklet(vtkm::Id nPoints)
      : NPoints(nPoints)
    {
    }

    VTKM_EXEC
    void operator()(const vtkm::Int64& cidHash, vtkm::Id3& cid) const
    {
      cid[0] = static_cast<vtkm::Id>(cidHash % this->NPoints);
      vtkm::Int64 t = cidHash / this->NPoints;
      cid[1] = static_cast<vtkm::Id>(t % this->NPoints);
      cid[2] = static_cast<vtkm::Id>(t / this->NPoints);
    }
  };

public:
  ///////////////////////////////////////////////////
  /// \brief VertexClustering: Mesh simplification
  ///
  template <typename DynamicCellSetType,
            typename DynamicCoordinateHandleType,
            typename DeviceAdapter>
  vtkm::cont::DataSet Run(const DynamicCellSetType& cellSet,
                          const DynamicCoordinateHandleType& coordinates,
                          const vtkm::Bounds& bounds,
                          const vtkm::Id3& nDivisions,
                          DeviceAdapter)
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    /// determine grid resolution for clustering
    GridInfo gridInfo;
    {
      gridInfo.origin[0] = bounds.X.Min;
      gridInfo.origin[1] = bounds.Y.Min;
      gridInfo.origin[2] = bounds.Z.Min;
      gridInfo.dim[0] = nDivisions[0];
      gridInfo.dim[1] = nDivisions[1];
      gridInfo.dim[2] = nDivisions[2];
      gridInfo.bin_size[0] = bounds.X.Length() / static_cast<vtkm::Float64>(nDivisions[0]);
      gridInfo.bin_size[1] = bounds.Y.Length() / static_cast<vtkm::Float64>(nDivisions[1]);
      gridInfo.bin_size[2] = bounds.Z.Length() / static_cast<vtkm::Float64>(nDivisions[2]);
      gridInfo.inv_bin_size[0] = 1. / gridInfo.bin_size[0];
      gridInfo.inv_bin_size[1] = 1. / gridInfo.bin_size[1];
      gridInfo.inv_bin_size[2] = 1. / gridInfo.bin_size[2];
    }

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    vtkm::cont::Timer<> totalTimer;
    vtkm::cont::Timer<> timer;
#endif

    //////////////////////////////////////////////
    /// start algorithm

    /// pass 1 : assign points with (cluster) ids based on the grid it falls in
    ///
    /// map points
    vtkm::cont::ArrayHandle<vtkm::Id> pointCidArray;

    vtkm::worklet::DispatcherMapField<MapPointsWorklet, DeviceAdapter>(MapPointsWorklet(gridInfo))
      .Invoke(coordinates, pointCidArray);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time map points (s): " << timer.GetElapsedTime() << std::endl;
    timer.Reset();
#endif

    /// pass 2 : Reduce each cluster to a single point from the cluster.
    ///          Choosing a random point from the input gives a better visual
    ///          result than a simple average in the common case of surface
    ///          geometry, as the average results in a blocky output with
    ///          obvious grid artifacts.
    this->PointIdMap = internal::SortAndUniqueIndices(pointCidArray, DeviceAdapter());

    using RepPointCidType = vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
                                                               vtkm::cont::ArrayHandle<vtkm::Id>>;
    RepPointCidType repPointCidArray =
      vtkm::cont::make_ArrayHandlePermutation(this->PointIdMap, pointCidArray);

    // representative points
    vtkm::cont::DynamicArrayHandle repPointArray =
      internal::DynamicPermutationFunctor<DeviceAdapter>::Run(this->PointIdMap, coordinates);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after averaging (s): " << timer.GetElapsedTime() << std::endl;
    timer.Reset();
#endif

    /// Pass 3 : Decimated mesh generation
    ///          For each original triangle, only output vertices from
    ///          three different clusters

    /// map each triangle vertex to the cluster id's
    /// of the cell vertices
    vtkm::cont::ArrayHandle<vtkm::Id3> cid3Array;

    vtkm::worklet::DispatcherMapTopology<MapCellsWorklet, DeviceAdapter>(MapCellsWorklet())
      .Invoke(cellSet, pointCidArray, cid3Array);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after clustering cells (s): " << timer.GetElapsedTime() << std::endl;
    timer.Reset();
#endif


    /// preparation: Get the indexes of the clustered points to prepare for new cell array
    vtkm::cont::ArrayHandle<vtkm::Id> cidIndexArray;
    cidIndexArray.PrepareForOutput(gridInfo.dim[0] * gridInfo.dim[1] * gridInfo.dim[2],
                                   DeviceAdapter());

    vtkm::worklet::DispatcherMapField<IndexingWorklet, DeviceAdapter>().Invoke(repPointCidArray,
                                                                               cidIndexArray);

    pointCidArray.ReleaseResources();
    repPointCidArray.ReleaseResources();

    ///
    /// map: convert each triangle vertices from original point id to the new cluster indexes
    ///      If the triangle is degenerated, set the ids to <nPoints, nPoints, nPoints>
    ///      This ensures it will be placed at the end of the array when sorted.
    ///
    vtkm::Id nPoints = repPointArray.GetNumberOfValues();

    vtkm::cont::ArrayHandle<vtkm::Id3> pointId3Array;

    vtkm::worklet::DispatcherMapField<Cid2PointIdWorklet, DeviceAdapter>(
      Cid2PointIdWorklet(nPoints))
      .Invoke(cid3Array, pointId3Array, cidIndexArray);

    cid3Array.ReleaseResources();
    cidIndexArray.ReleaseResources();

    bool doHashing = (nPoints < (1 << 21)); // Check whether we can hash Id3 into 64-bit integers

    if (doHashing)
    {
      /// Create hashed array
      vtkm::cont::ArrayHandle<vtkm::Int64> pointId3HashArray;

      vtkm::worklet::DispatcherMapField<Cid3HashWorklet, DeviceAdapter>(Cid3HashWorklet(nPoints))
        .Invoke(pointId3Array, pointId3HashArray);

      pointId3Array.ReleaseResources();

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time before sort and unique with hashing (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Reset();
#endif

      this->CellIdMap = internal::SortAndUniqueIndices(pointId3HashArray, DeviceAdapter());

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique with hashing (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Reset();
#endif

      // Create a temporary permutation array and use that for unhashing.
      auto tmpPerm = vtkm::cont::make_ArrayHandlePermutation(this->CellIdMap, pointId3HashArray);

      // decode
      vtkm::worklet::DispatcherMapField<Cid3UnhashWorklet, DeviceAdapter>(
        Cid3UnhashWorklet(nPoints))
        .Invoke(tmpPerm, pointId3Array);
    }
    else
    {
#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time before sort and unique [no hashing] (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Reset();
#endif

      this->CellIdMap = internal::SortAndUniqueIndices(pointId3Array, DeviceAdapter());

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique [no hashing] (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Reset();
#endif

      // Permute the connectivity array into a basic array handle:
      {
        vtkm::cont::ArrayHandle<vtkm::Id3> tmp;
        tmp = internal::ConcretePermutationArray(this->CellIdMap, pointId3Array, DeviceAdapter());
        pointId3Array = tmp;
      }
    }

    // remove the last element if invalid
    vtkm::Id cells = pointId3Array.GetNumberOfValues();
    if (cells > 0 && pointId3Array.GetPortalConstControl().Get(cells - 1)[2] >= nPoints)
    {
      cells--;
      pointId3Array.Shrink(cells);
      this->CellIdMap.Shrink(cells);
    }

    /// output
    vtkm::cont::DataSet output;

    output.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", repPointArray));

    vtkm::cont::CellSetSingleType<> triangles("cells");
    triangles.Fill(repPointArray.GetNumberOfValues(),
                   vtkm::CellShapeTagTriangle::Id,
                   3,
                   internal::copyFromVec(pointId3Array, DeviceAdapter()));
    output.AddCellSet(triangles);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Wrap-up (s): " << timer.GetElapsedTime() << std::endl;
    vtkm::Float64 t = totalTimer.GetElapsedTime();
    std::cout << "Time (s): " << t << std::endl;
    std::cout << "number of output points: " << repPointArray.GetNumberOfValues() << std::endl;
    std::cout << "number of output cells: " << pointId3Array.GetNumberOfValues() << std::endl;
#endif

    return output;
  }

  template <typename ValueType, typename StorageType, typename DeviceAdapter>
  vtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& input,
    const DeviceAdapter&) const
  {
    return internal::ConcretePermutationArray(this->PointIdMap, input, DeviceAdapter());
  }

  template <typename ValueType, typename StorageType, typename DeviceAdapter>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& input,
    const DeviceAdapter&) const
  {
    return internal::ConcretePermutationArray(this->CellIdMap, input, DeviceAdapter());
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> PointIdMap;
  vtkm::cont::ArrayHandle<vtkm::Id> CellIdMap;
}; // struct VertexClustering
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_VertexClustering_h
