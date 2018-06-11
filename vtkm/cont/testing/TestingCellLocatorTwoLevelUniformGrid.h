//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_testing_TestingCellLocatorTwoLevelUniformGrid_h
#define vtk_m_cont_testing_TestingCellLocatorTwoLevelUniformGrid_h

#include <vtkm/cont/CellLocatorTwoLevelUniformGrid.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/exec/CellInterpolate.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterPermutation.h>
#include <vtkm/worklet/Tetrahedralize.h>
#include <vtkm/worklet/Triangulate.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/CellShape.h>

#include <ctime>
#include <random>

namespace
{

using PointType = vtkm::Vec<vtkm::FloatDefault, 3>;

std::default_random_engine RandomGenerator;

class ParametricToWorldCoordinates : public vtkm::worklet::WorkletMapPointToCell
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint<Vec3> coords,
                                FieldInOutCell<Vec3> pcs,
                                FieldOutCell<Vec3> wcs);
  using ExecutionSignature = void(CellShape, _2, _3, _4);

  using ScatterType = vtkm::worklet::ScatterPermutation<>;

  VTKM_CONT
  static ScatterType MakeScatter(const vtkm::cont::ArrayHandle<vtkm::Id>& cellIds)
  {
    return ScatterType(cellIds);
  }

  template <typename CellShapeTagType, typename PointsVecType>
  VTKM_EXEC void operator()(CellShapeTagType cellShape,
                            PointsVecType points,
                            const PointType& pc,
                            PointType& wc) const
  {
    wc = vtkm::exec::CellInterpolate(points, pc, cellShape, *this);
  }
};

template <vtkm::IdComponent DIMENSIONS, typename DeviceAdapter>
vtkm::cont::DataSet MakeTestDataSet(const vtkm::Vec<vtkm::Id, DIMENSIONS>& dims,
                                    DeviceAdapter device)
{
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
  using Connectivity = vtkm::internal::ConnectivityStructuredInternals<DIMENSIONS>;

  const vtkm::IdComponent PointsPerCell = 1 << DIMENSIONS;

  auto uniformDs =
    vtkm::cont::DataSetBuilderUniform::Create(dims,
                                              vtkm::Vec<vtkm::FloatDefault, DIMENSIONS>(0.0f),
                                              vtkm::Vec<vtkm::FloatDefault, DIMENSIONS>(1.0f));

  // copy points
  vtkm::cont::ArrayHandle<PointType> points;
  Algorithm::Copy(uniformDs.GetCoordinateSystem().GetData(), points);

  vtkm::Id numberOfCells = uniformDs.GetCellSet().GetNumberOfCells();
  vtkm::Id numberOfIndices = numberOfCells * PointsPerCell;

  Connectivity structured;
  structured.SetPointDimensions(dims);

  // copy connectivity
  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  connectivity.Allocate(numberOfIndices);
  for (vtkm::Id i = 0, idx = 0; i < numberOfCells; ++i)
  {
    auto ptids = structured.GetPointsOfCell(i);
    for (vtkm::IdComponent j = 0; j < PointsPerCell; ++j, ++idx)
    {
      connectivity.GetPortalControl().Set(idx, ptids[j]);
    }
  }

  auto uniformCs =
    uniformDs.GetCellSet().template Cast<vtkm::cont::CellSetStructured<DIMENSIONS>>();
  vtkm::cont::CellSetSingleType<> cellset;

  // triangulate the cellset
  switch (DIMENSIONS)
  {
    case 2:
      cellset = vtkm::worklet::Triangulate().Run(uniformCs, device);
      break;
    case 3:
      cellset = vtkm::worklet::Tetrahedralize().Run(uniformCs, device);
      break;
    default:
      VTKM_ASSERT(false);
  }

  // It is possible that the warping will result in invalid cells. So use a
  // local random generator with a known seed that does not create invalid cells.
  std::default_random_engine rgen;

  // Warp the coordinates
  std::uniform_real_distribution<vtkm::FloatDefault> warpFactor(-0.25f, 0.25f);
  auto pointsPortal = points.GetPortalControl();
  for (vtkm::Id i = 0; i < pointsPortal.GetNumberOfValues(); ++i)
  {
    PointType warpVec(0);
    for (vtkm::IdComponent c = 0; c < DIMENSIONS; ++c)
    {
      warpVec[c] = warpFactor(rgen);
    }
    pointsPortal.Set(i, pointsPortal.Get(i) + warpVec);
  }

  // build dataset
  vtkm::cont::DataSet out;
  out.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", points));
  out.AddCellSet(cellset);
  return out;
}

