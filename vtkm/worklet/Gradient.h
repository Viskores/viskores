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

#ifndef vtk_m_worklet_Gradient_h
#define vtk_m_worklet_Gradient_h

#include <vtkm/CellTraits.h>
#include <vtkm/VecFromPortal.h>
#include <vtkm/VecFromPortalPermute.h>
#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/exec/ParametricCoordinates.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/exec/CellDerivative.h>

namespace vtkm
{
namespace worklet
{
namespace gradient
{

template <typename T>
struct GradientInType : vtkm::ListTagBase<T>
{
};

template <typename T>
struct GradientOutType : vtkm::ListTagBase<vtkm::Vec<T, 3>>
{
};

struct GradientVecOutTypes : vtkm::ListTagBase<vtkm::Vec<vtkm::Vec<vtkm::Float32, 3>, 3>,
                                               vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3>>
{
};

template <typename T>
struct CellGradient : vtkm::worklet::WorkletMapPointToCell
{
  typedef void ControlSignature(CellSetIn,
                                FieldInPoint<Vec3> pointCoordinates,
                                FieldInPoint<GradientInType<T>> inputField,
                                FieldOutCell<GradientOutType<T>> outputField);

  typedef void ExecutionSignature(CellShape, PointCount, _2, _3, _4);
  typedef _1 InputDomain;

  template <typename CellTagType,
            typename PointCoordVecType,
            typename FieldInVecType,
            typename FieldOutType>
  VTKM_EXEC void operator()(CellTagType shape,
                            vtkm::IdComponent pointCount,
                            const PointCoordVecType& wCoords,
                            const FieldInVecType& field,
                            FieldOutType& outputField) const
  {
    vtkm::Vec<vtkm::FloatDefault, 3> center =
      vtkm::exec::ParametricCoordinatesCenter(pointCount, shape, *this);

    outputField = vtkm::exec::CellDerivative(field, wCoords, center, shape, *this);
  }
};

template <typename T>
struct PointGradient : public vtkm::worklet::WorkletMapCellToPoint
{
  typedef void ControlSignature(CellSetIn,
                                WholeCellSetIn<Point, Cell>,
                                WholeArrayIn<Vec3> pointCoordinates,
                                WholeArrayIn<GradientInType<T>> inputField,
                                FieldOutPoint<GradientOutType<T>> outputField);

  typedef void ExecutionSignature(CellCount, CellIndices, WorkIndex, _2, _3, _4, _5);
  typedef _1 InputDomain;

  template <typename FromIndexType,
            typename CellSetInType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename FieldOutType>
  VTKM_EXEC void operator()(const vtkm::IdComponent& numCells,
                            const FromIndexType& cellIds,
                            const vtkm::Id& pointId,
                            const CellSetInType& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            FieldOutType& outputField) const
  {
    using ThreadIndices = vtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>;
    using ValueType = typename WholeFieldIn::ValueType;
    using CellShapeTag = typename CellSetInType::CellShapeTag;

    vtkm::Vec<ValueType, 3> gradient(ValueType(0.0));
    for (vtkm::IdComponent i = 0; i < numCells; ++i)
    {
      const vtkm::Id cellId = cellIds[i];
      ThreadIndices cellIndices(cellId, cellId, 0, geometry);

      const CellShapeTag cellShape = cellIndices.GetCellShape();

      // compute the parametric coordinates for the current point
      const auto wCoords = this->GetValues(cellIndices, pointCoordinates);
      const auto field = this->GetValues(cellIndices, inputField);

      const vtkm::IdComponent pointIndexForCell = this->GetPointIndexForCell(cellIndices, pointId);

      this->ComputeGradient(cellShape, pointIndexForCell, wCoords, field, gradient);
    }

    using BaseGradientType = typename vtkm::BaseComponent<ValueType>::Type;
    const BaseGradientType invNumCells =
      static_cast<BaseGradientType>(1.) / static_cast<BaseGradientType>(numCells);
    using OutValueType = typename FieldOutType::ComponentType;
    outputField[0] = static_cast<OutValueType>(gradient[0] * invNumCells);
    outputField[1] = static_cast<OutValueType>(gradient[1] * invNumCells);
    outputField[2] = static_cast<OutValueType>(gradient[2] * invNumCells);
  }

private:
  template <typename CellShapeTag,
            typename PointCoordVecType,
            typename FieldInVecType,
            typename OutValueType>
  inline VTKM_EXEC void ComputeGradient(CellShapeTag cellShape,
                                        const vtkm::IdComponent& pointIndexForCell,
                                        const PointCoordVecType& wCoords,
                                        const FieldInVecType& field,
                                        vtkm::Vec<OutValueType, 3>& gradient) const
  {
    vtkm::Vec<vtkm::FloatDefault, 3> pCoords;
    vtkm::exec::ParametricCoordinatesPoint(
      wCoords.GetNumberOfComponents(), pointIndexForCell, pCoords, cellShape, *this);

    //we need to add this to a return value
    gradient += vtkm::exec::CellDerivative(field, wCoords, pCoords, cellShape, *this);
  }

  template <typename CellSetInType>
  VTKM_EXEC vtkm::IdComponent GetPointIndexForCell(
    const vtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>& indices,
    vtkm::Id pointId) const
  {
    vtkm::IdComponent result = 0;
    const auto& topo = indices.GetIndicesFrom();
    for (vtkm::IdComponent i = 0; i < topo.GetNumberOfComponents(); ++i)
    {
      if (topo[i] == pointId)
      {
        result = i;
      }
    }
    return result;
  }

