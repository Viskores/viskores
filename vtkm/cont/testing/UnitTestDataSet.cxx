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

class CellType : public vtkm::worklet::WorkletMapCell
{
public:
  typedef void ControlSignature(FieldCellIn<IdType> inCells, TopologyIn topology, FieldCellOut<Scalar> outCells);
  typedef _3 ExecutionSignature(_1, vtkm::exec::arg::NodeIdTriplet);
  typedef _2 InputDomain;

  VTKM_CONT_EXPORT
  CellType() { };

  VTKM_EXEC_EXPORT
  vtkm::Float32 operator()(const vtkm::Id &cell, const vtkm::Id3 &nodeIDs) const
  {
      std::cout << "CellType worklet: " << std::endl;
      std::cout << "   -- input field value: " << cell << std::endl;
      std::cout << "   -- input node IDs (not really, it's just workindex+0,10,50 for now): "<<nodeIDs[0]<<","<<nodeIDs[1]<<","<<nodeIDs[2]<<","<<std::endl;
      return (vtkm::Float32)cell;
  }

};


void TestDataSet()
{
    vtkm::cont::DataSet ds;
    
    ds.x_idx = 0;
    ds.y_idx = 1;
    ds.z_idx = 2;
    ds.Fields.resize(5);

    const int nVerts = 4;
    vtkm::Float32 xVals[nVerts] = {0, 1, 1, 0};
    vtkm::Float32 yVals[nVerts] = {0, 0, 1, 1};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0};
    vtkm::Float32 vars[nVerts] = {10, 20, 30, 40};


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


    //Add connectivity
    vtkm::cont::ArrayHandle<vtkm::Id> tmp2;
    std::vector<vtkm::Id> shapes;
    shapes.push_back(1); //Triangle
    shapes.push_back(39); //A Special Triangle

    std::vector<vtkm::Id> conn;
    // First Triangle
    conn.push_back(0);
    conn.push_back(1);
    conn.push_back(2);
    // Second Triangle
    conn.push_back(2);
    conn.push_back(1);
    conn.push_back(3);
    
    tmp2 = vtkm::cont::make_ArrayHandle(shapes);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp2, ds.conn.Shapes);

    tmp2 = vtkm::cont::make_ArrayHandle(conn);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp2, ds.conn.Connectivity);


    //Run a worklet to populate a cell centered field.
    vtkm::Float32 cellVals[2] = {-1.0, -1.0};
    tmp = vtkm::cont::make_ArrayHandle(cellVals, 2);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(tmp, ds.Fields[4]);

    vtkm::worklet::DispatcherMapCell<CellType> dispatcher;
    dispatcher.Invoke(ds.conn.Shapes, ds.conn, ds.Fields[4]);

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

int UnitTestDataSet(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(TestDataSet);
}