template <vtkm::IdComponent DIMENSIONS, typename DeviceAdapter>
void GenerateRandomInput(const vtkm::cont::DataSet& ds,
                         vtkm::Id count,
                         vtkm::cont::ArrayHandle<vtkm::Id>& cellIds,
                         vtkm::cont::ArrayHandle<PointType>& pcoords,
                         vtkm::cont::ArrayHandle<PointType>& wcoords)
{
  vtkm::Id numberOfCells = ds.GetCellSet().GetNumberOfCells();

  std::uniform_int_distribution<vtkm::Id> cellIdGen(0, numberOfCells - 1);

  cellIds.Allocate(count);
  pcoords.Allocate(count);
  wcoords.Allocate(count);

  for (vtkm::Id i = 0; i < count; ++i)
  {
    cellIds.GetPortalControl().Set(i, cellIdGen(RandomGenerator));

    PointType pc(0.0f);
    vtkm::FloatDefault minPc = 1e-3f;
    vtkm::FloatDefault sum = 0.0f;
    for (vtkm::IdComponent c = 0; c < DIMENSIONS; ++c)
    {
      vtkm::FloatDefault maxPc =
        1.0f - (static_cast<vtkm::FloatDefault>(DIMENSIONS - c) * minPc) - sum;
      std::uniform_real_distribution<vtkm::FloatDefault> pcoordGen(minPc, maxPc);
      pc[c] = pcoordGen(RandomGenerator);
      sum += pc[c];
    }
    pcoords.GetPortalControl().Set(i, pc);
  }

  vtkm::worklet::DispatcherMapTopology<ParametricToWorldCoordinates, DeviceAdapter> dispatcher(
    ParametricToWorldCoordinates::MakeScatter(cellIds));
  dispatcher.Invoke(ds.GetCellSet(), ds.GetCoordinateSystem().GetData(), pcoords, wcoords);
}

template <vtkm::IdComponent DIMENSIONS, typename DeviceAdapter>
void TestCellLocator(const vtkm::Vec<vtkm::Id, DIMENSIONS>& dim,
                     vtkm::Id numberOfPoints,
                     DeviceAdapter device)
{
  auto ds = MakeTestDataSet(dim, device);

  std::cout << "Testing " << DIMENSIONS << "D dataset with " << ds.GetCellSet().GetNumberOfCells()
            << " cells\n";

  vtkm::cont::CellLocatorTwoLevelUniformGrid locator;
  locator.SetDensityL1(64.0f);
  locator.SetDensityL2(1.0f);
  locator.SetCellSet(ds.GetCellSet());
  locator.SetCoordinates(ds.GetCoordinateSystem());
  locator.Build(device);

  vtkm::cont::ArrayHandle<vtkm::Id> expCellIds;
  vtkm::cont::ArrayHandle<PointType> expPCoords;
  vtkm::cont::ArrayHandle<PointType> points;
  GenerateRandomInput<DIMENSIONS, DeviceAdapter>(
    ds, numberOfPoints, expCellIds, expPCoords, points);

  std::cout << "Finding cells for " << numberOfPoints << " points\n";
  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<PointType> pcoords;
  locator.FindCells(points, cellIds, pcoords, device);

  for (vtkm::Id i = 0; i < numberOfPoints; ++i)
  {
    VTKM_TEST_ASSERT(cellIds.GetPortalConstControl().Get(i) ==
                       expCellIds.GetPortalConstControl().Get(i),
                     "Incorrect cell ids");
    VTKM_TEST_ASSERT(test_equal(pcoords.GetPortalConstControl().Get(i),
                                expPCoords.GetPortalConstControl().Get(i),
                                1e-3),
                     "Incorrect parameteric coordinates");
  }
}

} // anonymous

template <typename DeviceAdapter>
void TestingCellLocatorTwoLevelUniformGrid()
{
  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  RandomGenerator.seed(seed);

  TestCellLocator(vtkm::Id3(8), 512, DeviceAdapter());  // 3D dataset
  TestCellLocator(vtkm::Id2(18), 512, DeviceAdapter()); // 2D dataset
}

#endif // vtk_m_cont_testing_TestingCellLocatorTwoLevelUniformGrid_h
