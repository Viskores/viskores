//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_Probe_h
#define vtk_m_worklet_Probe_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellLocatorHelper.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/VecFromPortalPermute.h>

namespace vtkm
{
namespace worklet
{

class Probe
{
  //============================================================================
private:
  template <typename CellSetType>
  static vtkm::ListTagBase<CellSetType> GetCellSetListTag(const CellSetType&)
  {
    return {};
  }

  template <typename CellSetList>
  static CellSetList GetCellSetListTag(const vtkm::cont::DynamicCellSetBase<CellSetList>&)
  {
    return {};
  }

  template <typename CellSetType, typename PointsType, typename PointsStorage, typename Device>
  void RunImpl(const CellSetType& cells,
               const vtkm::cont::CoordinateSystem& coords,
               const vtkm::cont::ArrayHandle<PointsType, PointsStorage>& points,
               Device device)
  {
    this->InputCellSet = vtkm::cont::DynamicCellSet(cells);

    vtkm::cont::CellLocatorHelper locator;
    locator.SetCellSet(this->InputCellSet);
    locator.SetCoordinates(coords);
    locator.Build(device);
    locator.FindCells(
      points, this->CellIds, this->ParametricCoordinates, device, GetCellSetListTag(cells));
  }

  //============================================================================
public:
  class ProbeUniformPoints : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<Vec3> coords,
                                  WholeArrayIn<Vec3> points,
                                  WholeArrayOut<IdType> cellIds,
                                  WholeArrayOut<Vec3> parametricCoords);
    using ExecutionSignature = void(InputIndex, CellShape, _2, _3, _4, _5);
    using InputDomain = _1;

    template <typename CellShapeTag,
              typename CoordsVecType,
              typename UniformPoints,
              typename CellIdsType,
              typename ParametricCoordsType>
    VTKM_EXEC void operator()(vtkm::Id cellId,
                              CellShapeTag cellShape,
                              const CoordsVecType& cellPoints,
                              const UniformPoints& points,
                              CellIdsType& cellIds,
                              ParametricCoordsType& pcoords) const
    {
      // Compute cell bounds
      using CoordsType = typename vtkm::VecTraits<CoordsVecType>::ComponentType;
      auto numPoints = vtkm::VecTraits<CoordsVecType>::GetNumberOfComponents(cellPoints);

      CoordsType cbmin = cellPoints[0], cbmax = cellPoints[0];
      for (vtkm::IdComponent i = 1; i < numPoints; ++i)
      {
        cbmin = vtkm::Min(cbmin, cellPoints[i]);
        cbmax = vtkm::Max(cbmax, cellPoints[i]);
      }

      // Compute points inside cell bounds
      auto portal = points.GetPortal();
      auto minp =
        static_cast<vtkm::Id3>(vtkm::Ceil((cbmin - portal.GetOrigin()) / portal.GetSpacing()));
      auto maxp =
        static_cast<vtkm::Id3>(vtkm::Floor((cbmax - portal.GetOrigin()) / portal.GetSpacing()));

      // clamp
      minp = vtkm::Max(minp, vtkm::Id3(0));
      maxp = vtkm::Min(maxp, portal.GetDimensions() - vtkm::Id3(1));

      for (vtkm::Id k = minp[2]; k <= maxp[2]; ++k)
      {
        for (vtkm::Id j = minp[1]; j <= maxp[1]; ++j)
        {
          for (vtkm::Id i = minp[0]; i <= maxp[0]; ++i)
          {
            auto pt = portal.Get(vtkm::Id3(i, j, k));
            bool success = false;
            auto pc = vtkm::exec::WorldCoordinatesToParametricCoordinates(
              cellPoints, pt, cellShape, success, *this);
            if (success && vtkm::exec::CellInside(pc, cellShape))
            {
              auto pointId = i + portal.GetDimensions()[0] * (j + portal.GetDimensions()[1] * k);
              cellIds.Set(pointId, cellId);
              pcoords.Set(pointId, pc);
            }
          }
        }
      }
    }
  };

private:
  template <typename CellSetType, typename Device>
  void RunImpl(const CellSetType& cells,
               const vtkm::cont::CoordinateSystem& coords,
               const vtkm::cont::ArrayHandleUniformPointCoordinates::Superclass& points,
               Device)
  {
    this->InputCellSet = vtkm::cont::DynamicCellSet(cells);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Copy(
      vtkm::cont::make_ArrayHandleConstant(vtkm::Id(-1), points.GetNumberOfValues()),
      this->CellIds);
    this->ParametricCoordinates.Allocate(points.GetNumberOfValues());

    vtkm::worklet::DispatcherMapTopology<ProbeUniformPoints, Device>().Invoke(
      cells, coords, points, this->CellIds, this->ParametricCoordinates);
  }

  //============================================================================
  struct RunImplCaller
  {
    template <typename PointsArrayType, typename CellSetType, typename Device>
    void operator()(const PointsArrayType& points,
                    Probe& worklet,
                    const CellSetType& cells,
                    const vtkm::cont::CoordinateSystem& coords,
                    Device device) const
    {
      worklet.RunImpl(cells, coords, points, device);
    }
  };

public:
  template <typename CellSetType, typename PointsArrayType, typename Device>
  void Run(const CellSetType& cells,
           const vtkm::cont::CoordinateSystem& coords,
           const PointsArrayType& points,
           Device device)
  {
    vtkm::cont::CastAndCall(points, RunImplCaller(), *this, cells, coords, device);
  }

