//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>

#include <vtkm/exec/arg/ThreadIndicesBasic3D.h>

#include <vtkm/testing/Testing.h>

namespace
{

static constexpr vtkm::Id3 ARRAY_SIZE = { 10, 10, 3 };

template <typename T>
struct TestPortal
{
  using ValueType = T;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return vtkm::ReduceProduct(ARRAY_SIZE); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id3 index) const
  {
    VTKM_TEST_ASSERT(index[0] >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index[1] >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index[2] >= 0, "Bad portal index.");

    VTKM_TEST_ASSERT(index[0] < ARRAY_SIZE[0], "Bad portal index.");
    VTKM_TEST_ASSERT(index[1] < ARRAY_SIZE[1], "Bad portal index.");
    VTKM_TEST_ASSERT(index[2] < ARRAY_SIZE[2], "Bad portal index.");

    auto flatIndex = index[0] + ARRAY_SIZE[0] * (index[1] + ARRAY_SIZE[1] * index[2]);
    return TestValue(flatIndex, ValueType());
  }
};
}

namespace vtkm
{
namespace exec
{
namespace arg
{
// Fetch for ArrayPortalTex3D when being used for Loads
template <typename T>
struct Fetch<vtkm::exec::arg::FetchTagArrayDirectIn,
             vtkm::exec::arg::AspectTagDefault,
             TestPortal<T>>
{
  using ValueType = T;
  using PortalType = const TestPortal<T>&;

  template <typename ThreadIndicesType>
  VTKM_EXEC ValueType Load(const ThreadIndicesType& indices, PortalType field) const
  {
    return field.Get(indices.GetInputIndex3D());
  }

  template <typename ThreadIndicesType>
  VTKM_EXEC void Store(const ThreadIndicesType&, PortalType, ValueType) const
  {
  }
};
}
}
}

namespace
{

template <typename T>
struct FetchArrayDirectIn3DTests
{
  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayDirectIn,
                                             vtkm::exec::arg::AspectTagDefault,
                                             TestPortal<T>>;

    FetchType fetch;

    vtkm::Id index1d = 0;
    vtkm::Id3 index3d = { 0, 0, 0 };
    for (vtkm::Id k = 0; k < ARRAY_SIZE[2]; ++k)
    {
      index3d[2] = k;
      for (vtkm::Id j = 0; j < ARRAY_SIZE[1]; ++j)
      {
        index3d[1] = j;
        for (vtkm::Id i = 0; i < ARRAY_SIZE[0]; i++, index1d++)
        {
          index3d[0] = i;
          vtkm::exec::arg::ThreadIndicesBasic3D indices(index3d, index1d, index1d, 0, index1d);
          T value = fetch.Load(indices, execObject);
          VTKM_TEST_ASSERT(test_equal(value, TestValue(index1d, T())),
                           "Got invalid value from Load.");

          value = T(T(2) * value);

          // This should be a no-op, but we should be able to call it.
          fetch.Store(indices, execObject, value);
        }
      }
    }
  }
};

struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayDirectIn3DTests<T>()();
  }
};

void TestExecObjectFetch3D()
{
  vtkm::testing::Testing::TryTypes(TryType());
}

} // anonymous namespace

int UnitTestFetchArrayDirectIn3d(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestExecObjectFetch3D, argc, argv);
}
