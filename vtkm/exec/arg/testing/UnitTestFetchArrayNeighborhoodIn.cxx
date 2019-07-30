//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

template <typename NeighborhoodType, typename T>
void verify_neighbors(NeighborhoodType neighbors, vtkm::Id index, vtkm::Id3 index3d, T)
{

  T expected;
  auto* boundary = neighbors.Boundary;

  //Verify the boundary flags first
  VTKM_TEST_ASSERT(((index3d[0] != 0) && (index3d[0] != (POINT_DIMS[0] - 1))) ==
                     boundary->IsRadiusInXBoundary(1),
                   "Got invalid X radius boundary");
  VTKM_TEST_ASSERT(((index3d[1] != 0) && (index3d[1] != (POINT_DIMS[1] - 1))) ==
                     boundary->IsRadiusInYBoundary(1),
                   "Got invalid Y radius boundary");
  VTKM_TEST_ASSERT(((index3d[2] != 0) && (index3d[2] != (POINT_DIMS[2] - 1))) ==
                     boundary->IsRadiusInZBoundary(1),
                   "Got invalid Z radius boundary");

  VTKM_TEST_ASSERT((index3d[0] != 0) == boundary->IsNeighborInXBoundary(-1),
                   "Got invalid X negative neighbor boundary");
  VTKM_TEST_ASSERT((index3d[1] != 0) == boundary->IsNeighborInYBoundary(-1),
                   "Got invalid Y negative neighbor boundary");
  VTKM_TEST_ASSERT((index3d[2] != 0) == boundary->IsNeighborInZBoundary(-1),
                   "Got invalid Z negative neighbor boundary");

  VTKM_TEST_ASSERT((index3d[0] != (POINT_DIMS[0] - 1)) == boundary->IsNeighborInXBoundary(1),
                   "Got invalid X positive neighbor boundary");
  VTKM_TEST_ASSERT((index3d[1] != (POINT_DIMS[1] - 1)) == boundary->IsNeighborInYBoundary(1),
                   "Got invalid Y positive neighbor boundary");
  VTKM_TEST_ASSERT((index3d[2] != (POINT_DIMS[2] - 1)) == boundary->IsNeighborInZBoundary(1),
                   "Got invalid Z positive neighbor boundary");

  VTKM_TEST_ASSERT(((boundary->MinNeighborIndices(1)[0] == -1) &&
                    (boundary->MaxNeighborIndices(1)[0] == 1)) == boundary->IsRadiusInXBoundary(1),
                   "Got invalid min/max X indices");
  VTKM_TEST_ASSERT(((boundary->MinNeighborIndices(1)[1] == -1) &&
                    (boundary->MaxNeighborIndices(1)[1] == 1)) == boundary->IsRadiusInYBoundary(1),
                   "Got invalid min/max Y indices");
  VTKM_TEST_ASSERT(((boundary->MinNeighborIndices(1)[2] == -1) &&
                    (boundary->MaxNeighborIndices(1)[2] == 1)) == boundary->IsRadiusInZBoundary(1),
                   "Got invalid min/max Z indices");

  T forwardX = neighbors.Get(1, 0, 0);
  expected = (index3d[0] == POINT_DIMS[0] - 1) ? TestValue(index, T()) : TestValue(index + 1, T());
  VTKM_TEST_ASSERT(test_equal(forwardX, expected), "Got invalid value from Load.");

  T backwardsX = neighbors.Get(-1, 0, 0);
  expected = (index3d[0] == 0) ? TestValue(index, T()) : TestValue(index - 1, T());
  VTKM_TEST_ASSERT(test_equal(backwardsX, expected), "Got invalid value from Load.");
}


template <typename T>
struct FetchArrayNeighborhoodInTests
{
  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayNeighborhoodIn,
                                             vtkm::exec::arg::AspectTagDefault,
                                             vtkm::exec::arg::ThreadIndicesPointNeighborhood,
                                             TestPortal<T>>;

    FetchType fetch;



    vtkm::internal::ConnectivityStructuredInternals<3> connectivityInternals;
    connectivityInternals.SetPointDimensions(POINT_DIMS);
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                       vtkm::TopologyElementTagCell,
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
            vtkm::exec::arg::ThreadIndicesPointNeighborhood indices(index3d, connectivity);

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
      vtkm::exec::arg::ThreadIndicesPointNeighborhood indices(index, index, 0, index, connectivity);

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

int UnitTestFetchArrayNeighborhoodIn(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestExecNeighborhoodFetch, argc, argv);
}
