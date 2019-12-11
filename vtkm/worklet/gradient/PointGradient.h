//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_gradient_PointGradient_h
#define vtk_m_worklet_gradient_PointGradient_h

#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/ParametricCoordinates.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/worklet/gradient/GradientOutput.h>


namespace vtkm
{
namespace worklet
{
namespace gradient
{

template <typename T>
using PointGradientInType = vtkm::List<T>;

template <typename T>
struct PointGradient : public vtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn,
                                WholeCellSetIn<Cell, Point>,
                                WholeArrayIn pointCoordinates,
                                WholeArrayIn inputField,
                                GradientOutputs outputFields);

  using ExecutionSignature = void(CellCount, CellIndices, WorkIndex, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename FromIndexType,
            typename CellSetInType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename GradientOutType>
  VTKM_EXEC void operator()(const vtkm::IdComponent& numCells,
                            const FromIndexType& cellIds,
                            const vtkm::Id& pointId,
                            const CellSetInType& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            GradientOutType& outputGradient) const
  {
    using CellThreadIndices = vtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>;
    using ValueType = typename WholeFieldIn::ValueType;
    using CellShapeTag = typename CellSetInType::CellShapeTag;

    vtkm::Vec<ValueType, 3> gradient(ValueType(0.0));
    for (vtkm::IdComponent i = 0; i < numCells; ++i)
    {
      const vtkm::Id cellId = cellIds[i];
      CellThreadIndices cellIndices(cellId, cellId, 0, cellId, geometry);

      const CellShapeTag cellShape = cellIndices.GetCellShape();

      // compute the parametric coordinates for the current point
      const auto wCoords = this->GetValues(cellIndices, pointCoordinates);
      const auto field = this->GetValues(cellIndices, inputField);

      const vtkm::IdComponent pointIndexForCell = this->GetPointIndexForCell(cellIndices, pointId);

      this->ComputeGradient(cellShape, pointIndexForCell, wCoords, field, gradient);
    }

    if (numCells != 0)
    {
      using BaseGradientType = typename vtkm::VecTraits<ValueType>::BaseComponentType;
      const BaseGradientType invNumCells =
        static_cast<BaseGradientType>(1.) / static_cast<BaseGradientType>(numCells);

      gradient[0] = gradient[0] * invNumCells;
      gradient[1] = gradient[1] * invNumCells;
      gradient[2] = gradient[2] * invNumCells;
    }
    outputGradient = gradient;
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
    vtkm::Vec3f pCoords;
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
    const auto& topo = indices.GetIndicesIncident();
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
}
}
}

#endif
