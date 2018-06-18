//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/TestingSerialization.h>

using namespace vtkm::cont::testing::serialization;

namespace
{

struct TestEqualCellSet
{
  template <typename ShapeST, typename CountST, typename ConnectivityST, typename OffsetST>
  void operator()(
    const vtkm::cont::CellSetExplicit<ShapeST, CountST, ConnectivityST, OffsetST>& cs1,
    const vtkm::cont::CellSetExplicit<ShapeST, CountST, ConnectivityST, OffsetST>& cs2) const
  {
    vtkm::TopologyElementTagPoint p2cFrom{};
    vtkm::TopologyElementTagCell p2cTo{};

    VTKM_TEST_ASSERT(cs1.GetName() == cs2.GetName(), "cellset names don't match");
    VTKM_TEST_ASSERT(cs1.GetNumberOfPoints() == cs2.GetNumberOfPoints(),
                     "cellset number of points don't match");
    TestEqualArrayHandle{}(cs1.GetShapesArray(p2cFrom, p2cTo), cs2.GetShapesArray(p2cFrom, p2cTo));
    TestEqualArrayHandle{}(cs1.GetNumIndicesArray(p2cFrom, p2cTo),
                           cs2.GetNumIndicesArray(p2cFrom, p2cTo));
    TestEqualArrayHandle{}(cs1.GetConnectivityArray(p2cFrom, p2cTo),
                           cs2.GetConnectivityArray(p2cFrom, p2cTo));
    TestEqualArrayHandle{}(cs1.GetIndexOffsetArray(p2cFrom, p2cTo),
                           cs2.GetIndexOffsetArray(p2cFrom, p2cTo));
  }

  template <vtkm::IdComponent DIMENSION>
  void operator()(const vtkm::cont::CellSetStructured<DIMENSION>& cs1,
                  const vtkm::cont::CellSetStructured<DIMENSION>& cs2) const
  {
    VTKM_TEST_ASSERT(cs1.GetName() == cs2.GetName(), "cellset names don't match");
    VTKM_TEST_ASSERT(cs1.GetPointDimensions() == cs2.GetPointDimensions(),
                     "CellSetStructured: point dimensions don't match");
  }

  template <typename CellSetTypes>
  void operator()(const vtkm::cont::DynamicCellSetBase<CellSetTypes>& cs1,
                  const vtkm::cont::DynamicCellSetBase<CellSetTypes>& cs2)
  {
    cs1.CastAndCall(*this, cs2);
  }

