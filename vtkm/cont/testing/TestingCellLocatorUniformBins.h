//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingCellLocatorUniformBins_h
#define vtk_m_cont_testing_TestingCellLocatorUniformBins_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/CellLocatorUniformBins.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/exec/CellInterpolate.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterPermutation.h>
#include <vtkm/worklet/Tetrahedralize.h>
#include <vtkm/worklet/Triangulate.h>
#include <vtkm/worklet/WorkletMapField.h>
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
                                FieldInPoint coords,
                                FieldInOutCell pcs,
                                FieldOutCell wcs);
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

template <vtkm::IdComponent DIMENSIONS>
vtkm::cont::DataSet MakeTestDataSet(const vtkm::Vec<vtkm::Id, DIMENSIONS>& dims)
{
  using Connectivity = vtkm::internal::ConnectivityStructuredInternals<DIMENSIONS>;

  const vtkm::IdComponent PointsPerCell = 1 << DIMENSIONS;

  auto uniformDs =
    vtkm::cont::DataSetBuilderUniform::Create(dims,
                                              vtkm::Vec<vtkm::FloatDefault, DIMENSIONS>(0.0f),
                                              vtkm::Vec<vtkm::FloatDefault, DIMENSIONS>(1.0f));

  // copy points
  vtkm::cont::ArrayHandle<PointType> points;
  vtkm::cont::ArrayCopy(uniformDs.GetCoordinateSystem().GetData(), points);

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
      cellset = vtkm::worklet::Triangulate().Run(uniformCs);
      break;
    case 3:
      cellset = vtkm::worklet::Tetrahedralize().Run(uniformCs);
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

template <vtkm::IdComponent DIMENSIONS>
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

  vtkm::worklet::DispatcherMapTopology<ParametricToWorldCoordinates> dispatcher(
    ParametricToWorldCoordinates::MakeScatter(cellIds));
  dispatcher.Invoke(ds.GetCellSet(), ds.GetCoordinateSystem().GetData(), pcoords, wcoords);
}

class FindCellWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points,
                                ExecObject locator,
                                FieldOut cellIds,
                                FieldOut pcoords);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename LocatorType>
  VTKM_EXEC void operator()(const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                            const LocatorType& locator,
                            vtkm::Id& cellId,
                            vtkm::Vec<vtkm::FloatDefault, 3>& pcoords) const
  {
    locator->FindCell(point, cellId, pcoords, *this);
  }
};

template <vtkm::IdComponent DIMENSIONS>
void TestCellLocator(const vtkm::Vec<vtkm::Id, DIMENSIONS>& dim, vtkm::Id numberOfPoints)
{
  auto ds = MakeTestDataSet(dim);

  std::cout << "Testing " << DIMENSIONS << "D dataset with " << ds.GetCellSet().GetNumberOfCells()
            << " cells\n";

  vtkm::cont::CellLocatorUniformBins locator;
  locator.SetDensityL1(64.0f);
  locator.SetDensityL2(1.0f);
  locator.SetCellSet(ds.GetCellSet());
  locator.SetCoordinates(ds.GetCoordinateSystem());
  locator.Update();

  vtkm::cont::ArrayHandle<vtkm::Id> expCellIds;
  vtkm::cont::ArrayHandle<PointType> expPCoords;
  vtkm::cont::ArrayHandle<PointType> points;
  GenerateRandomInput<DIMENSIONS>(ds, numberOfPoints, expCellIds, expPCoords, points);

  std::cout << "Finding cells for " << numberOfPoints << " points\n";
  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<PointType> pcoords;

  vtkm::worklet::DispatcherMapField<FindCellWorklet> dispatcher;
  dispatcher.Invoke(points, locator, cellIds, pcoords);

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
void TestingCellLocatorUniformBins()
{
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapter());

  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  RandomGenerator.seed(seed);

  TestCellLocator(vtkm::Id3(8), 512);  // 3D dataset
  TestCellLocator(vtkm::Id2(18), 512); // 2D dataset
}

#endif // vtk_m_cont_testing_TestingCellLocatorUniformBins_h
