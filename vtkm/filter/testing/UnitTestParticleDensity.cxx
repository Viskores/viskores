//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/ParticleDensityNearestGridPoint.h>
#include <vtkm/worklet/DescriptiveStatistics.h>

void TestNGP()
{
  const vtkm::Id N = 1000000;
  // This is a better way to create this array, but I am temporarily breaking this
  // functionality (November 2020) so that I can split up merge requests that move
  // ArrayHandles to the new buffer types. This should be restored once
  // ArrayHandleTransform is converted to the new type.
  auto composite = vtkm::cont::make_ArrayHandleCompositeVector(
    vtkm::cont::ArrayHandleRandomUniformReal<vtkm::Float32>(N, 0xceed),
    vtkm::cont::ArrayHandleRandomUniformReal<vtkm::Float32>(N, 0xdeed),
    vtkm::cont::ArrayHandleRandomUniformReal<vtkm::Float32>(N, 0xabba));
  vtkm::cont::ArrayHandle<vtkm::Vec3f> positions;
  vtkm::cont::ArrayCopy(composite, positions);

  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleIndex(N), connectivity);

  auto dataSet = vtkm::cont::DataSetBuilderExplicit::Create(
    positions, vtkm::CellShapeTagVertex{}, 1, connectivity);

  vtkm::cont::ArrayHandle<vtkm::Float32> mass;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleRandomUniformReal<vtkm::Float32>(N, 0xd1ce), mass);
  dataSet.AddCellField("mass", mass);

  auto cellDims = vtkm::Id3{ 3, 3, 3 };
  vtkm::filter::ParticleDensityNearestGridPoint filter{
    cellDims, { 0.f, 0.f, 0.f }, vtkm::Vec3f{ 1.f / 3.f, 1.f / 3.f, 1.f / 3.f }
  };
  filter.SetActiveField("mass");
  auto density = filter.Execute(dataSet);

  vtkm::cont::ArrayHandle<vtkm::Float32> field;
  density.GetCellField("density").GetData().AsArrayHandle<vtkm::Float32>(field);

  auto mass_result = vtkm::worklet::DescriptiveStatistics::Run(mass);
  auto density_result = vtkm::worklet::DescriptiveStatistics::Run(field);
  // Unfortunately, floating point atomics suffer from precision error more than everything else.
  VTKM_TEST_ASSERT(test_equal(density_result.Sum(), mass_result.Sum() * 27.0, 0.1));
}

void TestParticleDensity()
{
  TestNGP();
}

int UnitTestParticleDensity(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestParticleDensity, argc, argv);
}
