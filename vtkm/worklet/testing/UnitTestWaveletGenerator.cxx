//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/WaveletGenerator.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/Testing.h>

int UnitTestWaveletGenerator(int, char* [])
{
  using Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;

  vtkm::worklet::WaveletGenerator gen;

  vtkm::cont::Timer<Device> timer;
  auto ds = gen.GenerateDataSet<Device>();
  double time = timer.GetElapsedTime();

  std::cout << "Default wavelet took " << time << "s.\n";

  {
    auto coords = ds.GetCoordinateSystem("coords");
    auto data = coords.GetData();
    VTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 8000), "Incorrect number of points.");
  }

  {
    auto cells = ds.GetCellSet(ds.GetCellSetIndex("cells"));
    VTKM_TEST_ASSERT(test_equal(cells.GetNumberOfCells(), 6859), "Incorrect number of cells.");
  }

  // Spot check some scalars
  {
    using ScalarHandleType = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;

    auto field = ds.GetField("scalars", vtkm::cont::Field::Association::POINTS);
    auto dynData = field.GetData();
    VTKM_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.Cast<ScalarHandleType>();
    auto data = handle.GetPortalConstControl();

    VTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 8000), "Incorrect number of scalars.");

    VTKM_TEST_ASSERT(test_equal(data.Get(0), 60.7635), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(16), 99.6115), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(21), 94.8764), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(256), 133.639), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(1024), 123.641), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(1987), 129.683), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(2048), 143.527), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(3110), 203.051), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(4097), 170.763), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(6599), 153.964), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(7999), 54.9307), "Incorrect scalar value.");
  }

  return 0;
}
