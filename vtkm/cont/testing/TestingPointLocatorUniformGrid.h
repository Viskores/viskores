//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_testing_TestingPointLocatorUniformGrid_h
#define vtk_m_cont_testing_TestingPointLocatorUniformGrid_h

#include <random>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/PointLocatorUniformGrid.h>
#include <vtkm/exec/PointLocatorUniformGrid.h>

////brute force method /////
template <typename CoordiVecT, typename CoordiPortalT, typename CoordiT>
VTKM_EXEC_CONT vtkm::Id NNSVerify3D(CoordiVecT qc, CoordiPortalT coordiPortal, CoordiT& dis2)
{
  dis2 = std::numeric_limits<CoordiT>::max();
  vtkm::Id nnpIdx = -1;

  for (vtkm::Int32 i = 0; i < coordiPortal.GetNumberOfValues(); i++)
  {
    CoordiT splitX = coordiPortal.Get(i)[0];
    CoordiT splitY = coordiPortal.Get(i)[1];
    CoordiT splitZ = coordiPortal.Get(i)[2];
    CoordiT _dis2 = (splitX - qc[0]) * (splitX - qc[0]) + (splitY - qc[1]) * (splitY - qc[1]) +
      (splitZ - qc[2]) * (splitZ - qc[2]);
    if (_dis2 < dis2)
    {
      dis2 = _dis2;
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

class PointLocatorUniformGridWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn qcIn,
                                ExecObject locator,
                                FieldOut nnIdOut,
                                FieldOut nnDistOut);

  typedef void ExecutionSignature(_1, _2, _3, _4);

  VTKM_CONT
  PointLocatorUniformGridWorklet() {}

  template <typename CoordiVecType, typename Locator>
  VTKM_EXEC void operator()(const CoordiVecType& qc,
                            const Locator& locator,
                            vtkm::Id& nnIdOut,
                            vtkm::FloatDefault& nnDis) const
  {
    locator->FindNearestNeighbor(qc, nnIdOut, nnDis);
  }
};

template <typename DeviceAdapter>
class TestingPointLocatorUniformGrid
{
public:
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
  void TestTest() const
  {
    vtkm::Int32 nTrainingPoints = 5;
    vtkm::Int32 nTestingPoint = 1;

    std::vector<vtkm::Vec3f_32> coordi;

    ///// randomly generate training points/////
    std::default_random_engine dre;
    std::uniform_real_distribution<vtkm::Float32> dr(0.0f, 10.0f);

    for (vtkm::Int32 i = 0; i < nTrainingPoints; i++)
    {
      coordi.push_back(vtkm::make_Vec(dr(dre), dr(dre), dr(dre)));
    }
    // Add a point to each corner to test the case where points might slip out
    // of the range by epsilon
    coordi.push_back(vtkm::make_Vec(00.0f, 00.0f, 00.0f));
    coordi.push_back(vtkm::make_Vec(00.0f, 10.0f, 00.0f));
    coordi.push_back(vtkm::make_Vec(10.0f, 00.0f, 00.0f));
    coordi.push_back(vtkm::make_Vec(10.0f, 10.0f, 00.0f));
    coordi.push_back(vtkm::make_Vec(00.0f, 00.0f, 10.0f));
    coordi.push_back(vtkm::make_Vec(00.0f, 10.0f, 10.0f));
    coordi.push_back(vtkm::make_Vec(10.0f, 00.0f, 10.0f));
    coordi.push_back(vtkm::make_Vec(10.0f, 10.0f, 10.0f));
    auto coordi_Handle = vtkm::cont::make_ArrayHandle(coordi);

    vtkm::cont::CoordinateSystem coord("points", coordi_Handle);

    vtkm::cont::PointLocatorUniformGrid pointLocatorUG;
    pointLocatorUG.SetCoordinates(coord);
    pointLocatorUG.SetRange({ { 0.0, 10.0 } });
    pointLocatorUG.SetNumberOfBins({ 5, 5, 5 });

    vtkm::cont::PointLocator* locator = &pointLocatorUG;
    locator->Update();

    ///// randomly generate testing points/////
    std::vector<vtkm::Vec3f_32> qcVec;
    for (vtkm::Int32 i = 0; i < nTestingPoint; i++)
    {
      qcVec.push_back(vtkm::make_Vec(dr(dre), dr(dre), dr(dre)));
    }
    // Test near each corner to make sure that corner gets included
    qcVec.push_back(vtkm::make_Vec(0.01f, 0.01f, 0.01f));
    qcVec.push_back(vtkm::make_Vec(0.01f, 9.99f, 0.01f));
    qcVec.push_back(vtkm::make_Vec(9.99f, 0.01f, 0.01f));
    qcVec.push_back(vtkm::make_Vec(9.99f, 9.99f, 0.01f));
    qcVec.push_back(vtkm::make_Vec(0.01f, 0.01f, 9.991f));
    qcVec.push_back(vtkm::make_Vec(0.01f, 9.99f, 9.99f));
    qcVec.push_back(vtkm::make_Vec(9.99f, 0.01f, 9.99f));
    qcVec.push_back(vtkm::make_Vec(9.99f, 9.99f, 9.99f));
    auto qc_Handle = vtkm::cont::make_ArrayHandle(qcVec);

    vtkm::cont::ArrayHandle<vtkm::Id> nnId_Handle;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> nnDis_Handle;

    PointLocatorUniformGridWorklet pointLocatorUniformGridWorklet;
    vtkm::worklet::DispatcherMapField<PointLocatorUniformGridWorklet> locatorDispatcher(
      pointLocatorUniformGridWorklet);
    locatorDispatcher.SetDevice(DeviceAdapter());
    locatorDispatcher.Invoke(qc_Handle, locator, nnId_Handle, nnDis_Handle);

    // brute force
    vtkm::cont::ArrayHandle<vtkm::Id> bfnnId_Handle;
    vtkm::cont::ArrayHandle<vtkm::Float32> bfnnDis_Handle;
    NearestNeighborSearchBruteForce3DWorklet nnsbf3dWorklet;
    vtkm::worklet::DispatcherMapField<NearestNeighborSearchBruteForce3DWorklet> nnsbf3DDispatcher(
      nnsbf3dWorklet);
    nnsbf3DDispatcher.SetDevice(DeviceAdapter());
    nnsbf3DDispatcher.Invoke(
      qc_Handle, vtkm::cont::make_ArrayHandle(coordi), bfnnId_Handle, bfnnDis_Handle);

    ///// verify search result /////
    bool passTest = true;
    for (vtkm::Int32 i = 0; i < nTestingPoint; i++)
    {
      vtkm::Id workletIdx = nnId_Handle.GetPortalControl().Get(i);
      vtkm::FloatDefault workletDis = nnDis_Handle.GetPortalConstControl().Get(i);
      vtkm::Id bfworkletIdx = bfnnId_Handle.GetPortalControl().Get(i);
      vtkm::FloatDefault bfworkletDis = bfnnDis_Handle.GetPortalConstControl().Get(i);

      if (workletIdx != bfworkletIdx)
      {
        std::cout << "bf index: " << bfworkletIdx << ", dis: " << bfworkletDis
                  << ", grid: " << workletIdx << ", dis " << workletDis << std::endl;
        passTest = false;
      }
    }
    VTKM_TEST_ASSERT(passTest, "Uniform Grid NN search result incorrect.");
  }

  void operator()() const
  {
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapter());
    this->TestTest();
  }
};

#endif // vtk_m_cont_testing_TestingPointLocatorUniformGrid_h
