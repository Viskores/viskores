//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_TemporalGridEvaluators_h
#define vtk_m_worklet_particleadvection_TemporalGridEvaluators_h

#include <vtkm/worklet/particleadvection/GridEvaluators.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename DeviceAdapter, typename FieldArrayType>
class ExecutionTemporalGridEvaluator
{
private:
  using GridEvaluator = vtkm::worklet::particleadvection::GridEvaluator<FieldArrayType>;
  using ExecutionGridEvaluator =
    vtkm::worklet::particleadvection::ExecutionGridEvaluator<DeviceAdapter, FieldArrayType>;

public:
  VTKM_CONT
  ExecutionTemporalGridEvaluator() = default;

  VTKM_CONT
  ExecutionTemporalGridEvaluator(const GridEvaluator& evaluatorOne,
                                 const GridEvaluator& evaluatorTwo,
                                 const vtkm::FloatDefault timeOne,
                                 const vtkm::FloatDefault timeTwo)
    : EvaluatorOne(evaluatorOne.PrepareForExecution(DeviceAdapter()))
    , EvaluatorTwo(evaluatorTwo.PrepareForExecution(DeviceAdapter()))
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
  {
  }

  template <typename Point>
  VTKM_EXEC bool Evaluate(const Point& pos, vtkm::FloatDefault time, Point& out) const
  {
    // Validate time is in bounds for the current two slices.
    if (!(time >= TimeOne && time <= TimeTwo))
      return false;
    bool eval;
    Point one, two;
    eval = this->EvaluatorOne.Evaluate(pos, one);
    if (!eval)
      return false;
    eval = this->EvaluatorTwo.Evaluate(pos, two);
    if (!eval)
      return false;
    // LERP between the two values of calculated field
    // to obtain the new value
    ScalarType proportion = (time - this->TimeOne) / (this->TimeTwo - this->TimeOne);
    out = (1.0f - proportion) * one + proportion * two;
    return true;
  }

private:
  ExecutionGridEvaluator EvaluatorOne;
  ExecutionGridEvaluator EvaluatorTwo;
  vtkm::FloatDefault TimeOne;
  vtkm::FloatDefault TimeTwo;
};

template <typename FieldArrayType>
class TemporalGridEvaluator : public vtkm::cont::ExecutionObjectBase
{
private:
  using GridEvaluator = vtkm::worklet::particleadvection::GridEvaluator<FieldArrayType>;

public:
  TemporalGridEvaluator() = default;

  TemporalGridEvaluator(GridEvaluator& evaluatorOne,
                        GridEvaluator& evaluatorTwo,
                        const vtkm::FloatDefault timeOne,
                        const vtkm::FloatDefault timeTwo)
    : EvaluatorOne(evaluatorOne)
    , EvaluatorTwo(evaluatorTwo)
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
  {
  }

  template <typename DeviceAdapter>
  VTKM_CONT ExecutionTemporalGridEvaluator<DeviceAdapter, FieldArrayType> PrepareForExecution(
    DeviceAdapter) const
  {
    return ExecutionTemporalGridEvaluator<DeviceAdapter, FieldArrayType>(
      this->EvaluatorOne, this->EvaluatorTwo, this->TimeOne, this->TimeTwo);
  }

private:
  GridEvaluator EvaluatorOne;
  GridEvaluator EvaluatorTwo;
  vtkm::FloatDefault TimeOne;
  vtkm::FloatDefault TimeTwo;
};

} // namespace particleadvection
} // naemspace worklet
} // namespace vtkm

#endif
