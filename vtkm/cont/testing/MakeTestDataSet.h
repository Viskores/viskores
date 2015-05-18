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

#ifndef vtk_m_cont_testing_MakeTestDataSet_h
#define vtk_m_cont_testing_MakeTestDataSet_h

#include <vtkm/cont/DataSet.h>

namespace vtkm {
namespace cont {
namespace testing {

class MakeTestDataSet
{
public:
    // 2D regular datasets.
    vtkm::cont::DataSet * Make2DRegularDataSet0();

    // 3D regular datasets.
    vtkm::cont::DataSet * Make3DRegularDataSet0();
};


//Make a simple 2D, 2 cell regular dataset.

vtkm::cont::DataSet *
MakeTestDataSet::Make2DRegularDataSet0()
{
    vtkm::cont::DataSet *ds = new vtkm::cont::DataSet;
    ds->x_idx = 0;
    ds->y_idx = 1;
    ds->z_idx = -1;

    const int nVerts = 6;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1};
    vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.1, 40.1, 50.1, 60.1};
    ds->AddFieldViaCopy(xVals, nVerts);
    ds->AddFieldViaCopy(yVals, nVerts);
    
    //set node scalar.
    ds->AddFieldViaCopy(vars, nVerts);

    vtkm::Float32 cellvar[2] = {100.1, 200.1};
    ds->AddFieldViaCopy(cellvar, 2);

    vtkm::cont::CellSetStructured<2> *cs = new vtkm::cont::CellSetStructured<2>("cells");
    //Set regular structure
    cs->structure.SetNodeDimension(3,2);
    ds->AddCellSet(cs);

    return ds;
}

vtkm::cont::DataSet *
MakeTestDataSet::Make3DRegularDataSet0()
{
    vtkm::cont::DataSet *ds = new vtkm::cont::DataSet;

    ds->x_idx = 0;
    ds->y_idx = 1;
    ds->z_idx = 2;

    const int nVerts = 18;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.1, 40.1, 50.2, 60.2, 70.2, 80.2, 90.3,
                                  100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5,
                                  180.5};

    ds->AddFieldViaCopy(xVals, nVerts);
    ds->AddFieldViaCopy(yVals, nVerts);
    ds->AddFieldViaCopy(zVals, nVerts);

    //Set node scalar
    ds->AddFieldViaCopy(vars, nVerts);

    //Set cell scalar
    vtkm::Float32 cellvar[4] = {100.1, 100.2, 100.3, 100.4};
    ds->AddFieldViaCopy(cellvar, 4);
    
    static const vtkm::IdComponent dim = 3;
    vtkm::cont::CellSetStructured<dim> *cs = new vtkm::cont::CellSetStructured<dim>("cells");
    ds->AddCellSet(cs);

    //Set regular structure
    cs->structure.SetNodeDimension(3,2,3);

    //Run a worklet to populate a cell centered field.
    vtkm::Float32 cellVals[4] = {-1.1, -1.2, -1.3, -1.4};
    ds->AddFieldViaCopy(cellVals, 4);

    return ds;
}

}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_MakeTestDataSet_h
