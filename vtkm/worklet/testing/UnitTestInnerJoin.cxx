//
// Created by ollie on 12/19/17.
//

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/worklet/connectivities/InnerJoin.h>

template <typename DeviceAdapter>
class TestInnerJoin
{
public:
  void TestTwoArrays() const
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    std::vector<vtkm::Id> A = { 8, 3, 6, 8, 9, 5, 12, 10, 14 };
    std::vector<vtkm::Id> B = { 7, 11, 9, 8, 5, 1, 0, 5 };

    vtkm::cont::ArrayHandle<vtkm::Id> A_arr = vtkm::cont::make_ArrayHandle(A);
    vtkm::cont::ArrayHandle<vtkm::Id> B_arr = vtkm::cont::make_ArrayHandle(B);
    vtkm::cont::ArrayHandle<vtkm::Id> idxA;
    vtkm::cont::ArrayHandle<vtkm::Id> idxB;

    Algorithm::Copy(vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, A_arr.GetNumberOfValues()),
                    idxA);
    Algorithm::Copy(vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, B_arr.GetNumberOfValues()),
                    idxB);

    vtkm::cont::ArrayHandle<vtkm::Id> joinedIndex;
    vtkm::cont::ArrayHandle<vtkm::Id> outA;
    vtkm::cont::ArrayHandle<vtkm::Id> outB;

    InnerJoin<DeviceAdapter>().Run(A_arr, idxA, B_arr, idxB, joinedIndex, outA, outB);

    for (int i = 0; i < joinedIndex.GetNumberOfValues(); i++)
    {
      std::cout << "key: " << joinedIndex.GetPortalConstControl().Get(i)
                << ", value1: " << outA.GetPortalConstControl().Get(i)
                << ", value2: " << outB.GetPortalConstControl().Get(i) << std::endl;
    }
  }

  void operator()() const { this->TestTwoArrays(); }
};

int UnitTestInnerJoin(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestInnerJoin<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}