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

#include <vtkm/worklet/connectivities/GraphConnectivity.h>

class AdjacentDifference : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn index, WholeArrayIn counts, FieldOut outputCount);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename WholeArrayType>
  VTKM_EXEC void operator()(const vtkm::Id& index,
                            const WholeArrayType& counts,
                            int& difference) const
  {
    difference = counts.Get(index + 1) - counts.Get(index);
  }
};

class SameComponent : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn start,
                                FieldIn degree,
                                WholeArrayIn conns,
                                WholeArrayIn comps,
                                AtomicArrayInOut same);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4, _5);

  template <typename Conn, typename Comp, typename AtomicSame>
  VTKM_EXEC void operator()(vtkm::Id index,
                            int start,
                            int degree,
                            const Conn& conns,
                            const Comp& comps,
                            AtomicSame& same) const
  {
    for (vtkm::Id offset = start; offset < start + degree; ++offset)
    {
      vtkm::Id neighbor = conns.Get(offset);
      if (comps.Get(index) != comps.Get(neighbor))
      {
        same.Set(0, 0);
      }
    }
  }
};

class TestGraphConnectivity
{
public:
  void TestECL_CC(const std::string& filename, int ncomps) const
  {
    auto pathname =
      vtkm::cont::testing::Testing::GetTestDataBasePath() + "/third_party/ecl_cc/" + filename;
    std::ifstream stream(pathname, std::ios_base::in | std::ios_base::binary);

    int nnodes;
    stream.read(reinterpret_cast<char*>(&nnodes), sizeof(nnodes));

    int nedges;
    stream.read(reinterpret_cast<char*>(&nedges), sizeof(nedges));

    // CSR, there is one more element in offsets thant the actual number of nodes.
    std::vector<int> offsets(nnodes + 1);
    std::vector<int> conns(nedges);

    stream.read(reinterpret_cast<char*>(offsets.data()), (nnodes + 1) * sizeof(int));
    stream.read(reinterpret_cast<char*>(conns.data()), nedges * sizeof(int));

    vtkm::cont::ArrayHandle<int> counts_h;
    vtkm::cont::Invoker invoke;
    invoke(AdjacentDifference{},
           vtkm::cont::make_ArrayHandleCounting(0, 1, nnodes),
           vtkm::cont::make_ArrayHandle<int>(offsets, vtkm::CopyFlag::On),
           counts_h);

    offsets.pop_back();
    vtkm::cont::ArrayHandle<int> offsets_h =
      vtkm::cont::make_ArrayHandle(offsets, vtkm::CopyFlag::On);

    vtkm::cont::ArrayHandle<int> conns_h = vtkm::cont::make_ArrayHandle(conns, vtkm::CopyFlag::Off);

    vtkm::cont::ArrayHandle<vtkm::Id> comps_h;
    vtkm::worklet::connectivity::GraphConnectivity().Run(counts_h, offsets_h, conns_h, comps_h);

    VTKM_TEST_ASSERT(vtkm::cont::Algorithm::Reduce(comps_h, vtkm::Id(0), vtkm::Maximum{}) ==
                       ncomps - 1,
                     "number of components mismatch");

    vtkm::cont::ArrayHandle<vtkm::UInt32> atomicSame;
    atomicSame.Allocate(1);
    atomicSame.WritePortal().Set(0, 1);

    invoke(SameComponent{}, offsets_h, counts_h, conns_h, comps_h, atomicSame);
    VTKM_TEST_ASSERT(atomicSame.ReadPortal().Get(0) == 1,
                     "Neighboring nodes don't have the same component id");
  }

  void TestECL_CC_DataSets() const { TestECL_CC("internet.egr", 1); }

  void TestSimpleGraph() const
  {
    vtkm::cont::ArrayHandle<vtkm::Id> counts_h =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 1, 1, 2, 2, 2 });
    vtkm::cont::ArrayHandle<vtkm::Id> offsets_h =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 2, 4, 6 });
    vtkm::cont::ArrayHandle<vtkm::Id> conn_h =
      vtkm::cont::make_ArrayHandle<vtkm::Id>({ 2, 4, 0, 3, 2, 4, 1, 3 });
    vtkm::cont::ArrayHandle<vtkm::Id> comps;

    vtkm::worklet::connectivity::GraphConnectivity().Run(counts_h, offsets_h, conn_h, comps);

    for (int i = 0; i < comps.GetNumberOfValues(); i++)
    {
      VTKM_TEST_ASSERT(comps.ReadPortal().Get(i) == 0, "Components has unexpected value.");
    }
  }

  void operator()() const
  {
    TestSimpleGraph();
    TestECL_CC_DataSets();
  }
};

int UnitTestGraphConnectivity(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestGraphConnectivity(), argc, argv);
}
