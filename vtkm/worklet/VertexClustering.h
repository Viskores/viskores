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

#include <limits>
#include <vtkm/Math.h>

#include <vtkm/exec/Assert.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/Pair.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>

#include <vtkm/cont/Field.h>
#include <vtkm/cont/ExplicitConnectivity.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/worklet/AverageByKey.h>

//#define __VTKM_VERTEX_CLUSTERING_BENCHMARK
//#include <vtkm/cont/Timer.h>

namespace vtkm{ namespace worklet
{

template<typename DeviceAdapter>
struct VertexClustering{

  template<typename PointType>
  struct GridInfo
  {
    vtkm::Id dim[3];
    PointType origin;
    typename PointType::ComponentType grid_width;
    typename PointType::ComponentType inv_grid_width; // = 1/grid_width
  };

  // input: points  output: cid of the points
  template<typename PointType>
  class MapPointsWorklet : public vtkm::worklet::WorkletMapField {
  private:
    //const VTKM_EXEC_CONSTANT_EXPORT GridInfo<PointType> Grid;
    GridInfo<PointType> Grid;
  public:
    typedef void ControlSignature(FieldIn<> , FieldOut<>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    MapPointsWorklet(const GridInfo<PointType> &grid)
      : Grid(grid)
    { }

    /// determine grid resolution for clustering
    VTKM_EXEC_EXPORT
    vtkm::Id GetClusterId(const PointType &p) const
    {
      PointType p_rel = (p - this->Grid.origin) * this->Grid.inv_grid_width;
      vtkm::Id x = vtkm::Min((vtkm::Id)p_rel[0], this->Grid.dim[0]-1);
      vtkm::Id y = vtkm::Min((vtkm::Id)p_rel[1], this->Grid.dim[1]-1);
      vtkm::Id z = vtkm::Min((vtkm::Id)p_rel[2], this->Grid.dim[2]-1);
      return x + this->Grid.dim[0] * (y + this->Grid.dim[1] * z);  // get a unique hash value
    }

    VTKM_EXEC_EXPORT
    void operator()(const PointType &point, vtkm::Id &cid) const
    {
      cid = this->GetClusterId(point);
      VTKM_ASSERT_EXEC(cid>=0, *this);  // the id could overflow if too many cells
    }
  };


  class MapCellsWorklet: public vtkm::worklet::WorkletMapField {
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
    IdPortalType PointIdPortal;
    IdPortalType PointCidPortal;
    IdPortalType NumIndicesPortal;
  public:
    typedef void ControlSignature(FieldIn<> , FieldOut<>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    MapCellsWorklet(
        const IdArrayHandle &pointIdArray,   // the given point Ids
        const IdArrayHandle &pointCidArray)   // the cluser ids each pointId will map to
      : PointIdPortal(pointIdArray.PrepareForInput(DeviceAdapter())),
        PointCidPortal(pointCidArray.PrepareForInput(DeviceAdapter()))
    { }

    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &pointIdIndex, vtkm::Id3 &cid3) const
    {
      //VTKM_ASSERT_EXEC(pointIdIndex % 3 == 0, *this);  // TODO: may ignore non-triangle cells
      // assume it's a triangle
      cid3[0] = this->PointCidPortal.Get( this->PointIdPortal.Get(pointIdIndex) );
      cid3[1] = this->PointCidPortal.Get( this->PointIdPortal.Get(pointIdIndex+1) );
      cid3[2] = this->PointCidPortal.Get( this->PointIdPortal.Get(pointIdIndex+2) );
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
    vtkm::Id len;
  public:
      typedef void ControlSignature(FieldIn<>);
      typedef void ExecutionSignature(WorkIndex, _1);  // WorkIndex: use vtkm indexing

      VTKM_CONT_EXPORT
      IndexingWorklet( IdArrayHandle &cidIndexArray, vtkm::Id n ) : len(n)
      {
        this->CidIndexRaw = cidIndexArray.PrepareForOutput(n, DeviceAdapter() );
      }

      VTKM_EXEC_EXPORT
      void operator()(const vtkm::Id &counter, const vtkm::Id &cid) const
      {
        VTKM_ASSERT_EXEC( cid < this->len , *this );
        this->CidIndexRaw.Set(cid, counter);
      }
  };

