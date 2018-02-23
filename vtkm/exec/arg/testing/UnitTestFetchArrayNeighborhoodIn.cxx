//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/exec/arg/FetchTagArrayNeighborhoodIn.h>
#include <vtkm/exec/arg/ThreadIndicesPointNeighborhood.h>

#include <vtkm/testing/Testing.h>

namespace
{

static const vtkm::Id3 POINT_DIMS = { 10, 4, 16 };

template <typename T>
struct TestPortal
{
  using ValueType = T;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return POINT_DIMS[0] * POINT_DIMS[1] * POINT_DIMS[2]; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    VTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }
};

struct TestIndexPortal
{
  using ValueType = vtkm::Id;

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return index; }
};

template <typename NeighborhoodType, typename T>
void verify_neighbors(NeighborhoodType neighbors, vtkm::Id index, vtkm::Id3 index3d, T)
{

  T expected;
  auto* boundary = neighbors.Boundary;

  //Verify the boundar flags first
  VTKM_TEST_ASSERT(test_equal(index3d[0] == (POINT_DIMS[0] - 1), boundary->OnXPositive()),
                   "Got invalid X+ boundary");
  VTKM_TEST_ASSERT(test_equal(index3d[0] == 0, boundary->OnXNegative()), "Got invalid X- boundary");
  VTKM_TEST_ASSERT(test_equal(index3d[1] == (POINT_DIMS[1] - 1), boundary->OnYPositive()),
                   "Got invalid Y+ boundary");
  VTKM_TEST_ASSERT(test_equal(index3d[1] == 0, boundary->OnYNegative()), "Got invalid Y- boundary");
  VTKM_TEST_ASSERT(test_equal(index3d[2] == (POINT_DIMS[2] - 1), boundary->OnZPositive()),
                   "Got invalid Z+ boundary");
  VTKM_TEST_ASSERT(test_equal(index3d[2] == 0, boundary->OnZNegative()), "Got invalid Z- boundary");

  T forwardX = neighbors.Get(1, 0, 0);
  expected = boundary->OnXPositive() ? TestValue(index, T()) : TestValue(index + 1, T());
  VTKM_TEST_ASSERT(test_equal(forwardX, expected), "Got invalid value from Load.");

  T backwardsX = neighbors.Get(-1, 0, 0);
  expected = boundary->OnXNegative() ? TestValue(index, T()) : TestValue(index - 1, T());
  VTKM_TEST_ASSERT(test_equal(backwardsX, expected), "Got invalid value from Load.");
}


template <typename T>
struct FetchArrayNeighborhoodInTests
{
  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayNeighborhoodIn<1>,
                                             vtkm::exec::arg::AspectTagDefault,
                                             vtkm::exec::arg::ThreadIndicesPointNeighborhood<1>,
                                             TestPortal<T>>;

    FetchType fetch;



    vtkm::internal::ConnectivityStructuredInternals<3> connectivityInternals;
    connectivityInternals.SetPointDimensions(POINT_DIMS);
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                       vtkm::TopologyElementTagPoint,
                                       3>
      connectivity(connectivityInternals);

    // Verify that 3D scheduling works with neighborhoods
    {
      vtkm::Id3 index3d;
      vtkm::Id index = 0;
      for (vtkm::Id k = 0; k < POINT_DIMS[2]; k++)
      {
        index3d[2] = k;
        for (vtkm::Id j = 0; j < POINT_DIMS[1]; j++)
        {
          index3d[1] = j;
          for (vtkm::Id i = 0; i < POINT_DIMS[0]; i++, index++)
          {
            index3d[0] = i;
            vtkm::exec::arg::ThreadIndicesPointNeighborhood<1> indices(
              index3d, vtkm::internal::NullType(), vtkm::internal::NullType(), connectivity);

            auto neighbors = fetch.Load(indices, execObject);

            T value = neighbors.Get(0, 0, 0);
            VTKM_TEST_ASSERT(test_equal(value, TestValue(index, T())),
                             "Got invalid value from Load.");

            //We now need to check the neighbors.
            verify_neighbors(neighbors, index, index3d, value);

            // This should be a no-op, but we should be able to call it.
            fetch.Store(indices, execObject, neighbors);
          }
        }
      }
    }

    //Verify that 1D scheduling works with neighborhoods
    for (vtkm::Id index = 0; index < (POINT_DIMS[0] * POINT_DIMS[1] * POINT_DIMS[2]); index++)
    {
      vtkm::exec::arg::ThreadIndicesPointNeighborhood<1> indices(
        index, TestIndexPortal(), TestIndexPortal(), connectivity);

      auto neighbors = fetch.Load(indices, execObject);

      T value = neighbors.Get(0, 0, 0); //center value
      VTKM_TEST_ASSERT(test_equal(value, TestValue(index, T())), "Got invalid value from Load.");


      const vtkm::Id indexij = index % (POINT_DIMS[0] * POINT_DIMS[1]);
      vtkm::Id3 index3d(
        indexij % POINT_DIMS[0], indexij / POINT_DIMS[0], index / (POINT_DIMS[0] * POINT_DIMS[1]));

      //We now need to check the neighbors.
      verify_neighbors(neighbors, index, index3d, value);

      // This should be a no-op, but we should be able to call it.
      fetch.Store(indices, execObject, neighbors);
    }
  }
};

struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayNeighborhoodInTests<T>()();
  }
};

void TestExecNeighborhoodFetch()
{
  vtkm::testing::Testing::TryTypes(TryType());
}

} // anonymous namespace

int UnitTestFetchArrayNeighborhoodIn(int, char* [])
{
  return vtkm::testing::Testing::Run(TestExecNeighborhoodFetch);
}
