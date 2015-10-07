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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>

#include <vtkm/exec/arg/ThreadIndicesBasic.h>

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#include <vtkm/testing/Testing.h>

namespace {

static const vtkm::Id ARRAY_SIZE = 10;

static vtkm::Id g_NumSets;

template<typename T>
struct TestPortal
{
  typedef T ValueType;

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC_CONT_EXPORT
  void Set(vtkm::Id index, const ValueType &value) const {
    VTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    VTKM_TEST_ASSERT(test_equal(value, TestValue(index, ValueType())),
                     "Tried to set invalid value.");
    g_NumSets++;
  }
};

struct NullParam {  };

template<vtkm::IdComponent ParamIndex, typename T>
struct FetchArrayDirectOutTests
{

  template<typename Invocation>
  void TryInvocation(const Invocation &invocation) const
  {
    typedef vtkm::exec::arg::Fetch<
        vtkm::exec::arg::FetchTagArrayDirectOut,
        vtkm::exec::arg::AspectTagDefault,
        vtkm::exec::arg::ThreadIndicesBasic,
        TestPortal<T> > FetchType;

    FetchType fetch;

    g_NumSets = 0;

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      vtkm::exec::arg::ThreadIndicesBasic indices(index, invocation);

      // This is a no-op, but should be callable.
      T value = fetch.Load(
            indices, invocation.Parameters.template GetParameter<ParamIndex>());

      value = TestValue(index, T());

      // The portal will check to make sure we are setting a good value.
      fetch.Store(
            indices,
            invocation.Parameters.template GetParameter<ParamIndex>(),
            value);
    }

    VTKM_TEST_ASSERT(g_NumSets == ARRAY_SIZE,
                     "Array portal's set not called correct number of times."
                     "Store method must be wrong.");
  }

  void operator()() const
  {
    std::cout << "Trying ArrayDirectOut fetch on parameter " << ParamIndex
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
    FetchArrayDirectOutTests<1,T>()();
    FetchArrayDirectOutTests<2,T>()();
    FetchArrayDirectOutTests<3,T>()();
    FetchArrayDirectOutTests<4,T>()();
    FetchArrayDirectOutTests<5,T>()();
  }
};

void TestExecObjectFetch()
{
  vtkm::testing::Testing::TryAllTypes(TryType());
}

} // anonymous namespace

int UnitTestFetchArrayDirectOut(int, char *[])
{
  return vtkm::testing::Testing::Run(TestExecObjectFetch);
}
