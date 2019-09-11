//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <random>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/worklet/KdTree3D.h>

namespace
{

using Algorithm = vtkm::cont::Algorithm;

////brute force method /////
template <typename CoordiVecT, typename CoordiPortalT, typename CoordiT>
VTKM_EXEC_CONT vtkm::Id NNSVerify3D(CoordiVecT qc, CoordiPortalT coordiPortal, CoordiT& dis)
{
  dis = std::numeric_limits<CoordiT>::max();
  vtkm::Id nnpIdx = 0;

  for (vtkm::Int32 i = 0; i < coordiPortal.GetNumberOfValues(); i++)
  {
    CoordiT splitX = coordiPortal.Get(i)[0];
    CoordiT splitY = coordiPortal.Get(i)[1];
    CoordiT splitZ = coordiPortal.Get(i)[2];
    CoordiT _dis =
      vtkm::Sqrt((splitX - qc[0]) * (splitX - qc[0]) + (splitY - qc[1]) * (splitY - qc[1]) +
                 (splitZ - qc[2]) * (splitZ - qc[2]));
    if (_dis < dis)
    {
      dis = _dis;
      nnpIdx = i;
    }
  }
  return nnpIdx;
}

class NearestNeighborSearchBruteForce3DWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn qcIn,
                                WholeArrayIn treeCoordiIn,
                                FieldOut nnIdOut,
                                FieldOut nnDisOut);
  using ExecutionSignature = void(_1, _2, _3, _4);

  VTKM_CONT
  NearestNeighborSearchBruteForce3DWorklet() {}

  template <typename CoordiVecType, typename CoordiPortalType, typename IdType, typename CoordiType>
  VTKM_EXEC void operator()(const CoordiVecType& qc,
                            const CoordiPortalType& coordiPortal,
                            IdType& nnId,
                            CoordiType& nnDis) const
  {
    nnDis = std::numeric_limits<CoordiType>::max();

    nnId = NNSVerify3D(qc, coordiPortal, nnDis);
  }
};

void TestKdTreeBuildNNS(vtkm::cont::DeviceAdapterId deviceId)
{
  vtkm::Int32 nTrainingPoints = 1000;
  vtkm::Int32 nTestingPoint = 1000;

  std::vector<vtkm::Vec3f_32> coordi;

  ///// randomly generate training points/////
  std::default_random_engine dre;
  std::uniform_real_distribution<vtkm::Float32> dr(0.0f, 10.0f);

  for (vtkm::Int32 i = 0; i < nTrainingPoints; i++)
  {
    coordi.push_back(vtkm::make_Vec(dr(dre), dr(dre), dr(dre)));
  }

  ///// preprare data to build 3D kd tree /////
  auto coordi_Handle = vtkm::cont::make_ArrayHandle(coordi);

  // Run data
  vtkm::worklet::KdTree3D kdtree3d;
  kdtree3d.Build(coordi_Handle);

  //Nearest Neighbor worklet Testing
  /// randomly generate testing points /////
  std::vector<vtkm::Vec3f_32> qcVec;
  for (vtkm::Int32 i = 0; i < nTestingPoint; i++)
  {
    qcVec.push_back(vtkm::make_Vec(dr(dre), dr(dre), dr(dre)));
  }

  ///// preprare testing data /////
  auto qc_Handle = vtkm::cont::make_ArrayHandle(qcVec);

  vtkm::cont::ArrayHandle<vtkm::Id> nnId_Handle;
  vtkm::cont::ArrayHandle<vtkm::Float32> nnDis_Handle;

  kdtree3d.Run(coordi_Handle, qc_Handle, nnId_Handle, nnDis_Handle, deviceId);

  vtkm::cont::ArrayHandle<vtkm::Id> bfnnId_Handle;
  vtkm::cont::ArrayHandle<vtkm::Float32> bfnnDis_Handle;
  NearestNeighborSearchBruteForce3DWorklet nnsbf3dWorklet;
  vtkm::worklet::DispatcherMapField<NearestNeighborSearchBruteForce3DWorklet> nnsbf3DDispatcher(
    nnsbf3dWorklet);
  nnsbf3DDispatcher.Invoke(
    qc_Handle, vtkm::cont::make_ArrayHandle(coordi), bfnnId_Handle, bfnnDis_Handle);

  ///// verfity search result /////
  bool passTest = true;
  for (vtkm::Int32 i = 0; i < nTestingPoint; i++)
  {
    vtkm::Id workletIdx = nnId_Handle.GetPortalControl().Get(i);
    vtkm::Id bfworkletIdx = bfnnId_Handle.GetPortalControl().Get(i);

    if (workletIdx != bfworkletIdx)
    {
      passTest = false;
    }
  }

  VTKM_TEST_ASSERT(passTest, "Kd tree NN search result incorrect.");
}

} // anonymous namespace

int UnitTestKdTreeBuildNNS(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(TestKdTreeBuildNNS, argc, argv);
}
