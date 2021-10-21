//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/source/Wavelet.h>

#include <vtkm/cont/testing/Testing.h>

void WaveletSourceTest()
{
  vtkm::source::Wavelet source;
  vtkm::cont::DataSet ds = source.Execute();

  {
    auto coords = ds.GetCoordinateSystem("coordinates");
    auto data = coords.GetData();
    VTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of points.");
  }

  {
    auto cells = ds.GetCellSet();
    VTKM_TEST_ASSERT(test_equal(cells.GetNumberOfCells(), 8000), "Incorrect number of cells.");
  }

  // Spot check some scalars
  {
    using ScalarHandleType = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;

    auto field = ds.GetPointField("RTData");
    auto dynData = field.GetData();
    VTKM_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.AsArrayHandle<ScalarHandleType>();
    auto data = handle.ReadPortal();

    VTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of scalars.");

    VTKM_TEST_ASSERT(test_equal(data.Get(0), 60.7635), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(16), 99.6115), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(21), 69.1968), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(256), 118.620), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(1024), 140.466), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(1987), 203.720), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(2048), 223.010), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(3110), 128.282), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(4097), 153.913), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(6599), 120.068), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(7999), 65.6710), "Incorrect scalar value.");
  }
}

int UnitTestWaveletSource(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(WaveletSourceTest, argc, argv);
}
