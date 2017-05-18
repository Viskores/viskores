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
#ifndef vtk_m_worklet_VertexClustering_h
#define vtk_m_worklet_VertexClustering_h

#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/worklet/AverageByKey.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

//#define __VTKM_VERTEX_CLUSTERING_BENCHMARK
//#include <vtkm/cont/Timer.h>

namespace vtkm
{
namespace worklet
{

namespace internal
{

template <typename T, vtkm::IdComponent N, typename DeviceAdapter>
vtkm::cont::ArrayHandle<T> copyFromVec(vtkm::cont::ArrayHandle<vtkm::Vec<T, N>> const& other,
                                       DeviceAdapter)
{
  const T* vmem = reinterpret_cast<const T*>(&*other.GetPortalConstControl().GetIteratorBegin());
  vtkm::cont::ArrayHandle<T> mem =
    vtkm::cont::make_ArrayHandle(vmem, other.GetNumberOfValues() * N);
  vtkm::cont::ArrayHandle<T> result;
  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(mem, result);
  return result;
}

template <typename KeyArrayIn, typename KeyArrayOut, typename DeviceAdapter>
class AverageByKeyDynamicValue
{
private:
  typedef typename KeyArrayIn::ValueType KeyType;

public:
  VTKM_CONT
  AverageByKeyDynamicValue(const KeyArrayIn& inputKeys, KeyArrayOut& outputKeys,
                           vtkm::cont::DynamicArrayHandle& outputValues)
    : InputKeys(inputKeys)
    , OutputKeys(&outputKeys)
    , OutputValues(&outputValues)
  {
  }

  template <typename ValueArrayIn>
  VTKM_CONT void operator()(const ValueArrayIn& coordinates) const
  {
    typedef typename ValueArrayIn::ValueType ValueType;

    vtkm::cont::ArrayHandle<ValueType> outArray;
    vtkm::worklet::AverageByKey(InputKeys, coordinates, *(this->OutputKeys), outArray,
                                DeviceAdapter());
    *(this->OutputValues) = vtkm::cont::DynamicArrayHandle(outArray);
  }

private:
  KeyArrayIn InputKeys;
  KeyArrayOut* OutputKeys;
  vtkm::cont::DynamicArrayHandle* OutputValues;
};

} // namespace internal

struct VertexClustering
{
  struct GridInfo
  {
    vtkm::Id dim[3];
    vtkm::Vec<vtkm::Float64, 3> origin;
    vtkm::Float64 grid_width;
    vtkm::Float64 inv_grid_width; // = 1/grid_width
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

