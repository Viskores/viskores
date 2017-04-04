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

#include <vtkm/worklet/PICS.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

#include <fstream>
#include <vector>
#include <math.h>

int numSeeds = 1000;

template <typename T>
VTKM_EXEC_CONT
vtkm::Vec<T,3> Normalize(vtkm::Vec<T,3> v)
{
  T magnitude = static_cast<T>(sqrt(vtkm::dot(v, v)));
  T zero = static_cast<T>(0.0);
  T one = static_cast<T>(1.0);
  if (magnitude == zero)
    return vtkm::make_Vec(zero, zero, zero);
  else
    return one / magnitude * v;
}

vtkm::cont::DataSet
createDataSet()
{
  //const char *tfile = "/disk2TB/proj/vtkm/pics/vtk-m/data/tornado.vec";
  const char *tfile = "/Users/dpn/proj/vtkm/pics/data/tornado.vec";
  FILE * pFile = fopen(tfile, "rb");
  size_t ret_code = 0;
  int dims[3];
  ret_code = fread(dims, sizeof(int), 3, pFile);
  const vtkm::Id3 vdims(dims[0], dims[1], dims[2]);
  std::cout<<vdims<<std::endl;

  // Read vector data at each point of the uniform grid and store
  vtkm::Id nElements = vdims[0] * vdims[1] * vdims[2] * 3;
  float* data = new float[static_cast<std::size_t>(nElements)];
  ret_code = fread(data, sizeof(float), static_cast<std::size_t>(nElements), pFile);
  fclose(pFile);
  
  std::vector<vtkm::Vec<vtkm::Float32, 3> > *field = new std::vector<vtkm::Vec<vtkm::Float32, 3> >;
  for (vtkm::Id i = 0; i < nElements; i++)
  {
      vtkm::Float32 x = data[i];
      vtkm::Float32 y = data[++i];
      vtkm::Float32 z = data[++i];
      vtkm::Vec<vtkm::Float32, 3> vecData(-x, -y, -z);
      //vtkm::Vec<vtkm::Float32, 3> vecData(x, y, z);
      field->push_back(Normalize(vecData));
  }
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3> > fieldArray;
  fieldArray = vtkm::cont::make_ArrayHandle(*field);

  // Construct the input dataset (uniform) to hold the input and set vector data
  vtkm::cont::DataSet ds;
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims);
  ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));
  ds.AddField(vtkm::cont::Field("vecData", vtkm::cont::Field::ASSOC_POINTS, fieldArray));

  vtkm::cont::CellSetStructured<3> cells("cells");
  cells.SetPointDimensions(vtkm::make_Vec(vdims[0], vdims[1], vdims[2]));
  ds.AddCellSet(cells);
//  vtkm::io::writer::VTKDataSetWriter writer("out.vtk");
//  writer.WriteDataSet(ds);
                          
  return ds;
}

void TestPICSUniformGrid()
{
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;
  
  //std::cout << "Testing PICS uniform grid" << std::endl;

  //Read in data file.
  

  typedef vtkm::Float32 FieldType;
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalConstType;    
  vtkm::cont::DataSet ds = createDataSet();
  //ds.PrintSummary(std::cout);

  vtkm::worklet::RegularGridEvaluate<FieldPortalConstType, DeviceAdapter> eval(ds);
  
  vtkm::Vec<FieldType, 3> p(2,2,2), o;
  bool val = eval.Evaluate(p, o);
  //std::cout<<"EVAL: "<<p<<" --> "<<o<<" : "<<val<<std::endl;

  vtkm::Float32 h = 0.1f;
  typedef vtkm::worklet::RegularGridEvaluate<FieldPortalConstType, DeviceAdapter> RGEvalType;
  typedef vtkm::worklet::RK4Integrator<RGEvalType,FieldType> RK4RGType;

  RK4RGType rk4(eval, h);

  val = rk4.Step(p, o);
  //std::cout<<"RK4: "<<p<<" --> "<<o<<" : "<<val<<std::endl;

  std::vector<vtkm::Vec<FieldType,3> > seeds;
  vtkm::Bounds bounds = ds.GetCoordinateSystem().GetBounds();
  for (int i = 0; i < numSeeds; i++)
  {
      vtkm::Vec<FieldType, 3> p;
      vtkm::Float32 rx = (vtkm::Float32)rand()/(vtkm::Float32)RAND_MAX;
      vtkm::Float32 ry = (vtkm::Float32)rand()/(vtkm::Float32)RAND_MAX;
      vtkm::Float32 rz = (vtkm::Float32)rand()/(vtkm::Float32)RAND_MAX;
      p[0] = static_cast<FieldType>(bounds.X.Min + rx*bounds.X.Length());
      p[1] = static_cast<FieldType>(bounds.Y.Min + ry*bounds.Y.Length());
      p[2] = static_cast<FieldType>(bounds.Z.Min + rz*bounds.Z.Length());
/*
      p[0] = static_cast<FieldType>(15.0f + rx*(35.0f-15.0f));
      p[1] = static_cast<FieldType>(5.0f + ry*(35.0f-5.0f));      
      p[2] = static_cast<FieldType>(25.0f + rz*12.0f);
*/
      seeds.push_back(p);
  }

  vtkm::Id nSteps = 1000;
  vtkm::worklet::PICSFilter<RK4RGType,FieldType,DeviceAdapter> pic(rk4,seeds,nSteps);
  
  pic.run();
}

int UnitTestPICS(int argc, char **argv)
{
    if (argc > 1)
        numSeeds = atoi(argv[1]);
    std::cout<<"Num seeds= "<<numSeeds<<std::endl;
  return vtkm::cont::testing::Testing::Run(TestPICSUniformGrid);
}
