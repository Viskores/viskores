//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/internal/Hints.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/testing/Testing.h>

namespace UnitTestHintNamespace
{

void CheckFind()
{
  std::cout << "Empty list returns default.\n";
  VTKM_TEST_ASSERT(vtkm::cont::internal::HintFind<vtkm::cont::internal::HintList<>,
                                                  vtkm::cont::internal::HintThreadsPerBlock<128>,
                                                  vtkm::cont::DeviceAdapterTagKokkos>::MaxThreads ==
                   128);

  std::cout << "Find a hint that matches.\n";
  VTKM_TEST_ASSERT(vtkm::cont::internal::HintFind<
                     vtkm::cont::internal::HintList<vtkm::cont::internal::HintThreadsPerBlock<128>>,
                     vtkm::cont::internal::HintThreadsPerBlock<0>,
                     vtkm::cont::DeviceAdapterTagKokkos>::MaxThreads == 128);
  VTKM_TEST_ASSERT(
    vtkm::cont::internal::HintFind<
      vtkm::cont::internal::HintList<
        vtkm::cont::internal::HintThreadsPerBlock<128,
                                                  vtkm::List<vtkm::cont::DeviceAdapterTagKokkos>>>,
      vtkm::cont::internal::HintThreadsPerBlock<0>,
      vtkm::cont::DeviceAdapterTagKokkos>::MaxThreads == 128);

  std::cout << "Skip a hint that does not match.\n";
  VTKM_TEST_ASSERT(
    (vtkm::cont::internal::HintFind<
       vtkm::cont::internal::HintList<
         vtkm::cont::internal::HintThreadsPerBlock<128,
                                                   vtkm::List<vtkm::cont::DeviceAdapterTagKokkos>>>,
       vtkm::cont::internal::HintThreadsPerBlock<0>,
       vtkm::cont::DeviceAdapterTagSerial>::MaxThreads == 0));

  std::cout << "Given a list of hints, pick the last one that matches\n";
  {
    using HList = vtkm::cont::internal::HintList<
      vtkm::cont::internal::HintThreadsPerBlock<64>,
      vtkm::cont::internal::HintThreadsPerBlock<128, vtkm::List<vtkm::cont::DeviceAdapterTagCuda>>,
      vtkm::cont::internal::HintThreadsPerBlock<256,
                                                vtkm::List<vtkm::cont::DeviceAdapterTagKokkos>>>;
    using HInit = vtkm::cont::internal::HintThreadsPerBlock<0>;
    VTKM_TEST_ASSERT((vtkm::cont::internal::
                        HintFind<HList, HInit, vtkm::cont::DeviceAdapterTagSerial>::MaxThreads ==
                      64));
    VTKM_TEST_ASSERT(
      (vtkm::cont::internal::HintFind<HList, HInit, vtkm::cont::DeviceAdapterTagCuda>::MaxThreads ==
       128));
    VTKM_TEST_ASSERT((vtkm::cont::internal::
                        HintFind<HList, HInit, vtkm::cont::DeviceAdapterTagKokkos>::MaxThreads ==
                      256));
  }
}

struct MyFunctor : vtkm::exec::FunctorBase
{
  VTKM_EXEC void operator()(vtkm::Id vtkmNotUsed(index)) const
  {
    // NOP
  }

  VTKM_EXEC void operator()(vtkm::Id3 vtkmNotUsed(index)) const
  {
    // NOP
  }
};

void CheckSchedule()
{
  std::cout << "Schedule a functor using hints.\n";
  // There is no good way to see if the device adapter got or used the hints
  // as device adapters are free to ignore hints. This just tests that the
  // hints can be passed.
  using Hints = vtkm::cont::internal::HintList<vtkm::cont::internal::HintThreadsPerBlock<128>>;
  vtkm::cont::Algorithm::Schedule(Hints{}, MyFunctor{}, 10);
  vtkm::cont::Algorithm::Schedule(Hints{}, MyFunctor{}, vtkm::Id3{ 2 });
}

void Run()
{
  CheckFind();
  CheckSchedule();
}

} // anonymous UnitTestHintNamespace

int UnitTestHints(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(UnitTestHintNamespace::Run, argc, argv);
}