  //This is fairly complex so that we can trigger code to extract
  //VecRectilinearPointCoordinates when using structured connectivity, and
  //uniform point coordinates.
  //c++14 would make the return type simply auto
  template <typename CellSetInType, typename WholeFieldIn>
  VTKM_EXEC
    typename vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
                                    vtkm::exec::arg::AspectTagDefault,
                                    vtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>,
                                    typename WholeFieldIn::PortalType>::ValueType
    GetValues(const vtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>& indices,
              const WholeFieldIn& in) const
  {
    //the current problem is that when the topology is structured
    //we are passing in an vtkm::Id when it wants a Id2 or an Id3 that
    //represents the flat index of the topology
    using ExecObjectType = typename WholeFieldIn::PortalType;
    using Fetch = vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
                                         vtkm::exec::arg::AspectTagDefault,
                                         vtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>,
                                         ExecObjectType>;
    Fetch fetch;
    return fetch.Load(indices, in.GetPortal());
  }
};

//-----------------------------------------------------------------------------
template <typename CoordinateSystem, typename T, typename S, typename Device>
struct DeducedPointGrad
{
  DeducedPointGrad(const CoordinateSystem& coords,
                   const vtkm::cont::ArrayHandle<T, S>& field,
                   vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>* result)
    : Points(&coords)
    , Field(&field)
    , Result(result)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cellset) const
  {
    vtkm::worklet::DispatcherMapTopology<PointGradient<T>, Device> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      cellset, //whole cellset in
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  const CoordinateSystem* const Points;
  const vtkm::cont::ArrayHandle<T, S>* const Field;
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>* Result;

private:
  void operator=(const DeducedPointGrad<CoordinateSystem, T, S, Device>&) = delete;
};

} //namespace gradient

class PointGradient
{
public:
  template <typename CellSetType,
            typename CoordinateSystem,
            typename T,
            typename S,
            typename DeviceAdapter>
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> Run(const CellSetType& cells,
                                               const CoordinateSystem& coords,
                                               const vtkm::cont::ArrayHandle<T, S>& field,
                                               DeviceAdapter device)
  {
    //we are using cast and call here as we pass the cells twice to the invoke
    //and want the type resolved once before hand instead of twice
    //by the dispatcher ( that will cost more in time and binary size )

    vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> output;
    gradient::DeducedPointGrad<CoordinateSystem, T, S, DeviceAdapter> func(coords, field, &output);
    vtkm::cont::CastAndCall(cells, func);
    return output;
  }
};

class CellGradient
{
public:
  template <typename CellSetType,
            typename CoordinateSystem,
            typename T,
            typename S,
            typename DeviceAdapter>
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> Run(const CellSetType& cells,
                                               const CoordinateSystem& coords,
                                               const vtkm::cont::ArrayHandle<T, S>& field,
                                               DeviceAdapter device)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> output;
    vtkm::worklet::DispatcherMapTopology<vtkm::worklet::gradient::CellGradient<T>, DeviceAdapter>
      dispatcher;
    dispatcher.Invoke(cells, coords, field, output);
    return output;
  }
};

struct Divergence : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<gradient::GradientVecOutTypes> input,
                                FieldOut<Scalar> output);
  typedef void ExecutionSignature(_1, _2);
  typedef _1 InputDomain;

  template <typename InputType, typename OutputType>
  VTKM_EXEC void operator()(const InputType& input, OutputType& divergence) const
  {
    divergence = input[0][0] + input[1][1] + input[2][2];
  }
};

struct Vorticity : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<gradient::GradientVecOutTypes> input,
                                FieldOut<Vec3> output);
  typedef void ExecutionSignature(_1, _2);
  typedef _1 InputDomain;

  template <typename InputType, typename OutputType>
  VTKM_EXEC void operator()(const InputType& input, OutputType& vorticity) const
  {
    vorticity[0] = input[2][1] - input[1][2];
    vorticity[1] = input[0][2] - input[2][0];
    vorticity[2] = input[1][0] - input[0][1];
  }
};

struct QCriterion : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<gradient::GradientVecOutTypes> input,
                                FieldOut<Scalar> output);
  typedef void ExecutionSignature(_1, _2);
  typedef _1 InputDomain;

  template <typename InputType, typename OutputType>
  VTKM_EXEC void operator()(const InputType& input, OutputType& qcriterion) const
  {
    OutputType t1 = ((input[2][1] - input[1][2]) * (input[2][1] - input[1][2]) +
                     (input[1][0] - input[0][1]) * (input[1][0] - input[0][1]) +
                     (input[0][2] - input[2][0]) * (input[0][2] - input[2][0])) /
      2.0f;
    OutputType t2 = input[0][0] * input[0][0] + input[1][1] * input[1][1] +
      input[2][2] * input[2][2] +
      ((input[1][0] + input[0][1]) * (input[1][0] + input[0][1]) +
       (input[2][0] + input[0][2]) * (input[2][0] + input[0][2]) +
       (input[2][1] + input[1][2]) * (input[2][1] + input[1][2])) /
        2.0f;

    qcriterion = (t1 - t2) / 2.0f;
  }
};
}
} // namespace vtkm::worklet
#endif
