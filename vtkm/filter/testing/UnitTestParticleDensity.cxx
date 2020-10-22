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

  auto cellDims = vtkm::Id3{ 3, 3, 3 };
  vtkm::filter::ParticleDensityNearestGridPoint filter{
    cellDims, { 0.f, 0.f, 0.f }, vtkm::Vec3f{ 1.f / 3.f, 1.f / 3.f, 1.f / 3.f }
  };
  filter.SetUseCoordinateSystemAsField(true);
  auto density = filter.Execute(dataSet);

  vtkm::cont::ArrayHandle<vtkm::Id> field;
  density.GetCellField("density").GetData().AsArrayHandle<vtkm::Id>(field);
  auto field_f = vtkm::cont::make_ArrayHandleCast<vtkm::Float32>(field);

  auto result = vtkm::worklet::DescriptiveStatistics::Run(field_f);
  VTKM_TEST_ASSERT(test_equal(result.Sum(), N));
  VTKM_TEST_ASSERT(test_equal(result.Mean(), N / density.GetNumberOfCells()));
}

void TestParticleDensity()
{
  TestNGP();
}

int UnitTestParticleDensity(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestParticleDensity, argc, argv);
}
