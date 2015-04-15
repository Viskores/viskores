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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

//#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
//#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_CUDA
//#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/exec/arg/TopologyIdSet.h>
#include <vtkm/exec/arg/TopologyIdCount.h>
#include <vtkm/exec/arg/TopologyElementType.h>

/*
call notes.
wrap execution portal with restrictors. (abandone the fixed length array).
compile time polymorphic types.

*/

static const int LEN_IDS = 6;

class CellType : public vtkm::worklet::WorkletMapTopology
{
public:
  typedef void ControlSignature(FieldDestIn<Scalar> inCells, FieldSrcIn<Scalar> inNodes, TopologyIn<LEN_IDS> topology, FieldDestOut<Scalar> outCells);
  typedef _4 ExecutionSignature(_1, _2, vtkm::exec::arg::TopologyIdCount, vtkm::exec::arg::TopologyElementType, vtkm::exec::arg::TopologyIdSet);
  typedef _3 InputDomain;

  VTKM_CONT_EXPORT
  CellType() { };

  VTKM_EXEC_EXPORT
  vtkm::Float32 operator()(const vtkm::Float32 &cellval,
                           const vtkm::Vec<vtkm::Float32,LEN_IDS> &nodevals,
                           const vtkm::Id &count,
                           const vtkm::Id &type,
                           const vtkm::Vec<vtkm::Id,LEN_IDS> &nodeIDs) const
  {
      std::cout << "CellType worklet: " << std::endl;
      std::cout << "   -- input cell field value: " << cellval << std::endl;
      if (count > LEN_IDS)
          std::cout << "   ++ WARNING: DIDN'T USE A VALUE SET SIZE SUFFICIENTLY LARGE" << std::endl;
      std::cout << "   -- input node field values: ";
      for (int i=0; i<count; ++i)
      {
          std::cout << (i>0?", ":"");
          if (i < LEN_IDS)
              std::cout << nodevals[i];
          else
              std::cout << "?";
      }
      std::cout << std::endl;
      std::cout << "   -- cell type: " << type << std::endl;
      std::cout << "   -- number of IDs for this cell: " << count << std::endl;
      if (count > LEN_IDS)
          std::cout << "   ++ WARNING: DIDN'T USE AN ID SET SIZE SUFFICIENTLY LARGE" << std::endl;
      std::cout << "   -- input node IDs: ";
      for (int i=0; i<count; ++i)
      {
          std::cout << (i>0?", ":"");
          if (i < LEN_IDS)
              std::cout << nodeIDs[i];
          else
              std::cout << "?";
      }
      std::cout << std::endl;
      return (vtkm::Float32)cellval;
  }

};

void TestDataSet_Explicit()
{
    std::cout << std::endl;
    std::cout << "--TestDataSet_Explicit--" << std::endl << std::endl;
    vtkm::cont::DataSet ds;
    
    ds.x_idx = 0;
    ds.y_idx = 1;
    ds.z_idx = 2;

    const int nVerts = 5;
    vtkm::Float32 xVals[nVerts] = {0, 1, 1, 2, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 1, 1, 2};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0};
    vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.2, 40.2, 50.3};


    ds.AddFieldViaCopy(xVals, nVerts);
    ds.AddFieldViaCopy(yVals, nVerts);
    ds.AddFieldViaCopy(zVals, nVerts);

    //Set node scalar
    ds.AddFieldViaCopy(vars, nVerts);

    //Set cell scalar
    vtkm::Float32 cellvar[2] = {100.1, 100.2};
    ds.AddFieldViaCopy(cellvar, 2);

    //Add connectivity
    vtkm::cont::ArrayHandle<vtkm::Id> tmp2;
    std::vector<vtkm::Id> shapes;
    shapes.push_back(vtkm::VTKM_TRIANGLE);
    shapes.push_back(vtkm::VTKM_QUAD);

    std::vector<vtkm::Id> numindices;
    numindices.push_back(3);
    numindices.push_back(4);

    std::vector<vtkm::Id> conn;
    // First Cell: Triangle
    conn.push_back(0);
    conn.push_back(1);
    conn.push_back(2);
    // Second Cell: Quad
    conn.push_back(2);
    conn.push_back(1);
    conn.push_back(3);
    conn.push_back(4);

    std::vector<vtkm::Id> map_cell_to_iondex;
    map_cell_to_iondex.push_back(0);
    map_cell_to_iondex.push_back(3);

    vtkm::cont::CellSetExplicit *cs = new vtkm::cont::CellSetExplicit;
    
    tmp2 = vtkm::cont::make_ArrayHandle(shapes);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp2, cs->nodesOfCellsConnectivity.Shapes);

    tmp2 = vtkm::cont::make_ArrayHandle(numindices);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp2, cs->nodesOfCellsConnectivity.NumIndices);

    tmp2 = vtkm::cont::make_ArrayHandle(conn);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp2, cs->nodesOfCellsConnectivity.Connectivity);

    tmp2 = vtkm::cont::make_ArrayHandle(map_cell_to_iondex);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp2, cs->nodesOfCellsConnectivity.MapCellToConnectivityIndex);


    //Run a worklet to populate a cell centered field.
    //Here, we're filling it with test values.
    vtkm::Float32 outcellVals[2] = {-1.4, -1.7};
    ds.AddFieldViaCopy(outcellVals, 2);

    vtkm::worklet::DispatcherMapTopology<CellType> dispatcher;
    dispatcher.Invoke(ds.GetField(4).GetData(), ds.GetField(3).GetData(), cs->nodesOfCellsConnectivity, ds.GetField(5).GetData());

