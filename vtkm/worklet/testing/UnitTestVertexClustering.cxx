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

#include <iostream>
#include <algorithm>

#include <vtkm/worklet/PointElevation.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>


#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/Pair.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>

#include <vtkm/cont/Field.h>
#include <vtkm/cont/ExplicitConnectivity.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/worklet/AverageByKey.h>

namespace{

template <class DeviceAdapter>
struct VertexClustering{

  typedef vtkm::Vec<vtkm::Float32,3> Vector3;
  typedef Vector3 PointType;

  struct GridInfo
  {
      int dim[3];
      Vector3 origin;
      float grid_width;
      float inv_grid_width; // = 1/grid_width
  };

  // input: points  output: cid of the points
  class MapPointsWorklet : public vtkm::worklet::WorkletMapField {
  private:
      const VTKM_EXEC_CONSTANT_EXPORT GridInfo grid;
  public:
      typedef void ControlSignature(FieldIn<> , FieldOut<>);
      typedef void ExecutionSignature(_1, _2);

      VTKM_CONT_EXPORT
      MapPointsWorklet(const GridInfo &grid_)
          : grid(grid_)
      { }

      /// determine grid resolution for clustering
      VTKM_EXEC_EXPORT
      vtkm::Id get_cluster_id( const Vector3 &p) const
      {
          Vector3 p_rel = (p - grid.origin) * grid.inv_grid_width;
          vtkm::Id x = std::min((int)p_rel[0], grid.dim[0]-1);
          vtkm::Id y = std::min((int)p_rel[1], grid.dim[1]-1);
          vtkm::Id z = std::min((int)p_rel[2], grid.dim[2]-1);
          return x + grid.dim[0] * (y + grid.dim[1] * z);  // get a unique hash value
      }

      VTKM_EXEC_EXPORT
      void operator()(const PointType &point, vtkm::Id &cid) const
      {
          cid = get_cluster_id(point);
          VTKM_ASSERT_CONT(cid>=0);  // the id could overflow if too many cells
      }
  };


  class MapCellsWorklet: public vtkm::worklet::WorkletMapField {
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
    IdPortalType pointIdPortal;
    IdPortalType pointCidPortal;
    IdPortalType numIndicesPortal;
  public:
    typedef void ControlSignature(FieldIn<> , FieldOut<>);
    typedef void ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    MapCellsWorklet(
        const IdArrayHandle &pointIdArray,   // the given point Ids
        const IdArrayHandle &pointCidArray)   // the cluser ids each pointId will map to
      : pointIdPortal(pointIdArray.PrepareForInput(DeviceAdapter()) ),
        pointCidPortal( pointCidArray.PrepareForInput(DeviceAdapter()))
    { }

    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &pointIdIndex, vtkm::Id3 &cid3) const
    {
      VTKM_ASSERT_CONT(pointIdIndex % 3 == 0);  // TODO: may ignore non-triangle cells
      // assume it's a triangle
      cid3[0] = pointCidPortal.Get( pointIdPortal.Get(pointIdIndex) );
      cid3[1] = pointCidPortal.Get( pointIdPortal.Get(pointIdIndex+1) );
      cid3[2] = pointCidPortal.Get( pointIdPortal.Get(pointIdIndex+2) );
    }
  };

  /// pass 3
  class IndexingWorklet : public vtkm::worklet::WorkletMapField
  {
  public:
      typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
  private:
      typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::Portal IdPortalType;
      IdArrayHandle cidIndexArray;
      IdPortalType cidIndexRaw;
  public:
      typedef void ControlSignature(FieldIn<>, FieldIn<>);
      typedef void ExecutionSignature(_1, _2);

      VTKM_CONT_EXPORT
      IndexingWorklet( size_t n )
      {
          cidIndexRaw = cidIndexArray.PrepareForOutput(n, DeviceAdapter() );
      }

      VTKM_EXEC_EXPORT
      void operator()(const vtkm::Id &counter, const vtkm::Id &cid) const
      {
          cidIndexRaw.Set(cid, counter);
          //printf("cid[%d] = %d\n", cid, counter);
      }

      VTKM_CONT_EXPORT
      IdArrayHandle &getOutput()
      {
          return cidIndexArray;
      }
  };


  class Cid2PointIdWorklet : public vtkm::worklet::WorkletMapField
  {
  public:
      typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
  private:
      typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
      const IdPortalType cidIndexRaw;
  public:
      typedef void ControlSignature(FieldIn<>, FieldOut<>);
      typedef void ExecutionSignature(_1, _2);

      VTKM_CONT_EXPORT
      Cid2PointIdWorklet( IdArrayHandle &cidIndexArray )
          : cidIndexRaw ( cidIndexArray.PrepareForInput(DeviceAdapter()) )
      {
      }

