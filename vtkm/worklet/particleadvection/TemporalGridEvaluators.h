//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_TemporalGridEvaluators_h
#define vtk_m_worklet_particleadvection_TemporalGridEvaluators_h

#include <vtkm/worklet/particleadvection/GridEvaluatorStatus.h>
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
                                 const vtkm::FloatDefault timeOne,
                                 const GridEvaluator& evaluatorTwo,
                                 const vtkm::FloatDefault timeTwo)
    : EvaluatorOne(evaluatorOne.PrepareForExecution(DeviceAdapter()))
    , EvaluatorTwo(evaluatorTwo.PrepareForExecution(DeviceAdapter()))
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
    , TimeDiff(timeTwo - timeOne)
  {
  }

  template <typename Point>
  VTKM_EXEC bool IsWithinSpatialBoundary(const Point point) const
  {
    return this->EvaluatorOne.IsWithinSpatialBoundary(point) &&
      this->EvaluatorTwo.IsWithinSpatialBoundary(point);
  }

  VTKM_EXEC
  bool IsWithinTemporalBoundary(const vtkm::FloatDefault time) const
  {
    return time >= TimeOne && time <= TimeTwo;
  }

  VTKM_EXEC
  vtkm::Bounds GetSpatialBoundary() const { return this->EvaluatorTwo.GetSpatialBoundary(); }

  VTKM_EXEC_CONT
  vtkm::FloatDefault GetTemporalBoundary(vtkm::Id direction) const
  {
    return direction > 0 ? this->TimeTwo : this->TimeOne;
  }

  template <typename Point>
  VTKM_EXEC GridEvaluatorStatus Evaluate(const Point& pos,
                                         vtkm::FloatDefault time,
                                         Point& out) const
  {
    // Validate time is in bounds for the current two slices.
    GridEvaluatorStatus status;

    if (!(time >= TimeOne && time <= TimeTwo))
    {
      status.SetFail();
      status.SetTemporalBounds();
      return status;
    }

    Point one, two;
    status = this->EvaluatorOne.Evaluate(pos, one);
    if (status.CheckFail())
      return status;
    status = this->EvaluatorTwo.Evaluate(pos, two);
    if (status.CheckFail())
      return status;

    // LERP between the two values of calculated fields to obtain the new value
    vtkm::FloatDefault proportion = (time - this->TimeOne) / this->TimeDiff;
    out = vtkm::Lerp(one, two, proportion);

    status.SetOk();
    return status;
  }

private:
  ExecutionGridEvaluator EvaluatorOne;
  ExecutionGridEvaluator EvaluatorTwo;
  vtkm::FloatDefault TimeOne;
  vtkm::FloatDefault TimeTwo;
  vtkm::FloatDefault TimeDiff;
};

template <typename FieldArrayType>
class TemporalGridEvaluator : public vtkm::cont::ExecutionObjectBase
{
private:
  using GridEvaluator = vtkm::worklet::particleadvection::GridEvaluator<FieldArrayType>;

public:
  VTKM_CONT TemporalGridEvaluator() = default;

  VTKM_CONT TemporalGridEvaluator(GridEvaluator& evaluatorOne,
                                  const vtkm::FloatDefault timeOne,
                                  GridEvaluator& evaluatorTwo,
                                  const vtkm::FloatDefault timeTwo)
    : EvaluatorOne(evaluatorOne)
    , EvaluatorTwo(evaluatorTwo)
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
  {
  }

  VTKM_CONT TemporalGridEvaluator(const vtkm::cont::CoordinateSystem& coordinatesOne,
                                  const vtkm::cont::DynamicCellSet& cellsetOne,
                                  const FieldArrayType& fieldOne,
                                  const vtkm::FloatDefault timeOne,
                                  const vtkm::cont::CoordinateSystem& coordinatesTwo,
                                  const vtkm::cont::DynamicCellSet& cellsetTwo,
                                  const FieldArrayType& fieldTwo,
                                  const vtkm::FloatDefault timeTwo)
    : EvaluatorOne(GridEvaluator(coordinatesOne, cellsetOne, fieldOne))
    , EvaluatorTwo(GridEvaluator(coordinatesTwo, cellsetTwo, fieldTwo))
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
  {
  }

  template <typename DeviceAdapter>
  VTKM_CONT ExecutionTemporalGridEvaluator<DeviceAdapter, FieldArrayType> PrepareForExecution(
    DeviceAdapter) const
  {
    return ExecutionTemporalGridEvaluator<DeviceAdapter, FieldArrayType>(
      this->EvaluatorOne, this->TimeOne, this->EvaluatorTwo, this->TimeTwo);
  }

private:
  GridEvaluator EvaluatorOne;
  GridEvaluator EvaluatorTwo;
  vtkm::FloatDefault TimeOne;
  vtkm::FloatDefault TimeTwo;
};

} // namespace particleadvection
} // namespace worklet
} // namespace vtkm

#endif
