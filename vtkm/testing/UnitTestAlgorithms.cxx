//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>

VTKM_DEPRECATED_SUPPRESS_BEGIN
#include <vtkm/Algorithms.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vector>

namespace
{

using IdArray = vtkm::cont::ArrayHandle<vtkm::Id>;

struct TestBinarySearch
{
  struct Impl : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn needles, WholeArrayIn haystack, FieldOut results);
    using ExecutionSignature = _3(_1, _2);
    using InputDomain = _1;

    template <typename HaystackPortal>
    VTKM_EXEC vtkm::Id operator()(vtkm::Id needle, const HaystackPortal& haystack) const
    {
      return vtkm::BinarySearch(haystack, needle);
    }
  };

  static void Run()
  {
    IdArray needles = vtkm::cont::make_ArrayHandle<vtkm::Id>({ -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 });
    IdArray haystack =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ -3, -2, -2, -2, 0, 0, 1, 1, 1, 4, 4 });
    IdArray results;

    std::vector<bool> expectedFound{
      false, true, true, false, true, true, false, false, true, false
    };

    vtkm::cont::Invoker invoke;
    invoke(Impl{}, needles, haystack, results);

    // Verify:
    auto needlesPortal = needles.ReadPortal();
    auto haystackPortal = haystack.ReadPortal();
    auto resultsPortal = results.ReadPortal();
    for (vtkm::Id i = 0; i < needles.GetNumberOfValues(); ++i)
    {
      if (expectedFound[static_cast<size_t>(i)])
      {
        const auto resIdx = resultsPortal.Get(i);
        const auto expVal = needlesPortal.Get(i);
        VTKM_TEST_ASSERT(resIdx >= 0);
        VTKM_TEST_ASSERT(haystackPortal.Get(resIdx) == expVal);
      }
      else
      {
        VTKM_TEST_ASSERT(resultsPortal.Get(i) == -1);
      }
    }
  }
};

void RunAlgorithmsTests()
{
  std::cout << "Testing binary search." << std::endl;
  TestBinarySearch::Run();
}

} // anon namespace

VTKM_DEPRECATED_SUPPRESS_END

int UnitTestAlgorithms(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RunAlgorithmsTests, argc, argv);
}