#if 0
    //Add some verts.
    ds.GetField(ds.x_idx).PrepareForOutput(nVerts, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    ds.GetField(ds.y_idx).PrepareForOutput(nVerts, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    ds.GetField(ds.z_idx).PrepareForOutput(nVerts, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    
    //v0 = (0,0,0)
    ds.GetField(ds.x_idx).GetPortalControl().Set(0, 0.0);
    ds.GetField(ds.y_idx).GetPortalControl().Set(0, 0.0);
    ds.GetField(ds.z_idx).GetPortalControl().Set(0, 0.0);

    //v1 = (1,0,0)
    ds.GetField(ds.x_idx).GetPortalControl().Set(1, 1.0);
    ds.GetField(ds.y_idx).GetPortalControl().Set(1, 0.0);
    ds.GetField(ds.z_idx).GetPortalControl().Set(1, 0.0);

    //v2 = (1,1,0)
    ds.GetField(ds.x_idx).GetPortalControl().Set(2, 1.0);
    ds.GetField(ds.y_idx).GetPortalControl().Set(2, 1.0);
    ds.GetField(ds.z_idx).GetPortalControl().Set(2, 0.0);

    //scalar
    ds.GetField(3).PrepareForOutput(nVerts, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    for (int i = 0; i < nVerts; i++)
	ds.GetField(3).GetPortalControl().Set(i, i*10.0);
#endif

}

void TestDataSet_Regular()
{
    std::cout << std::endl;
    std::cout << "--TestDataSet_Regular--" << std::endl << std::endl;
    vtkm::cont::DataSet ds;
    
    ds.x_idx = 0;
    ds.y_idx = 1;
    ds.z_idx = 2;

    const int nVerts = 18;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.1, 40.1, 50.2, 60.2, 70.2, 80.2, 90.3, 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5};

    ds.AddFieldViaCopy(xVals, nVerts);
    ds.AddFieldViaCopy(yVals, nVerts);
    ds.AddFieldViaCopy(zVals, nVerts);

    //Set node scalar
    ds.AddFieldViaCopy(vars, nVerts);

    //Set cell scalar
    vtkm::Float32 cellvar[4] = {100.1, 100.2, 100.3, 100.4};
    ds.AddFieldViaCopy(cellvar, 4);


    vtkm::cont::CellSetStructured *cs = new vtkm::cont::CellSetStructured;

    //Set regular structure
    cs->structure.SetNodeDimension(3,2,3);

    //Run a worklet to populate a cell centered field.
    vtkm::Float32 cellVals[4] = {-1.1, -1.2, -1.3, -1.4};
    ds.AddFieldViaCopy(cellVals, 4);

    vtkm::worklet::DispatcherMapTopology<CellType> dispatcher;
    dispatcher.Invoke(ds.GetField(4).GetData(), ds.GetField(3).GetData(), cs->structure, ds.GetField(5).GetData());
}

int UnitTestDataSet(int, char *[])
{
    int err = 0;
    err += vtkm::cont::testing::Testing::Run(TestDataSet_Explicit);
    err += vtkm::cont::testing::Testing::Run(TestDataSet_Regular);
    return err;
}
