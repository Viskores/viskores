//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/ExtractPoints.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

using vtkm::cont::testing::MakeTestDataSet;

class TestingExtractPoints
{
public:
  void TestUniformById() const
  {
    std::cout << "Testing extract points structured by id:" << std::endl;

    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Points to extract
    const int nPoints = 13;
    vtkm::Id pointids[nPoints] = { 0, 1, 2, 3, 4, 5, 10, 15, 20, 25, 50, 75, 100 };
    vtkm::cont::ArrayHandle<vtkm::Id> pointIds = vtkm::cont::make_ArrayHandle(pointids, nPoints);

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted points
    vtkm::worklet::ExtractPoints extractPoints;
    OutCellSetType outCellSet = extractPoints.Run(dataset.GetCellSet(), pointIds);
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nPoints),
                     "Wrong result for ExtractPoints");
  }

  void TestUniformByBox0() const
  {
    std::cout << "Testing extract points with implicit function (box):" << std::endl;

    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    vtkm::Vec3f minPoint(1.f, 1.f, 1.f);
    vtkm::Vec3f maxPoint(3.f, 3.f, 3.f);
    bool extractInside = true;

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted points
    vtkm::worklet::ExtractPoints extractPoints;
    OutCellSetType outCellSet =
      extractPoints.Run(dataset.GetCellSet(),
                        dataset.GetCoordinateSystem("coords"),
                        vtkm::cont::make_ImplicitFunctionHandle<vtkm::Box>(minPoint, maxPoint),
                        extractInside);
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 27),
                     "Wrong result for ExtractPoints");
  }

  void TestUniformByBox1() const
  {
    std::cout << "Testing extract points with implicit function (box):" << std::endl;

    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    vtkm::Vec3f minPoint(1.f, 1.f, 1.f);
    vtkm::Vec3f maxPoint(3.f, 3.f, 3.f);
    bool extractInside = false;

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted points
    vtkm::worklet::ExtractPoints extractPoints;
    OutCellSetType outCellSet =
      extractPoints.Run(dataset.GetCellSet(),
                        dataset.GetCoordinateSystem("coords"),
                        vtkm::cont::make_ImplicitFunctionHandle<vtkm::Box>(minPoint, maxPoint),
                        extractInside);
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 98),
                     "Wrong result for ExtractPoints");
  }

  void TestUniformBySphere() const
  {
    std::cout << "Testing extract points with implicit function (sphere):" << std::endl;

    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    vtkm::Vec3f center(2.f, 2.f, 2.f);
    vtkm::FloatDefault radius(1.8f);
    bool extractInside = true;

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted points
    vtkm::worklet::ExtractPoints extractPoints;
    OutCellSetType outCellSet =
      extractPoints.Run(dataset.GetCellSet(),
                        dataset.GetCoordinateSystem("coords"),
                        vtkm::cont::make_ImplicitFunctionHandle<vtkm::Sphere>(center, radius),
                        extractInside);
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 27),
                     "Wrong result for ExtractPoints");
  }

  void TestExplicitByBox0() const
  {
    std::cout << "Testing extract points with implicit function (box) on explicit:" << std::endl;

    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Implicit function
    vtkm::Vec3f minPoint(0.f, 0.f, 0.f);
    vtkm::Vec3f maxPoint(1.f, 1.f, 1.f);
    bool extractInside = true;

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted points
    vtkm::worklet::ExtractPoints extractPoints;
    OutCellSetType outCellSet =
      extractPoints.Run(dataset.GetCellSet(),
                        dataset.GetCoordinateSystem("coordinates"),
                        vtkm::cont::make_ImplicitFunctionHandle<vtkm::Box>(minPoint, maxPoint),
                        extractInside);
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8),
                     "Wrong result for ExtractPoints");
  }

  void TestExplicitByBox1() const
  {
    std::cout << "Testing extract points with implicit function (box) on explicit:" << std::endl;

    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Implicit function
    vtkm::Vec3f minPoint(0.f, 0.f, 0.f);
    vtkm::Vec3f maxPoint(1.f, 1.f, 1.f);
    bool extractInside = false;

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted points
    vtkm::worklet::ExtractPoints extractPoints;
    OutCellSetType outCellSet =
      extractPoints.Run(dataset.GetCellSet(),
                        dataset.GetCoordinateSystem("coordinates"),
                        vtkm::cont::make_ImplicitFunctionHandle<vtkm::Box>(minPoint, maxPoint),
                        extractInside);
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 3),
                     "Wrong result for ExtractPoints");
  }

  void TestExplicitById() const
  {
    std::cout << "Testing extract points explicit by id:" << std::endl;

    using OutCellSetType = vtkm::cont::CellSetSingleType<>;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Points to extract
    const int nPoints = 6;
    vtkm::Id pointids[nPoints] = { 0, 4, 5, 7, 9, 10 };
    vtkm::cont::ArrayHandle<vtkm::Id> pointIds = vtkm::cont::make_ArrayHandle(pointids, nPoints);

    // Output dataset contains input coordinate system and point data
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted points
    vtkm::worklet::ExtractPoints extractPoints;
    OutCellSetType outCellSet = extractPoints.Run(dataset.GetCellSet(), pointIds);
    outDataSet.SetCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nPoints),
                     "Wrong result for ExtractPoints");
  }

  void operator()() const
  {
    this->TestUniformById();
    this->TestUniformByBox0();
    this->TestUniformByBox1();
    this->TestUniformBySphere();
    this->TestExplicitById();
    this->TestExplicitByBox0();
    this->TestExplicitByBox1();
  }
};

int UnitTestExtractPoints(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingExtractPoints(), argc, argv);
}
