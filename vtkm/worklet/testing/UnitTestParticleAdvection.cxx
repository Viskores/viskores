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

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/ParticleAdvectionFilters.h>

#include <fstream>
#include <vector>
#include <math.h>

#include <chrono>

int numSeeds = 1000;
vtkm::Id numSteps = 1000;
std::string tornadoFile;

vtkm::cont::DataSet
createDataSet(const std::string &fileName)
{
  FILE * pFile = fopen(fileName.c_str(), "rb");
  size_t ret_code = 0;
  int dims[3];
  ret_code = fread(dims, sizeof(int), 3, pFile);
  const vtkm::Id3 vdims(dims[0], dims[1], dims[2]);
  //std::cout<<vdims<<std::endl;

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
      vtkm::Normalize(vecData);
      field->push_back(vecData);
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

void TestPICSAnalyticalOrbit()
{
#if 0
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;
  typedef vtkm::Float32 FieldType;
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalConstType;
  typedef vtkm::worklet::AnalyticalOrbitEvaluate<FieldPortalConstType, DeviceAdapter> OrbitEvalType;

  vtkm::Bounds bounds{ -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f };
  OrbitEvalType orbit_eval{ bounds };

  typedef vtkm::worklet::RK4Integrator<OrbitEvalType, FieldType> IntegratorType;
  vtkm::Float32 step_size = 0.01f;
  IntegratorType rk4(orbit_eval, step_size);

  std::vector<vtkm::Vec<FieldType, 3> > seeds;
  vtkm::Float32 expected_radius = 0.4f;
  seeds.push_back({expected_radius, 0.0f, 0.0f});
  vtkm::Id num_steps_expected = 20;
  typedef vtkm::worklet::PICSFilter<IntegratorType, FieldType, DeviceAdapter> TracerType;
  TracerType tracer(rk4, seeds, num_steps_expected);
  tracer.run();

  auto result_traces = tracer.GetRecorder();
  VTKM_TEST_ASSERT(result_traces != nullptr, "PICS did not generate traces");

  vtkm::Id num_steps_taken = result_traces->GetStep(0);
  VTKM_TEST_ASSERT(test_equal(num_steps_expected, num_steps_taken), "PICS calculated wrong number of steps");

  auto last_point = result_traces->GetHistory(0, num_steps_taken-1);
  vtkm::Float32 last_point_radius = vtkm::Magnitude(last_point);
  VTKM_TEST_ASSERT(test_equal(expected_radius, last_point_radius), "PICS integrated point off orbit");
#endif
}


void TestParticleAdvectionUniformGrid()
{
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  typedef vtkm::Float32 FieldType;
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalConstType;    
  vtkm::cont::DataSet ds = createDataSet(tornadoFile);

  vtkm::worklet::particleadvection::RegularGridEvaluate<FieldPortalConstType, DeviceAdapter> eval(ds);

  vtkm::Float32 h = 0.1f;
  typedef vtkm::worklet::particleadvection::RegularGridEvaluate<FieldPortalConstType, DeviceAdapter> RGEvalType;
  typedef vtkm::worklet::particleadvection::RK4Integrator<RGEvalType,FieldType,FieldPortalConstType> RK4RGType;
  
  RK4RGType rk4(eval, h);

  std::vector<vtkm::Vec<FieldType,3> > seeds;
  vtkm::Bounds bounds = ds.GetCoordinateSystem().GetBounds();
  srand(314);
  for (int i = 0; i < numSeeds; i++)
  {
      vtkm::Vec<FieldType, 3> p;
      vtkm::Float32 rx = (vtkm::Float32)rand()/(vtkm::Float32)RAND_MAX;
      vtkm::Float32 ry = (vtkm::Float32)rand()/(vtkm::Float32)RAND_MAX;
      vtkm::Float32 rz = (vtkm::Float32)rand()/(vtkm::Float32)RAND_MAX;
      p[0] = static_cast<FieldType>(bounds.X.Min + rx*bounds.X.Length());
      p[1] = static_cast<FieldType>(bounds.Y.Min + ry*bounds.Y.Length());
      p[2] = static_cast<FieldType>(bounds.Z.Min + rz*bounds.Z.Length());
      seeds.push_back(p);
  }

  vtkm::worklet::particleadvection::ParticleAdvectionFilter<RK4RGType,
                                                            FieldType,
                                                            DeviceAdapter> pa(rk4,seeds,ds,numSteps);

  pa.run();
}

int UnitTestParticleAdvection(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr<<"Error: Usage "<<argv[0]<<" numSeeds path/tornado.vec"<<std::endl;
        return -1;
    }
    numSeeds = atoi(argv[1]);
    tornadoFile = argv[2];
    
    std::cerr<<"Num seeds= "<<numSeeds<<std::endl;
    std::cerr<<"Data file= "<<tornadoFile<<std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    auto test_result = vtkm::cont::testing::Testing::Run(TestParticleAdvectionUniformGrid);
    auto duration_taken = std::chrono::high_resolution_clock::now() - start;
    std::uint64_t runtime = std::chrono::duration_cast<std::chrono::milliseconds>(duration_taken).count();
    std::cerr << "Runtime = " << runtime << " ms" << std::endl;

    //test_result = vtkm::cont::testing::Testing::Run(TestPICSAnalyticalOrbit);

    return test_result;
}
