//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_VertexClustering_h
#define vtk_m_worklet_VertexClustering_h

#include <vtkm/BinaryOperators.h>
#include <vtkm/BinaryPredicates.h>
#include <vtkm/Types.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleDiscard.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/StableSortIndices.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

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

/// Selects the representative point somewhat randomly from the pool of points
/// in a cluster.
struct SelectRepresentativePoint : public vtkm::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn clusterIds, ValuesIn points, ReducedValuesOut repPoints);
  using ExecutionSignature = _3(_2);
  using InputDomain = _1;

  template <typename PointsInVecType>
  VTKM_EXEC typename PointsInVecType::ComponentType operator()(
    const PointsInVecType& pointsIn) const
  {
    // Grab the point from the middle of the set. This usually does a decent
    // job of selecting a representative point that won't emphasize the cluster
    // partitions.
    //
    // Note that we must use the stable sorting with the worklet::Keys for this
    // to be reproducible across backends.
    return pointsIn[pointsIn.GetNumberOfComponents() / 2];
  }

  struct RunTrampoline
  {
    template <typename InputPointsArrayType, typename KeyType>
    VTKM_CONT void operator()(const InputPointsArrayType& points,
                              const vtkm::worklet::Keys<KeyType>& keys,
                              vtkm::cont::VariantArrayHandle& output) const
    {

      vtkm::cont::ArrayHandle<typename InputPointsArrayType::ValueType> out;
      vtkm::worklet::DispatcherReduceByKey<SelectRepresentativePoint> dispatcher;
      dispatcher.Invoke(keys, points, out);

      output = out;
    }
  };

  template <typename KeyType, typename InputDynamicPointsArrayType>
  VTKM_CONT static vtkm::cont::VariantArrayHandle Run(
    const vtkm::worklet::Keys<KeyType>& keys,
    const InputDynamicPointsArrayType& inputPoints)
  {
    vtkm::cont::VariantArrayHandle output;
    RunTrampoline trampoline;
    vtkm::cont::CastAndCall(inputPoints, trampoline, keys, output);
    return output;
  }
};

template <typename ValueType, typename StorageType, typename IndexArrayType>
VTKM_CONT vtkm::cont::ArrayHandle<ValueType> ConcretePermutationArray(
  const IndexArrayType& indices,
  const vtkm::cont::ArrayHandle<ValueType, StorageType>& values)
{
  vtkm::cont::ArrayHandle<ValueType> result;
  auto tmp = vtkm::cont::make_ArrayHandlePermutation(indices, values);
  vtkm::cont::ArrayCopy(tmp, result);
  return result;
}

template <typename T, vtkm::IdComponent N>
vtkm::cont::ArrayHandle<T> copyFromVec(vtkm::cont::ArrayHandle<vtkm::Vec<T, N>> const& other)
{
  const T* vmem = reinterpret_cast<const T*>(&*other.GetPortalConstControl().GetIteratorBegin());
  vtkm::cont::ArrayHandle<T> mem =
    vtkm::cont::make_ArrayHandle(vmem, other.GetNumberOfValues() * N);
  vtkm::cont::ArrayHandle<T> result;
  vtkm::cont::ArrayCopy(mem, result);
  return result;
}

} // namespace internal

struct VertexClustering
{
  using PointIdMapType = vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
                                                            vtkm::cont::ArrayHandle<vtkm::Id>>;

  struct GridInfo
  {
    vtkm::Id3 dim;
    vtkm::Vec3f_64 origin;
    vtkm::Vec3f_64 bin_size;
    vtkm::Vec3f_64 inv_bin_size;
  };

  // input: points  output: cid of the points
  class MapPointsWorklet : public vtkm::worklet::WorkletMapField
  {
  private:
    GridInfo Grid;

  public:
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);

    VTKM_CONT
    MapPointsWorklet(const GridInfo& grid)
      : Grid(grid)
    {
    }

