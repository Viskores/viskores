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

#include <vtkm/exec/Assert.h>

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

namespace vtkm {
namespace worklet {

namespace internal {

template<typename T, vtkm::IdComponent N, typename DeviceAdapter>
vtkm::cont::ArrayHandle<T> copyFromVec(vtkm::cont::ArrayHandle<vtkm::Vec<T, N> > const& other,
                                       DeviceAdapter)
{
    const T *vmem = reinterpret_cast< const T *>(& *other.GetPortalConstControl().GetIteratorBegin());
    vtkm::cont::ArrayHandle<T> mem = vtkm::cont::make_ArrayHandle(vmem, other.GetNumberOfValues()*N);
    vtkm::cont::ArrayHandle<T> result;
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(mem,result);
    return result;
}

template <typename KeyArrayIn, typename KeyArrayOut, typename DeviceAdapter>
class AverageByKeyDynamicValue
{
private:
  typedef typename KeyArrayIn::ValueType KeyType;

public:
  VTKM_CONT_EXPORT
  AverageByKeyDynamicValue(const KeyArrayIn &inputKeys,
                           KeyArrayOut &outputKeys,
                           vtkm::cont::DynamicArrayHandle &outputValues)
    : InputKeys(inputKeys), OutputKeys(&outputKeys), OutputValues(&outputValues)
  { }

  template <typename ValueArrayIn>
  VTKM_CONT_EXPORT
  void operator()(const ValueArrayIn& coordinates) const
  {
    typedef typename ValueArrayIn::ValueType ValueType;

    vtkm::cont::ArrayHandle<ValueType> outArray;
    vtkm::worklet::AverageByKey(InputKeys,
                                coordinates,
                                *(this->OutputKeys),
                                outArray,
                                DeviceAdapter());
    *(this->OutputValues) = vtkm::cont::DynamicArrayHandle(outArray);
  }

private:
  KeyArrayIn InputKeys;
  KeyArrayOut *OutputKeys;
  vtkm::cont::DynamicArrayHandle *OutputValues;
};

template<typename ShapeStorageTag,
         typename NumIndicesStorageTag,
         typename ConnectivityStorageTag>
vtkm::cont::CellSetExplicit<
    ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag>
  make_CellSetExplicit(
      const vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorageTag> &cellTypes,
      const vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorageTag> &numIndices,
      const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> &connectivity)
{
  vtkm::cont::CellSetExplicit<
    ShapeStorageTag, NumIndicesStorageTag, ConnectivityStorageTag> cellSet;
  cellSet.Fill(cellTypes, numIndices, connectivity);

  return cellSet;
}

} // namespace internal

template<typename DeviceAdapter>
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
    typedef void ControlSignature(FieldIn<Vec3> , FieldOut<IdType>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    MapPointsWorklet(const GridInfo &grid)
      : Grid(grid)
    { }

    /// determine grid resolution for clustering
    template<typename PointType>
    VTKM_EXEC_EXPORT
    vtkm::Id GetClusterId(const PointType &p) const
    {
      typedef typename PointType::ComponentType ComponentType;
      PointType gridOrigin(
        static_cast<ComponentType>(this->Grid.origin[0]),
        static_cast<ComponentType>(this->Grid.origin[1]),
        static_cast<ComponentType>(this->Grid.origin[2]));

      PointType p_rel = (p - gridOrigin) *
                        static_cast<ComponentType>(this->Grid.inv_grid_width);
      vtkm::Id x = vtkm::Min((vtkm::Id)p_rel[0], this->Grid.dim[0]-1);
      vtkm::Id y = vtkm::Min((vtkm::Id)p_rel[1], this->Grid.dim[1]-1);
      vtkm::Id z = vtkm::Min((vtkm::Id)p_rel[2], this->Grid.dim[2]-1);
      return x + this->Grid.dim[0] * (y + this->Grid.dim[1] * z);  // get a unique hash value
    }

    template<typename PointType>
    VTKM_EXEC_EXPORT
    void operator()(const PointType &point, vtkm::Id &cid) const
    {
      cid = this->GetClusterId(point);
      VTKM_ASSERT_EXEC(cid>=0, *this);  // the id could overflow if too many cells
    }
  };


