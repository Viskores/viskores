//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ErrorUserAbort.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/source/Wavelet.h>

#include <sstream>

namespace
{

// A function that checks for abort request.
// This function will be called by `TryExecute` befaure lauching a device task
// to check if abort has been requested.
// For this test case, we are using a simple logic of returning true for the
// `abortAt`th check.
// If this test is failing, one of the things to check would be to see if the
// `Contour` filter has changed such that it no longer has atleast `abortAt`
// task invocations.
bool ShouldAbort()
{
  static int abortCheckCounter = 0;
  static constexpr int abortAt = 5;
  if (++abortCheckCounter >= abortAt)
  {
    std::cout << "Abort check " << abortCheckCounter << ": true\n";
    return true;
  }

  std::cout << "Abort check " << abortCheckCounter << ": false\n";
  return false;
}

int TestAbort()
{
  vtkm::source::Wavelet wavelet;
  wavelet.SetExtent(vtkm::Id3(-15), vtkm::Id3(16));
  auto input = wavelet.Execute();

  auto range = input.GetField("RTData").GetRange().ReadPortal().Get(0);
  std::vector<vtkm::Float64> isovals;
  static constexpr int numDivs = 5;
  for (int i = 1; i < numDivs - 1; ++i)
  {
    auto v = range.Min +
      (static_cast<vtkm::Float64>(i) *
       ((range.Max - range.Min) / static_cast<vtkm::Float64>(numDivs)));
    isovals.push_back(v);
  }

  vtkm::filter::contour::Contour contour;
  contour.SetActiveField("RTData");
  contour.SetIsoValues(isovals);

  // First we will run the filter with the abort function set
  std::cout << "Run #1 with the abort function set\n";
  try
  {
    vtkm::cont::ScopedRuntimeDeviceTracker tracker(ShouldAbort);

    auto result = contour.Execute(input);

    // execution shouldn't reach here
    VTKM_TEST_FAIL("Error: filter execution was not aborted. Result: ",
                   result.GetNumberOfPoints(),
                   " points and ",
                   result.GetNumberOfCells(),
                   " triangles");
  }
  catch (const vtkm::cont::ErrorUserAbort&)
  {
    std::cout << "Execution was successfully aborted\n";
  }

  // Now run the filter without the abort function
  std::cout << "Run #2 without the abort function set\n";
  try
  {
    auto result = contour.Execute(input);
    std::cout << "Success: filter execution was not aborted. Result: " << result.GetNumberOfPoints()
              << " points and " << result.GetNumberOfCells() << " triangles\n";
  }
  catch (const vtkm::cont::ErrorUserAbort&)
  {
    VTKM_TEST_FAIL("Execution was unexpectedly aborted");
  }

  return 0;
}
} // anon namespace

int UnitTestAbort(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestAbort, argc, argv);
}
