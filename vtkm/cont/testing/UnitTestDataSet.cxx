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

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/worklet/WorkletMapCell.h>
#include <vtkm/worklet/DispatcherMapCell.h>
#include <vtkm/exec/arg/TopologyIdSet.h>
#include <vtkm/exec/arg/TopologyIdCount.h>
#include <vtkm/exec/arg/TopologyElementType.h>

class CellType : public vtkm::worklet::WorkletMapCell
{
public:
  typedef void ControlSignature(FieldCellIn<Scalar> inCells, FieldNodeIn<Scalar> inNodes, TopologyIn topology, FieldCellOut<Scalar> outCells);
  typedef _4 ExecutionSignature(_1, _2, vtkm::exec::arg::TopologyIdCount, vtkm::exec::arg::TopologyElementType, vtkm::exec::arg::TopologyIdSet);
  typedef _3 InputDomain;

  VTKM_CONT_EXPORT
  CellType() { };

  VTKM_EXEC_EXPORT
  vtkm::Float32 operator()(const vtkm::Float32 &cellval, const vtkm::Vec<vtkm::Float32,8> &nodevals, const vtkm::Id &count, const vtkm::Id &type, const vtkm::Vec<vtkm::Id,8> &nodeIDs) const
  {
      std::cout << "CellType worklet: " << std::endl;
      std::cout << "   -- input cell field value: " << cellval << std::endl;
      std::cout << "   -- input node field values: ";
      for (int i=0; i<count; ++i)
          std::cout << (i>0?",":"") << nodevals[i];
      std::cout << std::endl;
      std::cout << "   -- cell type: " << type << std::endl;
      std::cout << "   -- number of IDs for this cell: " << count << std::endl;
      std::cout << "   -- input node IDs: ";
      for (int i=0; i<count; ++i)
          std::cout << (i>0?",":"") << nodeIDs[i];
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
    ds.Fields.resize(6);
    ds.Fields[0] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    ds.Fields[1] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    ds.Fields[2] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    ds.Fields[3] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    ds.Fields[4] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    ds.Fields[5] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();

    const int nVerts = 5;
    vtkm::Float32 xVals[nVerts] = {0, 1, 1, 2, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 1, 1, 2};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0};
    vtkm::Float32 vars[nVerts] = {10, 20, 30, 40, 50};


    vtkm::cont::ArrayHandle<vtkm::Float32> tmp;
    //Set X, Y, and Z fields.
    tmp = vtkm::cont::make_ArrayHandle(xVals, nVerts);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[ds.x_idx]);
    tmp = vtkm::cont::make_ArrayHandle(yVals, nVerts);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[ds.y_idx]);
    tmp = vtkm::cont::make_ArrayHandle(zVals, nVerts);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[ds.z_idx]);

    //Set node scalar
    tmp = vtkm::cont::make_ArrayHandle(vars, nVerts);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[3]);

    //Set cell scalar
    vtkm::Float32 cellvar[2] = {100.1, 100.2};
    tmp = vtkm::cont::make_ArrayHandle(cellvar, 2);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[4]);

    //Add connectivity
    vtkm::cont::ArrayHandle<vtkm::Id> tmp2;
    std::vector<vtkm::Id> shapes;
    shapes.push_back(vtkm::cont::VTKM_TRI);
    shapes.push_back(vtkm::cont::VTKM_QUAD);

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

    
    tmp2 = vtkm::cont::make_ArrayHandle(shapes);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp2, ds.conn.Shapes);

    tmp2 = vtkm::cont::make_ArrayHandle(numindices);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp2, ds.conn.NumIndices);

    tmp2 = vtkm::cont::make_ArrayHandle(conn);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp2, ds.conn.Connectivity);

    tmp2 = vtkm::cont::make_ArrayHandle(map_cell_to_iondex);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp2, ds.conn.MapCellToConnectivityIndex);


    //Run a worklet to populate a cell centered field.
    //Here, we're filling it with test values.
    vtkm::Float32 outcellVals[2] = {-1.4, -1.7};
    tmp = vtkm::cont::make_ArrayHandle(outcellVals, 2);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[5]);

    vtkm::worklet::DispatcherMapCell<CellType> dispatcher;
    dispatcher.Invoke(ds.Fields[4], ds.Fields[3], ds.conn, ds.Fields[5]);

