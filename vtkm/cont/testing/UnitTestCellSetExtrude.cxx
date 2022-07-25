//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/ScatterPermutation.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleXGCCoordinates.h>
#include <vtkm/cont/CellSetExtrude.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/field_conversion/PointAverage.h>

namespace
{
std::vector<float> points_rz = { 1.72485139f, 0.020562f,   1.73493571f,
                                 0.02052826f, 1.73478011f, 0.02299051f }; //really a vec<float,2>
std::vector<vtkm::Int32> topology = { 0, 2, 1 };
std::vector<vtkm::Int32> nextNode = { 0, 1, 2 };


struct CopyTopo : public vtkm::worklet::WorkletVisitCellsWithPoints
{
  typedef void ControlSignature(CellSetIn, FieldOutCell);
  typedef _2 ExecutionSignature(CellShape, PointIndices);

  template <typename T>
  VTKM_EXEC T&& operator()(vtkm::CellShapeTagWedge, T&& t) const
  {
    return std::forward<T>(t);
  }
};

struct CopyTopoScatter : public vtkm::worklet::WorkletVisitCellsWithPoints
{
  typedef void ControlSignature(CellSetIn, FieldOutCell);
  typedef _2 ExecutionSignature(CellShape, PointIndices);

  using ScatterType = vtkm::worklet::ScatterPermutation<vtkm::cont::StorageTagCounting>;

  template <typename T>
  VTKM_EXEC T&& operator()(vtkm::CellShapeTagWedge, T&& t) const
  {
    return std::forward<T>(t);
  }
};

struct CopyReverseCellCount : public vtkm::worklet::WorkletVisitPointsWithCells
{
  typedef void ControlSignature(CellSetIn, FieldOutPoint, FieldOutPoint);
  typedef _2 ExecutionSignature(CellShape, CellCount, CellIndices, _3);

  template <typename CellIndicesType, typename OutVec>
  VTKM_EXEC vtkm::Int32 operator()(vtkm::CellShapeTagVertex,
                                   vtkm::IdComponent count,
                                   CellIndicesType&& cellIndices,
                                   OutVec& outIndices) const
  {
    cellIndices.CopyInto(outIndices);

    bool valid = true;
    for (vtkm::IdComponent i = 0; i < count; ++i)
    {
      valid = valid && cellIndices[i] >= 0;
    }
    return (valid && count == cellIndices.GetNumberOfComponents()) ? count : -1;
  }
};

struct CopyReverseCellCountScatter : public vtkm::worklet::WorkletVisitPointsWithCells
{
  typedef void ControlSignature(CellSetIn, FieldOutPoint, FieldOutPoint);
  typedef _2 ExecutionSignature(CellShape, CellCount, CellIndices, _3);

  using ScatterType = vtkm::worklet::ScatterPermutation<vtkm::cont::StorageTagCounting>;