      PointType p_rel = (p - gridOrigin) * static_cast<ComponentType>(this->Grid.inv_grid_width);
      vtkm::Id x = vtkm::Min((vtkm::Id)p_rel[0], this->Grid.dim[0] - 1);
      vtkm::Id y = vtkm::Min((vtkm::Id)p_rel[1], this->Grid.dim[1] - 1);
      vtkm::Id z = vtkm::Min((vtkm::Id)p_rel[2], this->Grid.dim[2] - 1);
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
    typedef void ControlSignature(CellSetIn cellset, FieldInPoint<IdType> pointClusterIds,
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
    VTKM_EXEC void operator()(const vtkm::Id& counter, const vtkm::Id& cid,
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
    VTKM_EXEC void operator()(const vtkm::Id3& cid3, vtkm::Id3& pointId3,
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

  class Id3Less
  {
  public:
    VTKM_EXEC
    bool operator()(const vtkm::Id3& a, const vtkm::Id3& b) const
    {
      if (a[0] < 0)
      {
        // invalid id: place at the last after sorting
        // (comparing to 0 is faster than matching -1)
        return false;
      }
      return b[0] < 0 || a[0] < b[0] || (a[0] == b[0] && a[1] < b[1]) ||
        (a[0] == b[0] && a[1] == b[1] && a[2] < b[2]);
    }
  };

public:
  ///////////////////////////////////////////////////
  /// \brief VertexClustering: Mesh simplification
  ///
  template <typename DynamicCellSetType, typename DynamicCoordinateHandleType,
            typename DeviceAdapter>
  vtkm::cont::DataSet Run(const DynamicCellSetType& cellSet,
                          const DynamicCoordinateHandleType& coordinates,
                          const vtkm::Bounds& bounds, const vtkm::Id3& nDivisions, DeviceAdapter)
  {

    /// determine grid resolution for clustering
    GridInfo gridInfo;
    {
      vtkm::Vec<vtkm::Float64, 3> res(bounds.X.Length() / static_cast<vtkm::Float64>(nDivisions[0]),
                                      bounds.Y.Length() / static_cast<vtkm::Float64>(nDivisions[1]),
                                      bounds.Z.Length() /
                                        static_cast<vtkm::Float64>(nDivisions[2]));
      gridInfo.grid_width = vtkm::Max(res[0], vtkm::Max(res[1], res[2]));

      vtkm::Float64 inv_grid_width = gridInfo.inv_grid_width =
        vtkm::Float64(1) / gridInfo.grid_width;

      //printf("Bounds: %lf, %lf, %lf, %lf, %lf, %lf\n", bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
      gridInfo.dim[0] =
        vtkm::Min((vtkm::Id)vtkm::Ceil((bounds.X.Length()) * inv_grid_width), nDivisions[0]);
      gridInfo.dim[1] =
        vtkm::Min((vtkm::Id)vtkm::Ceil((bounds.Y.Length()) * inv_grid_width), nDivisions[1]);
      gridInfo.dim[2] =
        vtkm::Min((vtkm::Id)vtkm::Ceil((bounds.Z.Length()) * inv_grid_width), nDivisions[2]);

      // center the mesh in the grids
      vtkm::Vec<vtkm::Float64, 3> center = bounds.Center();
      gridInfo.origin[0] =
        center[0] - gridInfo.grid_width * static_cast<vtkm::Float64>(gridInfo.dim[0]) * .5;
      gridInfo.origin[1] =
        center[1] - gridInfo.grid_width * static_cast<vtkm::Float64>(gridInfo.dim[1]) * .5;
      gridInfo.origin[2] =
        center[2] - gridInfo.grid_width * static_cast<vtkm::Float64>(gridInfo.dim[2]) * .5;
    }

//construct the scheduler that will execute all the worklets
#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
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
#endif

    /// pass 2 : compute average point position for each cluster,
    ///          using pointCidArray as the key
    ///
    vtkm::cont::ArrayHandle<vtkm::Id> pointCidArrayReduced;
    vtkm::cont::DynamicArrayHandle repPointArray; // representative point

    internal::AverageByKeyDynamicValue<vtkm::cont::ArrayHandle<vtkm::Id>,
                                       vtkm::cont::ArrayHandle<vtkm::Id>, DeviceAdapter>
      averageByKey(pointCidArray, pointCidArrayReduced, repPointArray);
    CastAndCall(coordinates, averageByKey);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after averaging (s): " << timer.GetElapsedTime() << std::endl;
#endif

    /// Pass 3 : Decimated mesh generation
    ///          For each original triangle, only output vertices from
    ///          three different clusters

    /// map each triangle vertex to the cluster id's
    /// of the cell vertices
    vtkm::cont::ArrayHandle<vtkm::Id3> cid3Array;

    vtkm::worklet::DispatcherMapTopology<MapCellsWorklet, DeviceAdapter>(MapCellsWorklet())
      .Invoke(cellSet, pointCidArray, cid3Array);

    pointCidArray.ReleaseResources();

    /// preparation: Get the indexes of the clustered points to prepare for new cell array
    vtkm::cont::ArrayHandle<vtkm::Id> cidIndexArray;
    cidIndexArray.PrepareForOutput(gridInfo.dim[0] * gridInfo.dim[1] * gridInfo.dim[2],
                                   DeviceAdapter());

    vtkm::worklet::DispatcherMapField<IndexingWorklet, DeviceAdapter>().Invoke(pointCidArrayReduced,
                                                                               cidIndexArray);

    pointCidArrayReduced.ReleaseResources();

    ///
    /// map: convert each triangle vertices from original point id to the new cluster indexes
    ///      If the triangle is degenerated, set the ids to <-1, -1, -1>
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
#endif

      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Sort(pointId3HashArray);
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Unique(pointId3HashArray);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique with hashing (s): " << timer.GetElapsedTime()
                << std::endl;
#endif

      // decode
      vtkm::worklet::DispatcherMapField<Cid3UnhashWorklet, DeviceAdapter>(
        Cid3UnhashWorklet(nPoints))
        .Invoke(pointId3HashArray, pointId3Array);
    }
    else
    {

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time before sort and unique [no hashing] (s): " << timer.GetElapsedTime()
                << std::endl;
#endif

      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Sort(pointId3Array);
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Unique(pointId3Array);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique [no hashing] (s): " << timer.GetElapsedTime()
                << std::endl;
#endif
    }

    // remove the last element if invalid
    vtkm::Id cells = pointId3Array.GetNumberOfValues();
    if (cells > 0 && pointId3Array.GetPortalConstControl().Get(cells - 1)[2] >= nPoints)
    {
      cells--;
      pointId3Array.Shrink(cells);
    }

    /// output
    vtkm::cont::DataSet output;

    output.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", repPointArray));

    vtkm::cont::CellSetSingleType<> triangles("cells");
    triangles.Fill(repPointArray.GetNumberOfValues(), vtkm::CellShapeTagTriangle::Id, 3,
                   internal::copyFromVec(pointId3Array, DeviceAdapter()));
    output.AddCellSet(triangles);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    vtkm::Float64 t = timer.GetElapsedTime();
    std::cout << "Time (s): " << t << std::endl;
    std::cout << "number of output points: " << repPointArray.GetNumberOfValues() << std::endl;
    std::cout << "number of output cells: " << pointId3Array.GetNumberOfValues() << std::endl;
#endif

    return output;
  }
}; // struct VertexClustering
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_VertexClustering_h
