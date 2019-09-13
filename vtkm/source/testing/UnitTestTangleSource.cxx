//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/source/Tangle.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/Testing.h>

void TangleSourceTest()
{
  vtkm::cont::Timer timer;
  timer.Start();

  vtkm::source::Tangle source(vtkm::Id3{ 20, 20, 20 });
  vtkm::cont::DataSet ds = source.Execute();


  double time = timer.GetElapsedTime();

  std::cout << "Default wavelet took " << time << "s.\n";

  {
    auto coords = ds.GetCoordinateSystem("coordinates");
    auto data = coords.GetData();
    VTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of points.");
  }

  {
    auto cells = ds.GetCellSet();
    VTKM_TEST_ASSERT(test_equal(cells.GetNumberOfCells(), 8000), "Incorrect number of cells.");
  }

  // check the cell scalars
  {
    using ScalarHandleType = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;

    auto field = ds.GetCellField("cellvar");
    auto dynData = field.GetData();
    VTKM_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.Cast<ScalarHandleType>();
    auto data = handle.GetPortalConstControl();

    VTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 8000), "Incorrect number of elements.");

    for (vtkm::Id i = 0; i < 8000; ++i)
    {
      VTKM_TEST_ASSERT(test_equal(data.Get(i), i), "Incorrect scalar value.");
    }
  }

  // Spot check some node scalars
  {
    using ScalarHandleType = vtkm::cont::ArrayHandle<vtkm::Float32>;

    auto field = ds.GetPointField("nodevar");
    auto dynData = field.GetData();
    VTKM_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.Cast<ScalarHandleType>();
    auto data = handle.GetPortalConstControl();

    VTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of scalars.");

    VTKM_TEST_ASSERT(test_equal(data.Get(0), 24.46), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(16), 16.1195), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(21), 20.5988), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(256), 8.58544), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(1024), 1.56976), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(1987), 1.04074), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(2048), 0.95236), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(3110), 6.39556), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(4097), 2.62186), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(6599), 7.79722), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(7999), 7.94986), "Incorrect scalar value.");
  }
}

int UnitTestTangleSource(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TangleSourceTest, argc, argv);
}