#if 0
    //Add some verts.
    ds.Fields[ds.x_idx].PrepareForOutput(nVerts, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    ds.Fields[ds.y_idx].PrepareForOutput(nVerts, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    ds.Fields[ds.z_idx].PrepareForOutput(nVerts, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    
    //v0 = (0,0,0)
    ds.Fields[ds.x_idx].GetPortalControl().Set(0, 0.0);
    ds.Fields[ds.y_idx].GetPortalControl().Set(0, 0.0);
    ds.Fields[ds.z_idx].GetPortalControl().Set(0, 0.0);

    //v1 = (1,0,0)
    ds.Fields[ds.x_idx].GetPortalControl().Set(1, 1.0);
    ds.Fields[ds.y_idx].GetPortalControl().Set(1, 0.0);
    ds.Fields[ds.z_idx].GetPortalControl().Set(1, 0.0);

    //v2 = (1,1,0)
    ds.Fields[ds.x_idx].GetPortalControl().Set(2, 1.0);
    ds.Fields[ds.y_idx].GetPortalControl().Set(2, 1.0);
    ds.Fields[ds.z_idx].GetPortalControl().Set(2, 0.0);

    //scalar
    ds.Fields[3].PrepareForOutput(nVerts, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    for (int i = 0; i < nVerts; i++)
	ds.Fields[3].GetPortalControl().Set(i, i*10.0);
    
    /*
    ds.Points.PrepareForOutput(nVerts, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    ds.Field.PrepareForOutput(nVerts, vtkm::cont::DeviceAdapterTagSerial());
    vtkm::Vec<vtkm::FloatDefault,3> V0 = vtkm::Vec<vtkm::FloatDefault,3>(0, 0, 0);
    vtkm::Vec<vtkm::FloatDefault,3> V1 = vtkm::Vec<vtkm::FloatDefault,3>(1, 0, 0);
    vtkm::Vec<vtkm::FloatDefault,3> V2 = vtkm::Vec<vtkm::FloatDefault,3>(1, 1, 0);
    
    ds.Points.GetPortalControl().Set(0, V0);
    ds.Points.GetPortalControl().Set(1, V1);
    ds.Points.GetPortalControl().Set(2, V2);

    ds.Field.GetPortalControl().Set(0, vtkm::Vec<vtkm::FloatDefault,1>(10));
    ds.Field.GetPortalControl().Set(1, vtkm::Vec<vtkm::FloatDefault,1>(20));
    ds.Field.GetPortalControl().Set(2, vtkm::Vec<vtkm::FloatDefault,1>(30));
    */
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
    ds.Fields.resize(6);
    ds.Fields[0] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    ds.Fields[1] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    ds.Fields[2] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    ds.Fields[3] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    ds.Fields[4] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    ds.Fields[5] = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();

    const int nVerts = 18;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    vtkm::Float32 vars[nVerts] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180};


    vtkm::cont::ArrayHandle<vtkm::Float32> tmp;
    //Set X, Y, and Z fields.
    tmp = vtkm::cont::make_ArrayHandle(xVals, nVerts);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[ds.x_idx]);
    tmp = vtkm::cont::make_ArrayHandle(yVals, nVerts);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[ds.y_idx]);
    tmp = vtkm::cont::make_ArrayHandle(zVals, nVerts);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[ds.z_idx]);

    //Set node scalar
    tmp = vtkm::cont::make_ArrayHandle(vars, nVerts);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[3]);

    //Set cell scalar
    vtkm::Float32 cellvar[4] = {100.1, 100.2, 100.3, 100.4};
    tmp = vtkm::cont::make_ArrayHandle(cellvar, 4);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[4]);


    //Set regular structure
    ds.reg.SetNodeDimension3D(3,2,3);

    //Run a worklet to populate a cell centered field.
    vtkm::Float32 cellVals[4] = {-1.1, -1.2, -1.3, -1.4};
    tmp = vtkm::cont::make_ArrayHandle(cellVals, 4);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[5]);

    vtkm::worklet::DispatcherMapCell<CellType> dispatcher;
    dispatcher.Invoke(ds.Fields[4], ds.Fields[3], ds.reg, ds.Fields[5]);
}

int UnitTestDataSet(int, char *[])
{
    int err = 0;
    err += vtkm::cont::testing::Testing::Run(TestDataSet_Explicit);
    err += vtkm::cont::testing::Testing::Run(TestDataSet_Regular);
    return err;
}
