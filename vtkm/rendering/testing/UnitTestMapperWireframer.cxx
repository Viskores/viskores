//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

vtkm::cont::DataSet Make3DUniformDataSet(vtkm::Id size = 64)
{
  vtkm::Float32 center = static_cast<vtkm::Float32>(-size) / 2.0f;
  vtkm::cont::DataSetBuilderUniform builder;
  vtkm::cont::DataSet dataSet = builder.Create(vtkm::Id3(size, size, size),
                                               vtkm::Vec<vtkm::Float32, 3>(center, center, center),
                                               vtkm::Vec<vtkm::Float32, 3>(1.0f, 1.0f, 1.0f));
  const char* fieldName = "pointvar";
  vtkm::Id numValues = dataSet.GetCoordinateSystem().GetData().GetNumberOfValues();
  vtkm::cont::ArrayHandleCounting<vtkm::Float32> fieldValues(
    0.0f, 10.0f / static_cast<vtkm::Float32>(numValues), numValues);
  vtkm::cont::ArrayHandle<vtkm::Float32> scalarField;
  vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(fieldValues,
                                                                            scalarField);
  vtkm::cont::DataSetFieldAdd().AddPointField(dataSet, fieldName, scalarField);
  return dataSet;
}

void RenderTests()
{
  typedef vtkm::rendering::MapperWireframer M;
  typedef vtkm::rendering::CanvasRayTracer C;
  typedef vtkm::rendering::View3D V3;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::rendering::ColorTable colorTable("thermal");

  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRegularDataSet0(), "pointvar", colorTable, "reg3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "rect3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DExplicitDataSet4(), "pointvar", colorTable, "expl3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    Make3DUniformDataSet(), "pointvar", colorTable, "uniform3D.pnm");
}

} //namespace

int UnitTestMapperWireframer(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RenderTests);
}