  class Cid2PointIdWorklet : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
  private:
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
    const IdPortalType CidIndexRaw;
    vtkm::Id nPoints;
  public:
    typedef void ControlSignature(FieldIn<>, FieldOut<>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    Cid2PointIdWorklet( IdArrayHandle &cidIndexArray, vtkm::Id nPoints_ )
      : CidIndexRaw ( cidIndexArray.PrepareForInput(DeviceAdapter()) ),
      nPoints(nPoints_)
    {
    }

    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id3 &cid3, vtkm::Id3 &pointId3) const
    {
      if (cid3[0]==cid3[1] || cid3[0]==cid3[2] || cid3[1]==cid3[2])
      {
        pointId3[0] = pointId3[1] = pointId3[2] = this->nPoints ; // invalid cell to be removed
      } else {
        pointId3[0] = this->CidIndexRaw.Get( cid3[0] );
        pointId3[1] = this->CidIndexRaw.Get( cid3[1] );
        pointId3[2] = this->CidIndexRaw.Get( cid3[2] );
        VTKM_ASSERT_EXEC( pointId3[0] < nPoints && pointId3[1] < nPoints && pointId3[2] < nPoints , *this );
      }
    }

  };

  class Cid3HashWorklet : public vtkm::worklet::WorkletMapField {
  private:
    vtkm::Int64 nPoints;
  public:
    typedef void ControlSignature(FieldIn<> , FieldOut<>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    Cid3HashWorklet(vtkm::Id nPoints_)
      : nPoints(nPoints_)
    { }

    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id3 &cid, vtkm::Int64 &cidHash) const
    {
      cidHash = cid[0] + nPoints * (cid[1] + nPoints * cid[2]);  // get a unique hash value
    }
  };

  class Cid3UnhashWorklet : public vtkm::worklet::WorkletMapField {
  private:
    vtkm::Int64 nPoints;
  public:
    typedef void ControlSignature(FieldIn<> , FieldOut<>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    Cid3UnhashWorklet(vtkm::Id nPoints_)
      : nPoints(nPoints_)
    { }

    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Int64 &cidHash, vtkm::Id3 &cid) const
    {
      cid[0] = static_cast<vtkm::Id>( cidHash % nPoints );
      vtkm::Int64 t = cidHash / nPoints;
      cid[1] = static_cast<vtkm::Id>( t % nPoints );
      cid[2] = static_cast<vtkm::Id>( t / nPoints );
    }
  };