      VTKM_EXEC_EXPORT
      void operator()(const vtkm::Id3 &cid3, vtkm::Id3 &pointId3) const
      {
          if (cid3[0]==cid3[1] || cid3[0]==cid3[2] || cid3[1]==cid3[2])
          {
              pointId3[0] = pointId3[1] = pointId3[2] = std::numeric_limits<vtkm::Id>::max();
          } else {
              pointId3[0] = cidIndexRaw.Get( cid3[0] );
              pointId3[1] = cidIndexRaw.Get( cid3[1] );
              pointId3[2] = cidIndexRaw.Get( cid3[2] );
          }
      }

  };


  class Id3Less{
  public:
      VTKM_EXEC_EXPORT
      bool operator() (const vtkm::Id3 & a, const vtkm::Id3 & b) const
      {
          return (a[0] < b[0] ||
              (a[0]==b[0] && a[1] < b[1]) ||
              (a[0]==b[0] && a[1]==b[1] && a[2] < b[2]));
      }
  };

  template<typename T, int N>
  vtkm::cont::ArrayHandle<T> copyFromVec( vtkm::cont::ArrayHandle< vtkm::Vec<T, N> > const& other)
  {
      const T *vmem = reinterpret_cast< const T *>(& *other.GetPortalConstControl().GetRawIterator());
      vtkm::cont::ArrayHandle<T> mem = vtkm::cont::make_ArrayHandle(vmem, other.GetNumberOfValues()*N);
      vtkm::cont::ArrayHandle<T> result;
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(mem,result);
      return result;
  }

  template<typename T, typename StorageTag>
  vtkm::cont::ArrayHandle<T> copyFromImplicit( vtkm::cont::ArrayHandle<T, StorageTag> const& other)
  {
    vtkm::cont::ArrayHandle<T> result;
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(other, result);
    return result;
  }


  ///////////////////////////////////////////////////
  /// \brief VertexClustering: Mesh simplification
  /// \param ds : dataset
  /// \param bounds: dataset bounds
  /// \param nDivisions : number of max divisions per dimension
  ///
  vtkm::cont::DataSet run(vtkm::cont::DataSet &ds, double bounds[6], int nDivisions)
  {

    boost::shared_ptr<vtkm::cont::CellSet> scs = ds.GetCellSet(0);
    vtkm::cont::CellSetExplicit<> *cs =
        dynamic_cast<vtkm::cont::CellSetExplicit<> *>(scs.get());

    vtkm::cont::ArrayHandle<PointType> pointArray = ds.GetField("xyz").GetData().CastToArrayHandle<PointType, VTKM_DEFAULT_STORAGE_TAG>();
    vtkm::cont::ArrayHandle<vtkm::Id> pointIdArray = cs->GetNodeToCellConnectivity().GetConnectivityArray();
    vtkm::cont::ArrayHandle<vtkm::Id> cellToConnectivityIndexArray = cs->GetNodeToCellConnectivity().GetCellToConnectivityIndexArray();

    /// determine grid resolution for clustering
    GridInfo gridInfo;
    {
      double res[3];
      for (int i=0; i<3; i++)
          res[i] = (bounds[i*2+1]-bounds[i*2])/nDivisions;
      gridInfo.grid_width = std::max(res[0], std::min(res[1], res[2]));

      double inv_grid_width = gridInfo.inv_grid_width = 1. / gridInfo.grid_width;

      //printf("Bounds: %lf, %lf, %lf, %lf, %lf, %lf\n", bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
      gridInfo.dim[0] = ceil((bounds[1]-bounds[0])*inv_grid_width);
      gridInfo.dim[1] = ceil((bounds[3]-bounds[2])*inv_grid_width);
      gridInfo.dim[2] = ceil((bounds[5]-bounds[4])*inv_grid_width);
      gridInfo.origin[0] = (bounds[1]+bounds[0])*0.5 - gridInfo.grid_width*(gridInfo.dim[0])*.5;
      gridInfo.origin[1] = (bounds[3]+bounds[2])*0.5 - gridInfo.grid_width*(gridInfo.dim[1])*.5;
      gridInfo.origin[2] = (bounds[5]+bounds[4])*0.5 - gridInfo.grid_width*(gridInfo.dim[2])*.5;
    }

    //construct the scheduler that will execute all the worklets
    vtkm::cont::Timer<> timer;

    //////////////////////////////////////////////
    /// start algorithm

    /// pass 1 : assign points with (cluster) ids based on the grid it falls in
    ///
    /// map points
    vtkm::cont::ArrayHandle<vtkm::Id> pointCidArray;

    vtkm::worklet::DispatcherMapField<MapPointsWorklet>(MapPointsWorklet(gridInfo))
        .Invoke(pointArray, pointCidArray );

    std::cout << "Time map points (s): " << timer.GetElapsedTime() << std::endl;


    /// pass 2 : compute average point position for each cluster,
    ///          using pointCidArray as the key
    ///
    vtkm::cont::ArrayHandle<vtkm::Id> pointCidArrayReduced;
    vtkm::cont::ArrayHandle<Vector3> repPointArray;  // representative point

    vtkm::worklet::
      AverageByKey( pointCidArray, pointArray, pointCidArrayReduced, repPointArray );

    std::cout << "Time after averaging (s): " << timer.GetElapsedTime() << std::endl;


    /// Pass 3 : Decimated mesh generation
    ///          For each original triangle, only output vertices from
    ///          three different clusters

    /// map each triangle vertex to the cluser id's
    /// of the cell vertices
    vtkm::cont::ArrayHandle<vtkm::Id3> cid3Array;

    vtkm::worklet::DispatcherMapField<MapCellsWorklet>(
          MapCellsWorklet(pointIdArray, pointCidArray)
          ).Invoke(cellToConnectivityIndexArray, cid3Array );


    /// preparation: Get the indexes of the clustered points to prepare for new cell array
    /// The output indexes are stored in the worklet
    vtkm::cont::ArrayHandleCounting<vtkm::Id> counterArray3(0, pointCidArrayReduced.GetNumberOfValues());
    IndexingWorklet worklet3 ( gridInfo.dim[0]*gridInfo.dim[1]*gridInfo.dim[2] );

    vtkm::worklet::DispatcherMapField<IndexingWorklet> ( worklet3 )
                                  .Invoke(counterArray3, pointCidArrayReduced);

    ///
    /// map: convert each triangle vertices from original point id to the new cluster indexes
    ///      If the triangle is degenerated, set the ids to <-1, -1, -1>
    ///
    vtkm::cont::ArrayHandle<vtkm::Id3> pointId3Array;

    vtkm::worklet::DispatcherMapField<Cid2PointIdWorklet> ( Cid2PointIdWorklet( worklet3.getOutput() ) )
                                                    .Invoke(cid3Array, pointId3Array);


    std::cout << "Time before unique (s): " << timer.GetElapsedTime() << std::endl;

    ///
    /// Unique: Decimate replicated cells
    ///
    vtkm::cont::ArrayHandle<vtkm::Id3 > uniquePointId3Array;

    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(pointId3Array,uniquePointId3Array);

    std::cout << "Time after copy (s): " << timer.GetElapsedTime() << std::endl;

    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Sort(uniquePointId3Array, Id3Less());

    std::cout << "Time after sort (s): " << timer.GetElapsedTime() << std::endl;

    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Unique(uniquePointId3Array);

    std::cout << "Time after unique (s): " << timer.GetElapsedTime() << std::endl;


    /// generate output

    vtkm::cont::DataSet new_ds;

    new_ds.AddField(vtkm::cont::Field("xyz", 0, vtkm::cont::Field::ASSOC_POINTS, repPointArray));
    new_ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("xyz"));

