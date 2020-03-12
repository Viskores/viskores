//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/TestingSerialization.h>

using namespace vtkm::cont::testing::serialization;

namespace
{

using FieldTypeList = vtkm::List<vtkm::Float32, vtkm::Vec3f>;
using CellSetTypes = vtkm::List<vtkm::cont::CellSetExplicit<>,
                                vtkm::cont::CellSetSingleType<>,
                                vtkm::cont::CellSetStructured<1>,
                                vtkm::cont::CellSetStructured<2>,
                                vtkm::cont::CellSetStructured<3>>;

using DataSetWrapper = vtkm::cont::SerializableDataSet<FieldTypeList, CellSetTypes>;

VTKM_CONT void TestEqualDataSet(const DataSetWrapper& ds1, const DataSetWrapper& ds2)
{
  auto result = vtkm::cont::testing::test_equal_DataSets(
    ds1.DataSet, ds2.DataSet, CellSetTypes{}, FieldTypeList{});
  VTKM_TEST_ASSERT(result, result.GetMergedMessage());
}

void RunTest(const vtkm::cont::DataSet& ds)
{
  TestSerialization(DataSetWrapper(ds), TestEqualDataSet);
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

int UnitTestSerializationDataSet(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSetSerialization, argc, argv);
}