  class MapCellsWorklet: public vtkm::worklet::WorkletMapTopologyPointToCell
  {
  public:
    typedef void ControlSignature(TopologyIn topology,
                                  FieldInFrom<IdType> pointClusterIds,
                                  FieldOut<Id3Type> cellClusterIds);
    typedef void ExecutionSignature(_2, _3);

    VTKM_CONT_EXPORT
    MapCellsWorklet()
    { }

    // TODO: Currently only works with Triangle cell types
    template<typename ClusterIdsVecType>
    VTKM_EXEC_EXPORT
    void operator()(const ClusterIdsVecType &pointClusterIds,
                    vtkm::Id3 &cellClusterId) const
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
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
  private:
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::Portal IdPortalType;
    IdPortalType CidIndexRaw;
    vtkm::Id Len;
  public:
      typedef void ControlSignature(FieldIn<IdType>);
      typedef void ExecutionSignature(WorkIndex, _1);  // WorkIndex: use vtkm indexing

      VTKM_CONT_EXPORT
      IndexingWorklet( IdArrayHandle &cidIndexArray, vtkm::Id n ) : Len(n)
      {
        this->CidIndexRaw = cidIndexArray.PrepareForOutput(n, DeviceAdapter() );
      }

      VTKM_EXEC_EXPORT
      void operator()(const vtkm::Id &counter, const vtkm::Id &cid) const
      {
        VTKM_ASSERT_EXEC( cid < this->Len , *this );
        this->CidIndexRaw.Set(cid, counter);
      }
  };

  class Cid2PointIdWorklet : public vtkm::worklet::WorkletMapField
  {
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
    const IdPortalType CidIndexRaw;
    vtkm::Id NPoints;

    VTKM_EXEC_EXPORT
    void rotate(vtkm::Id3 &ids) const
    {
      vtkm::Id temp=ids[0]; ids[0] = ids[1]; ids[1] = ids[2]; ids[2] = temp;
    }

  public:
    typedef void ControlSignature(FieldIn<Id3Type>, FieldOut<Id3Type>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    Cid2PointIdWorklet( IdArrayHandle &cidIndexArray, vtkm::Id nPoints )
      : CidIndexRaw ( cidIndexArray.PrepareForInput(DeviceAdapter()) ),
      NPoints(nPoints)
    {}

    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id3 &cid3, vtkm::Id3 &pointId3) const
    {
      if (cid3[0]==cid3[1] || cid3[0]==cid3[2] || cid3[1]==cid3[2])
      {
        pointId3[0] = pointId3[1] = pointId3[2] = this->NPoints ; // invalid cell to be removed
      }
      else
      {
        pointId3[0] = this->CidIndexRaw.Get( cid3[0] );
        pointId3[1] = this->CidIndexRaw.Get( cid3[1] );
        pointId3[2] = this->CidIndexRaw.Get( cid3[2] );
        VTKM_ASSERT_EXEC( pointId3[0] < this->NPoints && pointId3[1] < this->NPoints && pointId3[2] < this->NPoints, *this );

        // Sort triangle point ids so that the same triangle will have the same signature
        // Rotate these ids making the first one the smallest
        if (pointId3[0]>pointId3[1] || pointId3[0]>pointId3[2])
        {
          rotate(pointId3);
          if (pointId3[0]>pointId3[1] || pointId3[0]>pointId3[2])
          {
            rotate(pointId3);
          }
        }
      }
    }
  };


  struct TypeInt64 : vtkm::ListTagBase<vtkm::Int64> { };

  class Cid3HashWorklet : public vtkm::worklet::WorkletMapField
  {
  private:
    vtkm::Int64 NPoints;

  public:
    typedef void ControlSignature(FieldIn<Id3Type> , FieldOut<TypeInt64>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    Cid3HashWorklet(vtkm::Id nPoints)
      : NPoints(nPoints)
    { }

    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id3 &cid, vtkm::Int64 &cidHash) const
    {
      cidHash = cid[0] + this->NPoints  * (cid[1] + this->NPoints * cid[2]);  // get a unique hash value
    }
  };