    /// determine grid resolution for clustering
    template <typename PointType>
    VTKM_EXEC vtkm::Id GetClusterId(const PointType& p) const
    {
      using ComponentType = typename PointType::ComponentType;
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

  class MapCellsWorklet : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint pointClusterIds,
                                  FieldOutCell cellClusterIds);
    using ExecutionSignature = void(_2, _3);

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
    using ControlSignature = void(FieldIn, WholeArrayOut);
    using ExecutionSignature = void(WorkIndex, _1, _2); // WorkIndex: use vtkm indexing

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
    using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn);
    using ExecutionSignature = void(_1, _2, _3);

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

  using TypeInt64 = vtkm::List<vtkm::Int64>;

  class Cid3HashWorklet : public vtkm::worklet::WorkletMapField
  {
  private:
    vtkm::Int64 NPoints;

  public:
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);

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
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);

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
  template <typename DynamicCellSetType, typename DynamicCoordinateHandleType>
  vtkm::cont::DataSet Run(const DynamicCellSetType& cellSet,
                          const DynamicCoordinateHandleType& coordinates,
                          const vtkm::Bounds& bounds,
                          const vtkm::Id3& nDivisions)
  {
    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "VertexClustering Worklet");

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
    vtkm::cont::Timer totalTimer;
    totalTimer.Start();
    vtkm::cont::Timer timer;
    timer.Start();
#endif

    //////////////////////////////////////////////
    /// start algorithm

    /// pass 1 : assign points with (cluster) ids based on the grid it falls in
    ///
    /// map points
    vtkm::cont::ArrayHandle<vtkm::Id> pointCidArray;

    vtkm::worklet::DispatcherMapField<MapPointsWorklet> mapPointsDispatcher(
      (MapPointsWorklet(gridInfo)));
    mapPointsDispatcher.Invoke(coordinates, pointCidArray);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    timer.stop();
    std::cout << "Time map points (s): " << timer.GetElapsedTime() << std::endl;
    timer.Start();
#endif

    /// pass 2 : Choose a representative point from each cluster for the output:
    vtkm::cont::VariantArrayHandle repPointArray;
    {
      vtkm::worklet::Keys<vtkm::Id> keys;
      keys.BuildArrays(pointCidArray, vtkm::worklet::KeysSortType::Stable);

      // For mapping properties, this map will select an arbitrary point from
      // the cluster:
      this->PointIdMap =
        vtkm::cont::make_ArrayHandlePermutation(keys.GetOffsets(), keys.GetSortedValuesMap());

      // Compute representative points from each cluster (may not match the
      // PointIdMap indexing)
      repPointArray = internal::SelectRepresentativePoint::Run(keys, coordinates);
    }

    auto repPointCidArray =
      vtkm::cont::make_ArrayHandlePermutation(this->PointIdMap, pointCidArray);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after reducing points (s): " << timer.GetElapsedTime() << std::endl;
    timer.Start();
#endif

    /// Pass 3 : Decimated mesh generation
    ///          For each original triangle, only output vertices from
    ///          three different clusters

    /// map each triangle vertex to the cluster id's
    /// of the cell vertices
    vtkm::cont::ArrayHandle<vtkm::Id3> cid3Array;

    vtkm::worklet::DispatcherMapTopology<MapCellsWorklet> mapCellsDispatcher;
    mapCellsDispatcher.Invoke(cellSet, pointCidArray, cid3Array);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after clustering cells (s): " << timer.GetElapsedTime() << std::endl;
    timer.Start();
