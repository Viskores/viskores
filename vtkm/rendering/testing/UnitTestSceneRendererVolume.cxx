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
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/rendering/SceneRendererVolume.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>
namespace {

void TestSceneRendererVolume()
{
  
  // test regular grid data set 
  {  
     vtkm::cont::testing::MakeTestDataSet maker;
     vtkm::cont::DataSet regularGrid = maker.Make3DRegularDataSet0();
     regularGrid.PrintSummary(std::cout);
     vtkm::cont::Field scalarField = regularGrid.GetField("cellvar");
     const vtkm::cont::CoordinateSystem coords = regularGrid.GetCoordinateSystem();
    
     vtkm::rendering::SceneRendererVolume<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> sceneRenderer;
     vtkm::rendering::View3D &view = sceneRenderer.GetView();

     vtkm::Float64 coordsBounds[6]; // Xmin,Xmax,Ymin..
     coords.GetBounds(coordsBounds,VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
     //set up a default view  
     vtkm::Vec<vtkm::Float32,3> totalExtent; 
     totalExtent[0] = vtkm::Float32(coordsBounds[1] - coordsBounds[0]); 
     totalExtent[1] = vtkm::Float32(coordsBounds[3] - coordsBounds[2]);
     totalExtent[2] = vtkm::Float32(coordsBounds[5] - coordsBounds[4]);
     vtkm::Float32 mag = vtkm::Magnitude(totalExtent);
     std::cout<<"Magnitude "<<mag<<std::endl;
     vtkm::Normalize(totalExtent);
     vtkm::Vec<vtkm::Float32,3> lookAt = totalExtent * (mag * .5f);
     view.LookAt = totalExtent * (mag * .5f);
     vtkm::Vec<vtkm::Float32,3> up;
     up[0] = 0.f;
     up[1] = 1.f; 
     up[2] = 0.f;
     view.Up = up;
     view.NearPlane = 1.f;
     view.FarPlane = 100.f;
     view.FieldOfView = 60.f;
     view.Height = 500;
     view.Width = 500;
     view.Position = totalExtent * (mag * 2.f);

     vtkm::Float64 scalarBounds[2];
     scalarField.GetBounds(scalarBounds,
                           VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
     vtkm::rendering::ColorTable colorTable("thermal");
     sceneRenderer.SetActiveColorTable(colorTable);
     sceneRenderer.RenderCells(regularGrid.GetCellSet(), coords, scalarField,
                               colorTable, scalarBounds);
  } 

}//TestMortonCodes

} //namespace
int UnitTestSceneRendererVolume(int, char *[])
{
      return vtkm::cont::testing::Testing::Run(TestSceneRendererVolume);
}