  class Cid3UnhashWorklet : public vtkm::worklet::WorkletMapField
  {
  private:
    vtkm::Int64 NPoints;

  public:
    typedef void ControlSignature(FieldIn<TypeInt64> , FieldOut<Id3Type>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    Cid3UnhashWorklet(vtkm::Id nPoints)
      : NPoints(nPoints)
    { }

    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Int64 &cidHash, vtkm::Id3 &cid) const
    {
      cid[0] = static_cast<vtkm::Id>( cidHash % this->NPoints  );
      vtkm::Int64 t = cidHash / this->NPoints ;
      cid[1] = static_cast<vtkm::Id>( t % this->NPoints  );
      cid[2] = static_cast<vtkm::Id>( t / this->NPoints  );
    }
  };

  class Id3Less
  {
  public:
    VTKM_EXEC_EXPORT
    bool operator() (const vtkm::Id3 & a, const vtkm::Id3 & b) const
    {
      if (a[0] < 0)
      {
        // invalid id: place at the last after sorting
        // (comparing to 0 is faster than matching -1)
        return false;
      }
      return b[0] < 0 ||
             a[0] < b[0] ||
             (a[0]==b[0] && a[1] < b[1]) ||
             (a[0]==b[0] && a[1]==b[1] && a[2] < b[2]);
    }
  };

  template <typename ValueType>
  void SortAndUnique(vtkm::cont::ArrayHandle<ValueType> &pointId3Array)
  {
    ///
    /// Unique: Decimate replicated cells
    ///
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Sort(pointId3Array);

    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Unique(pointId3Array);
  }

public:

  ///////////////////////////////////////////////////
  /// \brief VertexClustering: Mesh simplification
  ///
  vtkm::cont::DataSet Run(const vtkm::cont::DynamicCellSet &cellSet,
                          const vtkm::cont::CoordinateSystem &coordinates,
                          vtkm::Id nDivisions)
  {
    vtkm::Float64 bounds[6];
    coordinates.GetBounds(bounds, DeviceAdapter());

    /// determine grid resolution for clustering
    GridInfo gridInfo;
    {
      vtkm::Float64 res[3];
      for (vtkm::IdComponent i=0; i<3; i++)
      {
        res[i] = (bounds[i*2+1]-bounds[i*2])/nDivisions;
      }
      gridInfo.grid_width = vtkm::Max(res[0], vtkm::Max(res[1], res[2]));

      vtkm::Float64 inv_grid_width = gridInfo.inv_grid_width = vtkm::Float64(1) / gridInfo.grid_width;

      //printf("Bounds: %lf, %lf, %lf, %lf, %lf, %lf\n", bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
      gridInfo.dim[0] = vtkm::Min((vtkm::Id)vtkm::Ceil((bounds[1]-bounds[0])*inv_grid_width), nDivisions);
      gridInfo.dim[1] = vtkm::Min((vtkm::Id)vtkm::Ceil((bounds[3]-bounds[2])*inv_grid_width), nDivisions);
      gridInfo.dim[2] = vtkm::Min((vtkm::Id)vtkm::Ceil((bounds[5]-bounds[4])*inv_grid_width), nDivisions);

      // center the mesh in the grids
      gridInfo.origin[0] = (bounds[1]+bounds[0])*0.5 - gridInfo.grid_width*(gridInfo.dim[0])*.5;
      gridInfo.origin[1] = (bounds[3]+bounds[2])*0.5 - gridInfo.grid_width*(gridInfo.dim[1])*.5;
      gridInfo.origin[2] = (bounds[5]+bounds[4])*0.5 - gridInfo.grid_width*(gridInfo.dim[2])*.5;
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

    vtkm::worklet::DispatcherMapField<MapPointsWorklet, DeviceAdapter>(
        MapPointsWorklet(gridInfo)).Invoke(coordinates.GetData(), pointCidArray);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time map points (s): " << timer.GetElapsedTime() << std::endl;
#endif


    /// pass 2 : compute average point position for each cluster,
    ///          using pointCidArray as the key
    ///
    vtkm::cont::ArrayHandle<vtkm::Id> pointCidArrayReduced;
    vtkm::cont::DynamicArrayHandle repPointArray;  // representative point

    internal::AverageByKeyDynamicValue<vtkm::cont::ArrayHandle<vtkm::Id>,
                                       vtkm::cont::ArrayHandle<vtkm::Id>,
                                       DeviceAdapter>
        averageByKey(pointCidArray, pointCidArrayReduced, repPointArray);
    coordinates.GetData().CastAndCall(averageByKey);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after averaging (s): " << timer.GetElapsedTime() << std::endl;
#endif

    /// Pass 3 : Decimated mesh generation
    ///          For each original triangle, only output vertices from
    ///          three different clusters

    /// map each triangle vertex to the cluster id's
    /// of the cell vertices
    vtkm::cont::ArrayHandle<vtkm::Id3> cid3Array;

    vtkm::worklet::DispatcherMapTopology<MapCellsWorklet, DeviceAdapter>(
          MapCellsWorklet()).Invoke(cellSet, pointCidArray, cid3Array);

    pointCidArray.ReleaseResources();

    /// preparation: Get the indexes of the clustered points to prepare for new cell array
    vtkm::cont::ArrayHandle<vtkm::Id> cidIndexArray;

    vtkm::worklet::DispatcherMapField<IndexingWorklet, DeviceAdapter> (
      IndexingWorklet(cidIndexArray, gridInfo.dim[0]*gridInfo.dim[1]*gridInfo.dim[2]))
        .Invoke(pointCidArrayReduced);

    pointCidArrayReduced.ReleaseResources();

    ///
    /// map: convert each triangle vertices from original point id to the new cluster indexes
    ///      If the triangle is degenerated, set the ids to <-1, -1, -1>
    ///
    vtkm::Id nPoints = repPointArray.GetNumberOfValues();

    vtkm::cont::ArrayHandle<vtkm::Id3> pointId3Array;

    vtkm::worklet::DispatcherMapField<Cid2PointIdWorklet, DeviceAdapter>(
        Cid2PointIdWorklet( cidIndexArray, nPoints)).Invoke(cid3Array, pointId3Array);

    cid3Array.ReleaseResources();
    cidIndexArray.ReleaseResources();

    bool doHashing = (nPoints < (1<<21));  // Check whether we can hash Id3 into 64-bit integers

    if (doHashing)
    {
      /// Create hashed array
      vtkm::cont::ArrayHandle<vtkm::Int64> pointId3HashArray;

      vtkm::worklet::DispatcherMapField<Cid3HashWorklet, DeviceAdapter>(
          Cid3HashWorklet(nPoints)).Invoke( pointId3Array, pointId3HashArray );

      pointId3Array.ReleaseResources();

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time before sort and unique with hashing (s): " << timer.GetElapsedTime() << std::endl;
#endif

      SortAndUnique(pointId3HashArray);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after sort and unique with hashing (s): " << timer.GetElapsedTime() << std::endl;
#endif

      // decode
      vtkm::worklet::DispatcherMapField<Cid3UnhashWorklet, DeviceAdapter>(
          Cid3UnhashWorklet(nPoints)).Invoke( pointId3HashArray, pointId3Array );

    }
    else
    {

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time before sort and unique [no hashing] (s): " << timer.GetElapsedTime() << std::endl;
#endif

      SortAndUnique(pointId3Array);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique [no hashing] (s): " << timer.GetElapsedTime() << std::endl;
#endif
    }

    // remove the last element if invalid
    vtkm::Id cells = pointId3Array.GetNumberOfValues();
    if (cells > 0 && pointId3Array.GetPortalConstControl().Get(cells-1)[2] >= nPoints )
    {
      cells--;
      pointId3Array.Shrink(cells);
    }

    /// output
    vtkm::cont::DataSet output;

    output.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", 0, repPointArray));

    output.AddCellSet(internal::make_CellSetExplicit(
      vtkm::cont::make_ArrayHandleConstant<vtkm::UInt8>(vtkm::CELL_SHAPE_TRIANGLE, cells),
      vtkm::cont::make_ArrayHandleConstant<vtkm::IdComponent>(3, cells),
      internal::copyFromVec(pointId3Array, DeviceAdapter())));

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