#endif

    /// preparation: Get the indexes of the clustered points to prepare for new cell array
    vtkm::cont::ArrayHandle<vtkm::Id> cidIndexArray;
    cidIndexArray.Allocate(gridInfo.dim[0] * gridInfo.dim[1] * gridInfo.dim[2]);

    vtkm::worklet::DispatcherMapField<IndexingWorklet> indexingDispatcher;
    indexingDispatcher.Invoke(repPointCidArray, cidIndexArray);

    pointCidArray.ReleaseResources();
    repPointCidArray.ReleaseResources();

    ///
    /// map: convert each triangle vertices from original point id to the new cluster indexes
    ///      If the triangle is degenerated, set the ids to <nPoints, nPoints, nPoints>
    ///      This ensures it will be placed at the end of the array when sorted.
    ///
    vtkm::Id nPoints = repPointArray.GetNumberOfValues();

    vtkm::cont::ArrayHandle<vtkm::Id3> pointId3Array;

    vtkm::worklet::DispatcherMapField<Cid2PointIdWorklet> cid2PointIdDispatcher(
      (Cid2PointIdWorklet(nPoints)));
    cid2PointIdDispatcher.Invoke(cid3Array, pointId3Array, cidIndexArray);

    cid3Array.ReleaseResources();
    cidIndexArray.ReleaseResources();

    bool doHashing = (nPoints < (1 << 21)); // Check whether we can hash Id3 into 64-bit integers

    if (doHashing)
    {
      /// Create hashed array
      vtkm::cont::ArrayHandle<vtkm::Int64> pointId3HashArray;

      vtkm::worklet::DispatcherMapField<Cid3HashWorklet> cid3HashDispatcher(
        (Cid3HashWorklet(nPoints)));
      cid3HashDispatcher.Invoke(pointId3Array, pointId3HashArray);

      pointId3Array.ReleaseResources();

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time before sort and unique with hashing (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      this->CellIdMap = vtkm::worklet::StableSortIndices::Sort(pointId3HashArray);
      vtkm::worklet::StableSortIndices::Unique(pointId3HashArray, this->CellIdMap);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique with hashing (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      // Create a temporary permutation array and use that for unhashing.
      auto tmpPerm = vtkm::cont::make_ArrayHandlePermutation(this->CellIdMap, pointId3HashArray);

      // decode
      vtkm::worklet::DispatcherMapField<Cid3UnhashWorklet> cid3UnhashDispatcher(
        (Cid3UnhashWorklet(nPoints)));
      cid3UnhashDispatcher.Invoke(tmpPerm, pointId3Array);
    }
    else
    {
#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time before sort and unique [no hashing] (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      this->CellIdMap = vtkm::worklet::StableSortIndices::Sort(pointId3Array);
      vtkm::worklet::StableSortIndices::Unique(pointId3Array, this->CellIdMap);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique [no hashing] (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      // Permute the connectivity array into a basic array handle. Use a
      // temporary array handle to avoid memory aliasing.
      {
        vtkm::cont::ArrayHandle<vtkm::Id3> tmp;
        tmp = internal::ConcretePermutationArray(this->CellIdMap, pointId3Array);
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

    vtkm::cont::CellSetSingleType<> triangles;
    triangles.Fill(repPointArray.GetNumberOfValues(),
                   vtkm::CellShapeTagTriangle::Id,
                   3,
                   internal::copyFromVec(pointId3Array));
    output.SetCellSet(triangles);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Wrap-up (s): " << timer.GetElapsedTime() << std::endl;
    vtkm::Float64 t = totalTimer.GetElapsedTime();
    std::cout << "Time (s): " << t << std::endl;
    std::cout << "number of output points: " << repPointArray.GetNumberOfValues() << std::endl;
    std::cout << "number of output cells: " << pointId3Array.GetNumberOfValues() << std::endl;
#endif

    return output;
  }

  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& input) const
  {
    return internal::ConcretePermutationArray(this->PointIdMap, input);
  }

  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& input) const
  {
    return internal::ConcretePermutationArray(this->CellIdMap, input);
  }

private:
  PointIdMapType PointIdMap;
  vtkm::cont::ArrayHandle<vtkm::Id> CellIdMap;
}; // struct VertexClustering
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_VertexClustering_h
