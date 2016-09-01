//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/Bounds.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasOSMesa.h>
#include <vtkm/rendering/MapperGL.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View2D.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace {
    
void RenderTests()
{
    typedef vtkm::rendering::MapperGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> M;
    typedef vtkm::rendering::CanvasOSMesa C;
    typedef vtkm::rendering::View3D V3;
    typedef vtkm::rendering::View2D V2;

    vtkm::cont::testing::MakeTestDataSet maker;
    vtkm::rendering::ColorTable colorTable("thermal");    
    
    vtkm::rendering::testing::Render<M,C,V3>(maker.Make3DRegularDataSet0(),
                                             "pointvar", colorTable, "reg3D.pnm");
    vtkm::rendering::testing::Render<M,C,V3>(maker.Make3DRectilinearDataSet0(),
                                             "pointvar", colorTable, "rect3D.pnm");
    vtkm::rendering::testing::Render<M,C,V3>(maker.Make3DExplicitDataSet4(),
                                             "pointvar", colorTable, "expl3D.pnm");
    vtkm::rendering::testing::Render<M,C,V2>(maker.Make2DRectilinearDataSet0(),
                                             "pointvar", colorTable, "rect2D.pnm");
}  

} //namespace

int UnitTestMapperOSMesa(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(RenderTests);
}
