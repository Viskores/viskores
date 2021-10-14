//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/source/Oscillator.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/Testing.h>

void OscillatorSourceTest()
{
  vtkm::cont::Timer timer;
  timer.Start();

  vtkm::source::Oscillator source(vtkm::Id3{ 20, 20, 20 });
  source.SetTime(0.5);
  source.AddDamped(0.25f, 0.25f, 0.25f, 0.5f, 0.1f, 0.2f);
  source.AddDecaying(0.5f, 0.5f, 0.5f, 0.35f, 0.2f, 0.1f);
  source.AddPeriodic(0.6f, 0.2f, 0.7f, 0.15f, 0.1f, 0.2f);

  vtkm::cont::DataSet ds = source.Execute();

  double time = timer.GetElapsedTime();

  std::cout << "Default oscillator took " << time << "s.\n";

  {
    auto coords = ds.GetCoordinateSystem("coordinates");
    auto data = coords.GetData();
    VTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of points.");
  }

  {
    auto cells = ds.GetCellSet();
    VTKM_TEST_ASSERT(test_equal(cells.GetNumberOfCells(), 8000), "Incorrect number of cells.");
  }

  // Spot check some node scalars
  {
    using ScalarHandleType = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;

    auto field = ds.GetPointField("oscillating");
    auto dynData = field.GetData();
    VTKM_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.AsArrayHandle<ScalarHandleType>();
    auto data = handle.ReadPortal();

    VTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of scalars.");

    VTKM_TEST_ASSERT(test_equal(data.Get(0), -0.0163996), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(16), -0.0182232), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(21), -0.0181952), "Incorrect scalar value.");
    VTKM_TEST_ASSERT(test_equal(data.Get(3110), -0.0404135), "Incorrect scalar value.");
  }
}

int UnitTestOscillatorSource(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(OscillatorSourceTest, argc, argv);
}
