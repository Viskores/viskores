//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_testing_TestingAlgorithms_h
#define vtk_m_testing_TestingAlgorithms_h

#include <vtkm/Algorithms.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/testing/Testing.h>

#include <vector>

namespace
{

using IdArray = vtkm::cont::ArrayHandle<vtkm::Id>;

struct TestBinarySearch
{
  template <typename NeedlesT, typename HayStackT, typename ResultsT>
  struct Impl : public vtkm::exec::FunctorBase
  {
    NeedlesT Needles;
    HayStackT HayStack;
    ResultsT Results;

    VTKM_CONT
    Impl(const NeedlesT& needles, const HayStackT& hayStack, const ResultsT& results)
      : Needles(needles)
      , HayStack(hayStack)
      , Results(results)
    {
    }

    VTKM_EXEC
    void operator()(vtkm::Id index) const
    {
      this->Results.Set(index, vtkm::BinarySearch(this->HayStack, this->Needles.Get(index)));
    }
  };

  template <typename Device>
  static void Run()
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    std::vector<vtkm::Id> needlesData{ -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 };
    std::vector<vtkm::Id> hayStackData{ -3, -2, -2, -2, 0, 0, 1, 1, 1, 4, 4 };
    std::vector<bool> expectedFound{
      false, true, true, false, true, true, false, false, true, false
    };

    IdArray needles = vtkm::cont::make_ArrayHandle(needlesData);
    IdArray hayStack = vtkm::cont::make_ArrayHandle(hayStackData);
    IdArray results;

    using Functor = Impl<typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::Portal>;
    Functor functor{ needles.PrepareForInput(Device{}),
                     hayStack.PrepareForInput(Device{}),
                     results.PrepareForOutput(needles.GetNumberOfValues(), Device{}) };

    Algo::Schedule(functor, needles.GetNumberOfValues());

    // Verify:
    auto needlesPortal = needles.GetPortalConstControl();
    auto hayStackPortal = hayStack.GetPortalConstControl();
    auto resultsPortal = results.GetPortalConstControl();
    for (vtkm::Id i = 0; i < needles.GetNumberOfValues(); ++i)
    {
      if (expectedFound[static_cast<size_t>(i)])
      {
        const auto resIdx = resultsPortal.Get(i);
        const auto expVal = needlesPortal.Get(i);
        VTKM_TEST_ASSERT(resIdx >= 0);
        VTKM_TEST_ASSERT(hayStackPortal.Get(resIdx) == expVal);
      }
      else
      {
        VTKM_TEST_ASSERT(resultsPortal.Get(i) == -1);
      }
    }
  }
};

struct TestLowerBound
{
  template <typename NeedlesT, typename HayStackT, typename ResultsT>
  struct Impl : public vtkm::exec::FunctorBase
  {
    NeedlesT Needles;
    HayStackT HayStack;
    ResultsT Results;

    VTKM_CONT
    Impl(const NeedlesT& needles, const HayStackT& hayStack, const ResultsT& results)
      : Needles(needles)
      , HayStack(hayStack)
      , Results(results)
    {
    }

    VTKM_EXEC
    void operator()(vtkm::Id index) const
    {
      this->Results.Set(index, vtkm::LowerBound(this->HayStack, this->Needles.Get(index)));
    }
  };

  template <typename Device>
  static void Run()
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    std::vector<vtkm::Id> needlesData{ -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 };
    std::vector<vtkm::Id> hayStackData{ -3, -2, -2, -2, 0, 0, 1, 1, 1, 4, 4 };
    std::vector<vtkm::Id> expected{ 0, 0, 1, 4, 4, 6, 9, 9, 9, 11 };

    IdArray needles = vtkm::cont::make_ArrayHandle(needlesData);
    IdArray hayStack = vtkm::cont::make_ArrayHandle(hayStackData);
    IdArray results;

    using Functor = Impl<typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::Portal>;
    Functor functor{ needles.PrepareForInput(Device{}),
                     hayStack.PrepareForInput(Device{}),
                     results.PrepareForOutput(needles.GetNumberOfValues(), Device{}) };

    Algo::Schedule(functor, needles.GetNumberOfValues());

    // Verify:
    auto resultsPortal = results.GetPortalConstControl();
    for (vtkm::Id i = 0; i < needles.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(resultsPortal.Get(i) == expected[static_cast<size_t>(i)]);
    }
  }
};

struct TestUpperBound
{
  template <typename NeedlesT, typename HayStackT, typename ResultsT>
  struct Impl : public vtkm::exec::FunctorBase
  {
    NeedlesT Needles;
    HayStackT HayStack;
    ResultsT Results;

    VTKM_CONT
    Impl(const NeedlesT& needles, const HayStackT& hayStack, const ResultsT& results)
      : Needles(needles)
      , HayStack(hayStack)
      , Results(results)
    {
    }

    VTKM_EXEC
    void operator()(vtkm::Id index) const
    {
      this->Results.Set(index, vtkm::UpperBound(this->HayStack, this->Needles.Get(index)));
    }
  };

  template <typename Device>
  static void Run()
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    std::vector<vtkm::Id> needlesData{ -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 };
    std::vector<vtkm::Id> hayStackData{ -3, -2, -2, -2, 0, 0, 1, 1, 1, 4, 4 };
    std::vector<vtkm::Id> expected{ 0, 1, 4, 4, 6, 9, 9, 9, 11, 11 };

    IdArray needles = vtkm::cont::make_ArrayHandle(needlesData);
    IdArray hayStack = vtkm::cont::make_ArrayHandle(hayStackData);
    IdArray results;

    using Functor = Impl<typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::Portal>;
    Functor functor{ needles.PrepareForInput(Device{}),
                     hayStack.PrepareForInput(Device{}),
                     results.PrepareForOutput(needles.GetNumberOfValues(), Device{}) };

    Algo::Schedule(functor, needles.GetNumberOfValues());

    // Verify:
    auto resultsPortal = results.GetPortalConstControl();
    for (vtkm::Id i = 0; i < needles.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(resultsPortal.Get(i) == expected[static_cast<size_t>(i)]);
    }
  }
};

} // anon namespace

template <typename Device>
void RunAlgorithmsTests()
{
  std::cout << "Testing binary search." << std::endl;
  TestBinarySearch::Run<Device>();
  std::cout << "Testing lower bound." << std::endl;
  TestLowerBound::Run<Device>();
  std::cout << "Testing upper bound." << std::endl;
  TestUpperBound::Run<Device>();
}

#endif //vtk_m_testing_TestingAlgorithms_h
