//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#include <vtkm/testing/Testing.h>

namespace {

static const vtkm::Id ARRAY_SIZE = 10;

template<typename T>
struct TestPortal
{
  typedef T ValueType;

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC_CONT_EXPORT
  ValueType Get(vtkm::Id index) const {
    VTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }
};

struct NullParam {  };

template<vtkm::IdComponent ParamIndex, typename T>
struct FetchArrayDirectInTests
{

  template<typename Invocation>
  void TryInvocation(const Invocation &invocation) const
  {
    typedef vtkm::exec::arg::Fetch<
        vtkm::exec::arg::FetchTagArrayDirectIn,
        vtkm::exec::arg::AspectTagDefault,
        Invocation,
        ParamIndex> FetchType;

    FetchType fetch;

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      T value = fetch.Load(index, invocation);
      VTKM_TEST_ASSERT(test_equal(value, TestValue(index, T())),
                       "Got invalid value from Load.");

      value = T(2)*value;

      // This should be a no-op, but we should be able to call it.
      fetch.Store(index, invocation, value);
    }
  }

  void operator()() const
  {
    std::cout << "Trying ArrayDirectIn fetch on parameter " << ParamIndex
                 << " with type " << vtkm::testing::TypeName<T>::Name()
                 << std::endl;

    typedef vtkm::internal::FunctionInterface<
        void(NullParam,NullParam,NullParam,NullParam,NullParam)>
        BaseFunctionInterface;

    this->TryInvocation(vtkm::internal::make_Invocation<1>(
                          BaseFunctionInterface().Replace<ParamIndex>(
                            TestPortal<T>()),
                          NullParam(),
                          NullParam()));
  }

};

struct TryType
{
  template<typename T>
  void operator()(T) const
  {
    FetchArrayDirectInTests<1,T>()();
    FetchArrayDirectInTests<2,T>()();
    FetchArrayDirectInTests<3,T>()();
    FetchArrayDirectInTests<4,T>()();
    FetchArrayDirectInTests<5,T>()();
  }
};

void TestExecObjectFetch()
{
  vtkm::testing::Testing::TryAllTypes(TryType());
}

} // anonymous namespace

int UnitTestFetchArrayDirectIn(int, char *[])
{
  return vtkm::testing::Testing::Run(TestExecObjectFetch);
}
