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

void TestDataSet()
{
    vtkm::cont::DataSet ds;
    
    ds.x_idx = 0;
    ds.y_idx = 1;
    ds.z_idx = 2;
    ds.Fields.resize(4);

    int nVerts = 3;
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

}

int UnitTestDataSet(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(TestDataSet);
}