  //============================================================================
  class InterpolatePointField : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn<IdType> cellIds,
                                  FieldIn<Vec3> parametricCoords,
                                  WholeCellSetIn<> inputCells,
                                  WholeArrayIn<> inputField,
                                  FieldOut<> result);
    using ExecutionSignature = void(_1, _2, _3, _4, _5);

    template <typename ParametricCoordType, typename CellSetType, typename InputFieldPortalType>
    VTKM_EXEC void operator()(vtkm::Id cellId,
                              const ParametricCoordType& pc,
                              const CellSetType& cells,
                              const InputFieldPortalType& in,
                              typename InputFieldPortalType::ValueType& out) const
    {
      if (cellId != -1)
      {
        auto indices = cells.GetIndices(cellId);
        auto pointVals = vtkm::make_VecFromPortalPermute(&indices, in);
        out = vtkm::exec::CellInterpolate(pointVals, pc, cells.GetCellShape(cellId), *this);
      }
    }
  };

  /// Intepolate the input point field data at the points of the geometry
  template <typename T,
            typename Storage,
            typename Device,
            typename InputCellSetTypeList = VTKM_DEFAULT_CELL_SET_LIST_TAG>
  vtkm::cont::ArrayHandle<T> ProcessPointField(
    const vtkm::cont::ArrayHandle<T, Storage>& field,
    Device,
    InputCellSetTypeList icsTypes = InputCellSetTypeList()) const
  {
    vtkm::cont::ArrayHandle<T> result;
    vtkm::worklet::DispatcherMapField<InterpolatePointField, Device>().Invoke(
      this->CellIds,
      this->ParametricCoordinates,
      this->InputCellSet.ResetCellSetList(icsTypes),
      field,
      result);

    return result;
  }

  //============================================================================
  class MapCellField : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn<IdType> cellIds,
                                  WholeArrayIn<> inputField,
                                  FieldOut<> result);
    using ExecutionSignature = void(_1, _2, _3);

    template <typename InputFieldPortalType>
    VTKM_EXEC void operator()(vtkm::Id cellId,
                              const InputFieldPortalType& in,
                              typename InputFieldPortalType::ValueType& out) const
    {
      if (cellId != -1)
      {
        out = in.Get(cellId);
      }
    }
  };

  /// Map the input cell field data to the points of the geometry. Each point gets the value
  /// associated with its containing cell. For points that fall on cell edges, the containing
  /// cell is chosen arbitrarily.
  ///
  template <typename T, typename Storage, typename Device>
  vtkm::cont::ArrayHandle<T> ProcessCellField(const vtkm::cont::ArrayHandle<T, Storage>& field,
                                              Device) const
  {
    vtkm::cont::ArrayHandle<T> result;
    vtkm::worklet::DispatcherMapField<MapCellField, Device>().Invoke(this->CellIds, field, result);

    return result;
  }

  //============================================================================
  struct HiddenPointsWorklet : public WorkletMapField
  {
    using ControlSignature = void(FieldIn<IdType> cellids,
                                  FieldOut<vtkm::ListTagBase<vtkm::UInt8>> hfield);
    using ExecutionSignature = _2(_1);

    VTKM_EXEC vtkm::UInt8 operator()(vtkm::Id cellId) const { return (cellId == -1) ? HIDDEN : 0; }
  };

  /// Get an array of flags marking the invalid points (points that do not fall inside any of
  /// the cells of the input). The flag value is the same as the HIDDEN flag in VTK and VISIT.
  ///
  template <typename DeviceAdapter>
  vtkm::cont::ArrayHandle<vtkm::UInt8> GetHiddenPointsField(DeviceAdapter) const
  {
    vtkm::cont::ArrayHandle<vtkm::UInt8> field;
    vtkm::worklet::DispatcherMapField<HiddenPointsWorklet, DeviceAdapter>().Invoke(this->CellIds,
                                                                                   field);
    return field;
  }

  //============================================================================
  struct HiddenCellsWorklet : public WorkletMapPointToCell
  {
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<IdType> cellids,
                                  FieldOutCell<vtkm::ListTagBase<vtkm::UInt8>>);
    using ExecutionSignature = _3(_2, PointCount);

    template <typename CellIdsVecType>
    VTKM_EXEC vtkm::UInt8 operator()(const CellIdsVecType& cellIds,
                                     vtkm::IdComponent numPoints) const
    {
      for (vtkm::IdComponent i = 0; i < numPoints; ++i)
      {
        if (cellIds[i] == -1)
        {
          return HIDDEN;
        }
      }
      return 0;
    }
  };

  /// Get an array of flags marking the invalid cells. Invalid cells are the cells with at least
  /// one invalid point. The flag value is the same as the HIDDEN flag in VTK and VISIT.
  ///
  template <typename CellSetType, typename DeviceAdapter>
  vtkm::cont::ArrayHandle<vtkm::UInt8> GetHiddenCellsField(CellSetType cellset, DeviceAdapter) const
  {
    vtkm::cont::ArrayHandle<vtkm::UInt8> field;
    vtkm::worklet::DispatcherMapTopology<HiddenCellsWorklet, DeviceAdapter>().Invoke(
      cellset, this->CellIds, field);
    return field;
  }

  //============================================================================
private:
  static constexpr vtkm::UInt8 HIDDEN = 2; // from vtk

  vtkm::cont::ArrayHandle<vtkm::Id> CellIds;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> ParametricCoordinates;
  vtkm::cont::DynamicCellSet InputCellSet;
};
}
} // vtkm::worklet

#endif // vtk_m_worklet_Probe_h
