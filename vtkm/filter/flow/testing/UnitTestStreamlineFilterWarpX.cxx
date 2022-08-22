//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Particle.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/flow/Streamline.h>
#include <vtkm/io/VTKDataSetReader.h>

namespace
{

void GenerateChargedParticles(const vtkm::cont::ArrayHandle<vtkm::Vec3f>& pos,
                              const vtkm::cont::ArrayHandle<vtkm::Vec3f>& mom,
                              const vtkm::cont::ArrayHandle<vtkm::Float64>& mass,
                              const vtkm::cont::ArrayHandle<vtkm::Float64>& charge,
                              const vtkm::cont::ArrayHandle<vtkm::Float64>& weight,
                              vtkm::cont::ArrayHandle<vtkm::ChargedParticle>& seeds)
{
  auto pPortal = pos.ReadPortal();
  auto uPortal = mom.ReadPortal();
  auto mPortal = mass.ReadPortal();
  auto qPortal = charge.ReadPortal();
  auto wPortal = weight.ReadPortal();

  auto numValues = pos.GetNumberOfValues();

  seeds.Allocate(numValues);
  auto sPortal = seeds.WritePortal();

  for (vtkm::Id i = 0; i < numValues; i++)
  {
    vtkm::ChargedParticle electron(
      pPortal.Get(i), i, mPortal.Get(i), qPortal.Get(i), wPortal.Get(i), uPortal.Get(i));
    sPortal.Set(i, electron);
  }
}

void TestStreamlineFilters()
{
  std::string particleFile = vtkm::cont::testing::Testing::DataPath("misc/warpXparticles.vtk");
  std::string fieldFile = vtkm::cont::testing::Testing::DataPath("misc/warpXfields.vtk");

  using SeedsType = vtkm::cont::ArrayHandle<vtkm::ChargedParticle>;

  SeedsType seeds;
  vtkm::io::VTKDataSetReader seedsReader(particleFile);
  vtkm::cont::DataSet seedsData = seedsReader.ReadDataSet();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> pos, mom;
  vtkm::cont::ArrayHandle<vtkm::Float64> mass, charge, w;

  seedsData.GetCoordinateSystem().GetDataAsDefaultFloat().AsArrayHandle(pos);
  seedsData.GetField("Momentum").GetDataAsDefaultFloat().AsArrayHandle(mom);
  seedsData.GetField("Mass").GetData().AsArrayHandle(mass);
  seedsData.GetField("Charge").GetData().AsArrayHandle(charge);
  seedsData.GetField("Weighting").GetData().AsArrayHandle(w);

  GenerateChargedParticles(pos, mom, mass, charge, w, seeds);

  vtkm::io::VTKDataSetReader dataReader(fieldFile);
  vtkm::cont::DataSet dataset = dataReader.ReadDataSet();
  vtkm::cont::UnknownCellSet cells = dataset.GetCellSet();
  vtkm::cont::CoordinateSystem coords = dataset.GetCoordinateSystem();

  auto bounds = coords.GetBounds();
  std::cout << "Bounds : " << bounds << std::endl;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;
  Structured3DType castedCells;
  cells.AsCellSet(castedCells);
  auto dims = castedCells.GetSchedulingRange(vtkm::TopologyElementTagPoint());
  vtkm::Vec3f spacing = { static_cast<vtkm::FloatDefault>(bounds.X.Length()) / (dims[0] - 1),
                          static_cast<vtkm::FloatDefault>(bounds.Y.Length()) / (dims[1] - 1),
                          static_cast<vtkm::FloatDefault>(bounds.Z.Length()) / (dims[2] - 1) };
  std::cout << spacing << std::endl;
  constexpr static vtkm::FloatDefault SPEED_OF_LIGHT =
    static_cast<vtkm::FloatDefault>(2.99792458e8);
  spacing = spacing * spacing;

  vtkm::Id steps = 50;
  vtkm::FloatDefault length = static_cast<vtkm::FloatDefault>(
    1.0 / (SPEED_OF_LIGHT * vtkm::Sqrt(1. / spacing[0] + 1. / spacing[1] + 1. / spacing[2])));
  std::cout << "CFL length : " << length << std::endl;

  vtkm::filter::flow::Streamline streamline;

  streamline.SetStepSize(length);
  streamline.SetNumberOfSteps(steps);
  streamline.SetSeeds(seeds);
  streamline.SetVectorFieldType(vtkm::filter::flow::VectorFieldType::ELECTRO_MAGNETIC_FIELD_TYPE);
  streamline.SetEField("E");
  streamline.SetBField("B");

  auto output = streamline.Execute(dataset);

  VTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");
  VTKM_TEST_ASSERT(output.GetCoordinateSystem().GetNumberOfPoints() == 2550,
                   "Wrong number of coordinates");
  VTKM_TEST_ASSERT(output.GetCellSet().GetNumberOfCells() == 50, "Wrong number of cells");
}
}

int UnitTestStreamlineFilterWarpX(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamlineFilters, argc, argv);
}