  class Id3Less{
  public:
    VTKM_EXEC_EXPORT
    bool operator() (const vtkm::Id3 & a, const vtkm::Id3 & b) const
    {
      if (a[0] < 0) // invalid id: place at the last after sorting (comparing to 0 is faster than matching -1)
        return false;
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
  /// \param ds : dataset
  /// \param bounds: dataset bounds
  /// \param nDivisions : number of max divisions per dimension
  template <typename FloatType,
            typename BoundsType,
            typename StorageT,
            typename StorageU,
            typename StorageV>
  void run(const vtkm::cont::ArrayHandle<vtkm::Vec<FloatType,3>, StorageT> pointArray,
           const vtkm::cont::ArrayHandle<vtkm::Id, StorageU>  pointIdArray,
           const vtkm::cont::ArrayHandle<vtkm::Id, StorageV>  cellToConnectivityIndexArray,
           const BoundsType bounds[6], vtkm::Id nDivisions,
           vtkm::cont::ArrayHandle<vtkm::Vec<FloatType,3> > &output_pointArray,
           vtkm::cont::ArrayHandle<vtkm::Id3> &output_pointId3Array)
  {
    typedef vtkm::Vec<FloatType,3> PointType;

    /// determine grid resolution for clustering
    GridInfo<PointType> gridInfo;
    {
      FloatType res[3];
      for (vtkm::IdComponent i=0; i<3; i++)
      {
        res[i] = static_cast<FloatType>((bounds[i*2+1]-bounds[i*2])/nDivisions);
      }
      gridInfo.grid_width = vtkm::Max(res[0], vtkm::Max(res[1], res[2]));

      FloatType inv_grid_width = gridInfo.inv_grid_width = FloatType(1) / gridInfo.grid_width;

      //printf("Bounds: %lf, %lf, %lf, %lf, %lf, %lf\n", bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
      gridInfo.dim[0] = vtkm::Min((vtkm::Id)vtkm::Ceil((bounds[1]-bounds[0])*inv_grid_width), nDivisions);
      gridInfo.dim[1] = vtkm::Min((vtkm::Id)vtkm::Ceil((bounds[3]-bounds[2])*inv_grid_width), nDivisions);
      gridInfo.dim[2] = vtkm::Min((vtkm::Id)vtkm::Ceil((bounds[5]-bounds[4])*inv_grid_width), nDivisions);

      // center the mesh in the grids
      gridInfo.origin[0] = static_cast<FloatType>((bounds[1]+bounds[0])*0.5 - gridInfo.grid_width*(gridInfo.dim[0])*.5);
      gridInfo.origin[1] = static_cast<FloatType>((bounds[3]+bounds[2])*0.5 - gridInfo.grid_width*(gridInfo.dim[1])*.5);
      gridInfo.origin[2] = static_cast<FloatType>((bounds[5]+bounds[4])*0.5 - gridInfo.grid_width*(gridInfo.dim[2])*.5);
    }

    if( static_cast<vtkm::Int64>(gridInfo.dim[0])*gridInfo.dim[1]*gridInfo.dim[2] > std::numeric_limits<vtkm::Id>::max() )
      throw vtkm::cont::ErrorControlBadValue("Grid resolution larger than vtkm::Id capacity. ");
    // Use 64-bit id will solve the issue.

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

    vtkm::worklet::DispatcherMapField<MapPointsWorklet<PointType> >(
          MapPointsWorklet<PointType>(gridInfo))
        .Invoke(pointArray, pointCidArray );

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time map points (s): " << timer.GetElapsedTime() << std::endl;
#endif


    /// pass 2 : compute average point position for each cluster,
    ///          using pointCidArray as the key
    ///
    vtkm::cont::ArrayHandle<vtkm::Id> pointCidArrayReduced;
    vtkm::cont::ArrayHandle<PointType> repPointArray;  // representative point

    vtkm::worklet::
      AverageByKey( pointCidArray, pointArray, pointCidArrayReduced, repPointArray );

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after averaging (s): " << timer.GetElapsedTime() << std::endl;
#endif

    /// Pass 3 : Decimated mesh generation
    ///          For each original triangle, only output vertices from
    ///          three different clusters

    /// map each triangle vertex to the cluser id's
    /// of the cell vertices
    vtkm::cont::ArrayHandle<vtkm::Id3> cid3Array;

    vtkm::worklet::DispatcherMapField<MapCellsWorklet>(
          MapCellsWorklet(pointIdArray, pointCidArray)
          ).Invoke(cellToConnectivityIndexArray, cid3Array );

    pointCidArray.ReleaseResources();

    /// preparation: Get the indexes of the clustered points to prepare for new cell array
    vtkm::cont::ArrayHandle<vtkm::Id> cidIndexArray;

    vtkm::worklet::DispatcherMapField<IndexingWorklet> ( IndexingWorklet( cidIndexArray, gridInfo.dim[0]*gridInfo.dim[1]*gridInfo.dim[2] ) )
        .Invoke(pointCidArrayReduced);

    pointCidArrayReduced.ReleaseResources();

    ///
    /// map: convert each triangle vertices from original point id to the new cluster indexes
    ///      If the triangle is degenerated, set the ids to <-1, -1, -1>
    ///    
    vtkm::Id nPoints = repPointArray.GetNumberOfValues();

    vtkm::cont::ArrayHandle<vtkm::Id3> pointId3Array;

    vtkm::worklet::DispatcherMapField<Cid2PointIdWorklet>( Cid2PointIdWorklet( cidIndexArray, nPoints ) )
        .Invoke(cid3Array, pointId3Array);

    cid3Array.ReleaseResources();
    cidIndexArray.ReleaseResources();

    bool doHashing = (nPoints < (1<<21));  // Check whether we can hash Id3 into 64-bit integers

    if (doHashing) {
      /// Create hashed array
      vtkm::cont::ArrayHandle<vtkm::Int64> pointId3HashArray;

      vtkm::worklet::DispatcherMapField<Cid3HashWorklet>( Cid3HashWorklet( nPoints ) )
          .Invoke( pointId3Array, pointId3HashArray );

      pointId3Array.ReleaseResources();

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time before sort and unique with hashing (s): " << timer.GetElapsedTime() << std::endl;
#endif

      SortAndUnique(pointId3HashArray);

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after sort and unique with hashing (s): " << timer.GetElapsedTime() << std::endl;
#endif

      // decode
      vtkm::worklet::DispatcherMapField<Cid3UnhashWorklet>(Cid3UnhashWorklet(nPoints) )
          .Invoke( pointId3HashArray, pointId3Array );

    } else {

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
    if (cells > 0 && pointId3Array.GetPortalConstControl().Get(cells-1)[2] >= nPoints ) {
      cells-- ;
      pointId3Array.Shrink(cells);
    }

    /// output
    output_pointId3Array = pointId3Array;
    output_pointArray = repPointArray;

#ifdef __VTKM_VERTEX_CLUSTERING_BENCHMARK
    vtkm::Float64 t = timer.GetElapsedTime();
    std::cout << "Time (s): " << t << std::endl;
    std::cout << "number of output points: " << repPointArray.GetNumberOfValues() << std::endl;
    std::cout << "number of output cells: " << pointId3Array.GetNumberOfValues() << std::endl;
#endif

  }
}; // struct VertexClustering


}} // namespace vtkm::worklet

#endif // vtk_m_worklet_VertexClustering_h

