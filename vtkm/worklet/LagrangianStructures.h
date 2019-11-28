//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_LagrangianStructures_h
#define vtk_m_worklet_LagrangianStructures_h

#include <vtkm/Matrix.h>
#include <vtkm/Types.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/worklet/lcs/GridMetaData.h>
#include <vtkm/worklet/lcs/LagrangianStructureHelpers.h>

namespace vtkm
{
namespace worklet
{

template <vtkm::IdComponent dimensions>
class LagrangianStructures;

template <>
class LagrangianStructures<2> : public vtkm::worklet::WorkletMapField
{
public:
  using Scalar = vtkm::FloatDefault;

  VTKM_CONT
  LagrangianStructures(Scalar endTime, vtkm::cont::DynamicCellSet cellSet)
    : EndTime(endTime)
    , GridData(cellSet)
  {
  }

  using ControlSignature = void(WholeArrayIn, WholeArrayIn, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3);

  template <typename PointArray>
  VTKM_EXEC void operator()(const vtkm::Id index,
                            const PointArray& input,
                            const PointArray& output,
                            Scalar& outputField) const
  {
    const vtkm::Vec<vtkm::Id, 6> neighborIndices = this->GridData.GetNeighborIndices(index);

    // Calculate Stretching / Squeezing
    auto xin1 = input.Get(neighborIndices[0]);
    auto xin2 = input.Get(neighborIndices[1]);
    auto yin1 = input.Get(neighborIndices[2]);
    auto yin2 = input.Get(neighborIndices[3]);

    Scalar xDiff = 1.0f / (xin2[0] - xin1[0]);
    Scalar yDiff = 1.0f / (yin2[1] - yin1[1]);

    auto xout1 = output.Get(neighborIndices[0]);
    auto xout2 = output.Get(neighborIndices[1]);
    auto yout1 = output.Get(neighborIndices[2]);
    auto yout2 = output.Get(neighborIndices[3]);

    // Total X gradient w.r.t X, Y
    Scalar f1x = (xout2[0] - xout1[0]) * xDiff;
    Scalar f1y = (yout2[0] - yout1[0]) * yDiff;

    // Total Y gradient w.r.t X, Y
    Scalar f2x = (xout2[1] - xout1[1]) * xDiff;
    Scalar f2y = (yout2[1] - yout1[1]) * yDiff;

    vtkm::Matrix<Scalar, 2, 2> jacobian;
    vtkm::MatrixSetRow(jacobian, 0, vtkm::Vec<Scalar, 2>(f1x, f1y));
    vtkm::MatrixSetRow(jacobian, 1, vtkm::Vec<Scalar, 2>(f2x, f2y));

    detail::ComputeLeftCauchyGreenTensor(jacobian);

    vtkm::Vec<Scalar, 2> eigenValues;
    detail::Jacobi(jacobian, eigenValues);

    Scalar delta = eigenValues[0];
    // Check if we need to clamp these values
    // Also provide options.
    // 1. FTLE
    // 2. FLLE
    // 3. Eigen Values (Min/Max)
    //Scalar delta = trace + sqrtr;
    // Given endTime is in units where start time is 0,
    // else do endTime-startTime
    // return value for computation
    outputField = vtkm::Log(delta) / (static_cast<Scalar>(2.0f) * EndTime);
  }

public:
  // To calculate FTLE field
  Scalar EndTime;
  // To assist in calculation of indices
  detail::GridMetaData GridData;
};

template <>
class LagrangianStructures<3> : public vtkm::worklet::WorkletMapField
{
public:
  using Scalar = vtkm::FloatDefault;

  VTKM_CONT
  LagrangianStructures(Scalar endTime, vtkm::cont::DynamicCellSet cellSet)
    : EndTime(endTime)
    , GridData(cellSet)
  {
  }

  using ControlSignature = void(WholeArrayIn, WholeArrayIn, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3);

  /*
   * Point position arrays are the input and the output positions of the particle advection.
   */
  template <typename PointArray>
  VTKM_EXEC void operator()(const vtkm::Id index,
                            const PointArray& input,
                            const PointArray& output,
                            Scalar& outputField) const
  {
    const vtkm::Vec<vtkm::Id, 6> neighborIndices = this->GridData.GetNeighborIndices(index);

    auto xin1 = input.Get(neighborIndices[0]);
    auto xin2 = input.Get(neighborIndices[1]);
    auto yin1 = input.Get(neighborIndices[2]);
    auto yin2 = input.Get(neighborIndices[3]);
    auto zin1 = input.Get(neighborIndices[4]);
    auto zin2 = input.Get(neighborIndices[5]);

    Scalar xDiff = 1.0f / (xin2[0] - xin1[0]);
    Scalar yDiff = 1.0f / (yin2[1] - yin1[1]);
    Scalar zDiff = 1.0f / (zin2[2] - zin1[2]);

    auto xout1 = output.Get(neighborIndices[0]);
    auto xout2 = output.Get(neighborIndices[1]);
    auto yout1 = output.Get(neighborIndices[2]);
    auto yout2 = output.Get(neighborIndices[3]);
    auto zout1 = output.Get(neighborIndices[4]);
    auto zout2 = output.Get(neighborIndices[5]);

    // Total X gradient w.r.t X, Y, Z
    Scalar f1x = (xout2[0] - xout1[0]) * xDiff;
    Scalar f1y = (yout2[0] - yout1[0]) * yDiff;
    Scalar f1z = (zout2[0] - zout1[0]) * zDiff;

    // Total Y gradient w.r.t X, Y, Z
    Scalar f2x = (xout2[1] - xout1[1]) * xDiff;
    Scalar f2y = (yout2[1] - yout1[1]) * yDiff;
    Scalar f2z = (zout2[1] - zout1[1]) * zDiff;

    // Total Z gradient w.r.t X, Y, Z
    Scalar f3x = (xout2[2] - xout1[2]) * xDiff;
    Scalar f3y = (yout2[2] - yout1[2]) * yDiff;
    Scalar f3z = (zout2[2] - zout1[2]) * zDiff;

    vtkm::Matrix<Scalar, 3, 3> jacobian;
    vtkm::MatrixSetRow(jacobian, 0, vtkm::Vec<Scalar, 3>(f1x, f1y, f1z));
    vtkm::MatrixSetRow(jacobian, 1, vtkm::Vec<Scalar, 3>(f2x, f2y, f2z));
    vtkm::MatrixSetRow(jacobian, 2, vtkm::Vec<Scalar, 3>(f3x, f3y, f3z));

    detail::ComputeLeftCauchyGreenTensor(jacobian);

    vtkm::Vec<Scalar, 3> eigenValues;
    detail::Jacobi(jacobian, eigenValues);

    Scalar delta = eigenValues[0];
    // Given endTime is in units where start time is 0. else do endTime-startTime
    // return value for ftle computation
    outputField = vtkm::Log(delta) / (static_cast<Scalar>(2.0f) * EndTime);
  }

public:
  // To calculate FTLE field
  Scalar EndTime;
  // To assist in calculation of indices
  detail::GridMetaData GridData;
};

} // worklet
} // vtkm

#endif //vtk_m_worklet_LagrangianStructures_h