  template <typename CellIndicesType, typename OutVec>
  VTKM_EXEC vtkm::Int32 operator()(vtkm::CellShapeTagVertex,
                                   vtkm::IdComponent count,
                                   CellIndicesType&& cellIndices,
                                   OutVec& outIndices) const
  {
    cellIndices.CopyInto(outIndices);

    bool valid = true;
    for (vtkm::IdComponent i = 0; i < count; ++i)
    {
      valid = valid && cellIndices[i] >= 0;
    }
    return (valid && count == cellIndices.GetNumberOfComponents()) ? count : -1;
  }
};

template <typename T, typename S>
void verify_topo(vtkm::cont::ArrayHandle<vtkm::Vec<T, 6>, S> const& handle,
                 vtkm::Id expectedLen,
                 vtkm::Id skip)
{
  auto portal = handle.ReadPortal();
  VTKM_TEST_ASSERT((portal.GetNumberOfValues() * skip) == expectedLen,
                   "topology portal size is incorrect");

  for (vtkm::Id i = 0; i < expectedLen; i += skip)
  {
    auto v = portal.Get(i / skip);
    vtkm::Vec<vtkm::Id, 6> e;
    vtkm::Id offset1 = i * static_cast<vtkm::Id>(topology.size());
    vtkm::Id offset2 =
      (i < expectedLen - 1) ? (offset1 + static_cast<vtkm::Id>(topology.size())) : 0;
    e[0] = (static_cast<vtkm::Id>(topology[0]) + offset1);
    e[1] = (static_cast<vtkm::Id>(topology[1]) + offset1);
    e[2] = (static_cast<vtkm::Id>(topology[2]) + offset1);
    e[3] = (static_cast<vtkm::Id>(topology[0]) + offset2);
    e[4] = (static_cast<vtkm::Id>(topology[1]) + offset2);
    e[5] = (static_cast<vtkm::Id>(topology[2]) + offset2);
    std::cout << "v, e: " << v << ", " << e << "\n";
    VTKM_TEST_ASSERT(test_equal(v, e), "incorrect conversion of topology to Cartesian space");
  }
}

void verify_reverse_topo(const vtkm::cont::ArrayHandle<vtkm::Int32>& counts,
                         const vtkm::cont::ArrayHandle<vtkm::Id2>& indices,
                         vtkm::Id expectedLen,
                         vtkm::Id skip)
{
  auto countsPortal = counts.ReadPortal();
  VTKM_TEST_ASSERT((countsPortal.GetNumberOfValues() * skip) == expectedLen,
                   "topology portal size is incorrect");
  auto indicesPortal = indices.ReadPortal();
  VTKM_TEST_ASSERT((indicesPortal.GetNumberOfValues() * skip) == expectedLen);
  for (vtkm::Id i = 0; i < expectedLen - 1; i += skip)
  {
    auto vCount = countsPortal.Get(i / skip);
    auto vIndices = indicesPortal.Get(i / skip);
    std::cout << vCount << ":" << vIndices << " ";
    vtkm::Int32 eCount = 2;
    vtkm::Id2 eIndices((i / 3) - 1, i / 3);
    if (eIndices[0] < 0)
    {
      eIndices[0] = (expectedLen / 3) - 1;
    }
    VTKM_TEST_ASSERT(vCount == eCount);
    VTKM_TEST_ASSERT(vIndices == eIndices);
  }
  std::cout << "\n";
}
int TestCellSetExtrude()
{
  const std::size_t numPlanes = 8;

  auto coords = vtkm::cont::make_ArrayHandleXGCCoordinates(points_rz, numPlanes, false);
  auto cells = vtkm::cont::make_CellSetExtrude(topology, coords, nextNode);
  VTKM_TEST_ASSERT(cells.GetNumberOfPoints() == coords.GetNumberOfValues(),
                   "number of points don't match between cells and coordinates");

  vtkm::cont::Invoker invoke;

  std::cout << "Verify the topology by copying it into another array\n";
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<int, 6>> output;
    invoke(CopyTopo{}, cells, output);
    verify_topo(output, numPlanes, 1);
  }

  std::cout << "Verify the topology works with a scatter\n";
  {
    constexpr vtkm::Id skip = 2;
    vtkm::cont::ArrayHandle<vtkm::Vec<int, 6>> output;
    invoke(CopyTopoScatter{},
           CopyTopoScatter::ScatterType(
             vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, skip, numPlanes / skip)),
           cells,
           output);
    verify_topo(output, numPlanes, skip);
  }

  std::cout << "Verify the reverse topology by copying the number of cells each point is "
            << "used by it into another array.\n";
  {
    vtkm::cont::ArrayHandle<vtkm::Int32> incidentCount;
    vtkm::cont::ArrayHandle<vtkm::Id2> incidentIndices;
    invoke(CopyReverseCellCount{}, cells, incidentCount, incidentIndices);
    verify_reverse_topo(incidentCount, incidentIndices, 3 * numPlanes, 1);
  }

  std::cout << "Verify reverse topology map with scatter\n";
  {
    constexpr vtkm::Id skip = 2;
    vtkm::cont::ArrayHandle<vtkm::Int32> incidentCount;
    vtkm::cont::ArrayHandle<vtkm::Id2> incidentIndices;
    invoke(CopyReverseCellCountScatter{},
           CopyTopoScatter::ScatterType(
             vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, skip, (3 * numPlanes) / skip)),
           cells,
           incidentCount,
           incidentIndices);
    verify_reverse_topo(incidentCount, incidentIndices, 3 * numPlanes, skip);
  }

  return 0;
}
}

int UnitTestCellSetExtrude(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetExtrude, argc, argv);
}