  template <typename CellSet, typename CellSetTypes>
  void operator()(const CellSet& cs, const vtkm::cont::DynamicCellSetBase<CellSetTypes>& dcs)
  {
    this->operator()(cs, dcs.template Cast<CellSet>());
  }
};

template <typename FieldTypeList, typename FieldStorageList, typename CellSetTypes>
void TestEqualDataSet(
  const vtkm::cont::SerializableDataSet<FieldTypeList, FieldStorageList, CellSetTypes>& s1,
  const vtkm::cont::SerializableDataSet<FieldTypeList, FieldStorageList, CellSetTypes>& s2)
{
  const auto& ds1 = s1.DataSet;
  const auto& ds2 = s2.DataSet;

  VTKM_TEST_ASSERT(ds1.GetNumberOfCoordinateSystems() == ds2.GetNumberOfCoordinateSystems(),
                   "datasets' number of coordinate systems don't match");
  for (vtkm::IdComponent i = 0; i < ds1.GetNumberOfCoordinateSystems(); ++i)
  {
    TestEqualArrayHandle{}(ds1.GetCoordinateSystem(i).GetData(),
                           ds2.GetCoordinateSystem(i).GetData());
  }
  VTKM_TEST_ASSERT(ds1.GetNumberOfCellSets() == ds2.GetNumberOfCellSets(),
                   "datasets' number of cellsets don't match");
  for (vtkm::IdComponent i = 0; i < ds1.GetNumberOfCellSets(); ++i)
  {
    TestEqualCellSet{}(ds1.GetCellSet(i).ResetCellSetList(CellSetTypes{}),
                       ds2.GetCellSet(i).ResetCellSetList(CellSetTypes{}));
  }
  VTKM_TEST_ASSERT(ds1.GetNumberOfFields() == ds2.GetNumberOfFields(),
                   "datasets' number of fields don't match");
  for (vtkm::IdComponent i = 0; i < ds1.GetNumberOfFields(); ++i)
  {
    auto f1 = ds1.GetField(i);
    auto f2 = ds2.GetField(i);
    VTKM_TEST_ASSERT(f1.GetName() == f2.GetName(), "field names don't match");
    VTKM_TEST_ASSERT(f1.GetAssociation() == f1.GetAssociation(), "fields' association don't match");
    if (f1.GetAssociation() == vtkm::cont::Field::Association::CELL_SET)
    {
      VTKM_TEST_ASSERT(f1.GetAssocCellSet() == f2.GetAssocCellSet(),
                       "fields' associated cellset names don't match");
    }
    else if (f1.GetAssociation() == vtkm::cont::Field::Association::LOGICAL_DIM)
    {
      VTKM_TEST_ASSERT(f1.GetAssocLogicalDim() == f2.GetAssocLogicalDim(),
                       "fields' associated logical dims don't match");
    }
    TestEqualArrayHandle{}(
      f1.GetData().ResetTypeAndStorageLists(FieldTypeList{}, FieldStorageList{}),
      f2.GetData().ResetTypeAndStorageLists(FieldTypeList{}, FieldStorageList{}));
  }
}

void RunTest(const vtkm::cont::DataSet& ds)
{
  using TypeList = vtkm::ListTagBase<vtkm::Float32>;
  using StorageList = VTKM_DEFAULT_STORAGE_LIST_TAG;
  using CellSetTypes = vtkm::ListTagBase<vtkm::cont::CellSetExplicit<>,
                                         vtkm::cont::CellSetSingleType<>,
                                         vtkm::cont::CellSetStructured<1>,
                                         vtkm::cont::CellSetStructured<2>,
                                         vtkm::cont::CellSetStructured<3>>;
  TestSerialization(vtkm::cont::SerializableDataSet<TypeList, StorageList, CellSetTypes>(ds),
                    TestEqualDataSet<TypeList, StorageList, CellSetTypes>);
}

void TestDataSetSerialization()
{
  vtkm::cont::testing::MakeTestDataSet makeDS;

  std::cout << "Testing 1D Uniform DataSet #0\n";
  RunTest(makeDS.Make1DUniformDataSet0());
  std::cout << "Testing 1D Uniform DataSet #1\n";
  RunTest(makeDS.Make1DUniformDataSet1());

  std::cout << "Testing 2D Uniform DataSet #0\n";
  RunTest(makeDS.Make2DUniformDataSet0());
  std::cout << "Testing 2D Uniform DataSet #1\n";
  RunTest(makeDS.Make2DUniformDataSet1());

  std::cout << "Testing 3D Uniform DataSet #0\n";
  RunTest(makeDS.Make3DUniformDataSet0());
  std::cout << "Testing 3D Uniform DataSet #1\n";
  RunTest(makeDS.Make3DUniformDataSet1());
  std::cout << "Testing 3D Uniform DataSet #2\n";
  RunTest(makeDS.Make3DUniformDataSet2());

  std::cout << "Testing 3D Regular DataSet #0\n";
  RunTest(makeDS.Make3DRegularDataSet0());
  std::cout << "Testing 3D Regular DataSet #1\n";
  RunTest(makeDS.Make3DRegularDataSet1());

  std::cout << "Testing 2D Rectilinear DataSet #0\n";
  RunTest(makeDS.Make2DRectilinearDataSet0());
  std::cout << "Testing 3D Rectilinear DataSet #0\n";
  RunTest(makeDS.Make3DRectilinearDataSet0());

  std::cout << "Testing 1D Explicit DataSet #0\n";
  RunTest(makeDS.Make1DExplicitDataSet0());

  std::cout << "Testing 2D Explicit DataSet #0\n";
  RunTest(makeDS.Make2DExplicitDataSet0());

  std::cout << "Testing 3D Explicit DataSet #0\n";
  RunTest(makeDS.Make3DExplicitDataSet0());
  std::cout << "Testing 3D Explicit DataSet #1\n";
  RunTest(makeDS.Make3DExplicitDataSet1());
  std::cout << "Testing 3D Explicit DataSet #2\n";
  RunTest(makeDS.Make3DExplicitDataSet2());
  std::cout << "Testing 3D Explicit DataSet #3\n";
  RunTest(makeDS.Make3DExplicitDataSet3());
  std::cout << "Testing 3D Explicit DataSet #4\n";
  RunTest(makeDS.Make3DExplicitDataSet4());
  std::cout << "Testing 3D Explicit DataSet #5\n";
  RunTest(makeDS.Make3DExplicitDataSet5());
  std::cout << "Testing 3D Explicit DataSet #6\n";
  RunTest(makeDS.Make3DExplicitDataSet6());

  std::cout << "Testing 3D Polygonal DataSet #0\n";
  RunTest(makeDS.Make3DExplicitDataSetPolygonal());

  std::cout << "Testing Cow Nose DataSet\n";
  RunTest(makeDS.Make3DExplicitDataSetCowNose());
}

} // anonymous namespace

int UnitTestSerializationDataSet(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestDataSetSerialization);
}
