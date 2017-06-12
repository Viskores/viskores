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

#include <random>
#include <vtkm/worklet/spatialstructure/KdTree3D.h>
#include <vtkm/worklet/spatialstructure/KdTree3DNNSearch.h>

namespace
{

typedef vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> Algorithm;

////brute force method /////
template <typename CoordiVecT, typename CoordiPortalT, typename CoordiT>
VTKM_EXEC_CONT vtkm::Id NNSVerify3D(CoordiVecT qc, CoordiPortalT coordiPortal, CoordiT& dis)
{
  dis = std::numeric_limits<CoordiT>::max();
  vtkm::Id nnpIdx;

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
  typedef void ControlSignature(FieldIn<> qcIn,
                                WholeArrayIn<> treeCoordiIn,
                                FieldOut<> nnIdOut,
                                FieldOut<> nnDisOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);

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

void TestKdTreeBuildNNS()
{
  vtkm::Int32 nTrainingPoints = 1000;
  vtkm::Int32 nTestingPoint = 1000;

  std::vector<vtkm::Float32> xcoordi;
  std::vector<vtkm::Float32> ycoordi;
  std::vector<vtkm::Float32> zcoordi;

  std::vector<vtkm::Vec<vtkm::Float32, 3>> coordi;

  ///// randomly genarate training points/////
  std::default_random_engine dre;
  std::uniform_real_distribution<vtkm::Float32> dr(0.0f, 10.0f);

  for (vtkm::Int32 i = 0; i < nTrainingPoints; i++)
  {
    xcoordi.push_back(dr(dre));
    ycoordi.push_back(dr(dre));
    zcoordi.push_back(dr(dre));
    vtkm::Vec<vtkm::Float32, 3> c;
    c[0] = xcoordi[xcoordi.size() - 1];
    c[1] = ycoordi[ycoordi.size() - 1];
    c[2] = zcoordi[zcoordi.size() - 1];
    coordi.push_back(c);
  }

  ///// preprare data to build 3D kd tree /////
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> coordi_Handle;
  Algorithm::Copy(vtkm::cont::make_ArrayHandle(coordi), coordi_Handle);

  vtkm::cont::ArrayHandle<vtkm::Id> pointId_Handle;
  vtkm::cont::ArrayHandle<vtkm::Id> splitId_Handle;

  vtkm::worklet::spatialstructure::KdTree3D kdtree3D;
  // Run data
  kdtree3D.Run(coordi_Handle, pointId_Handle, splitId_Handle, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());

  //Nearest Neighbor worklet Testing
  /// randomly generate testing points /////
  std::vector<vtkm::Vec<vtkm::Float32, 3>> qcVec;
  for (vtkm::Int32 i = 0; i < nTestingPoint; i++)
  {
    vtkm::Vec<vtkm::Float32, 3> qc;
    qc[0] = dr(dre);
    qc[1] = dr(dre);
    qc[2] = dr(dre);
    qcVec.push_back(qc);
  }

  ///// preprare testing data /////
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> qc_Handle;
  Algorithm::Copy(vtkm::cont::make_ArrayHandle(qcVec), qc_Handle);

  vtkm::cont::ArrayHandle<vtkm::Id> nnId_Handle;
  vtkm::cont::ArrayHandle<vtkm::Float32> nnDis_Handle;

  vtkm::worklet::spatialstructure::KdTree3DNNSearch kdtree3DNNS;
  kdtree3DNNS.Run(coordi_Handle,
                  pointId_Handle,
                  splitId_Handle,
                  qc_Handle,
                  nnId_Handle,
                  nnDis_Handle,
                  VTKM_DEFAULT_DEVICE_ADAPTER_TAG());

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

int UnitTestKdTreeBuildNNS(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestKdTreeBuildNNS);
}