    {
      int cells = uniquePointId3Array.GetNumberOfValues();
      cells-- ;
      uniquePointId3Array.Shrink(cells);  // remove the last invalid one

      //typedef typename vtkm::cont::ArrayHandleConstant<vtkm::Id>::StorageTag ConstantStorage;
      //typedef typename vtkm::cont::ArrayHandleImplicit<vtkm::Id, CounterOfThree>::StorageTag CountingStorage;
      typedef vtkm::cont::CellSetExplicit<> Connectivity;

      boost::shared_ptr< Connectivity > new_cs(
          new Connectivity("cells", 0) );

      if (cells > 0)
        new_cs->GetNodeToCellConnectivity().Fill(
          copyFromImplicit(vtkm::cont::make_ArrayHandleConstant<vtkm::Id>(vtkm::VTKM_TRIANGLE, cells)),
          copyFromImplicit(vtkm::cont::make_ArrayHandleConstant<vtkm::Id>(3, cells)),
          copyFromVec(uniquePointId3Array)
              );

      //print_array("uniquePointId3Array", uniquePointId3Array);
      //print_array("CellToConnectivityIndexArray", new_cs->GetNodeToCellConnectivity().GetCellToConnectivityIndexArray());
      //print_array("PointIdArray", new_cs->GetNodeToCellConnectivity().GetConnectivityArray());

      new_ds.AddCellSet(new_cs);
    }


    /// end of algorithm
    /// Note that there is a cell with ids <-1, -1, -1>.
    /// The removal of it is deferred to the conversion to VTK
    /// ////////////////////////////////////////
    double t = timer.GetElapsedTime();
    std::cout << "Time (s): " << t << std::endl;

    return new_ds;
  }
}; // struct VertexClustering

void TestVertexClustering()
{
  double bounds[6];
  const int divisions = 4;
  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet ds = maker.Make3DExplicitDataSetCowNose(bounds);
  vtkm::cont::DataSet ds_out = VertexClustering<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().run(ds, bounds, divisions);
}
} // namespace

int UnitTestVertexClustering(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestVertexClustering);
}

