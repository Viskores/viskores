//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_CellLocatorHelper_h
#define vtk_m_cont_CellLocatorHelper_h

#include <vtkm/cont/CellLocatorTwoLevelUniformGrid.h>
#include <vtkm/exec/ParametricCoordinates.h>

namespace vtkm
{
namespace cont
{

class CellLocatorHelper
{
private:
  using StructuredCellSetList = vtkm::ListTagBase<vtkm::cont::CellSetStructured<1>,
                                                  vtkm::cont::CellSetStructured<2>,
                                                  vtkm::cont::CellSetStructured<3>>;

  VTKM_CONT static bool IsUniformGrid(const vtkm::cont::DynamicCellSet& cellset,
                                      const vtkm::cont::CoordinateSystem& coordinates)
  {
    return coordinates.GetData().IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>() &&
      (cellset.IsType<vtkm::cont::CellSetStructured<1>>() ||
       cellset.IsType<vtkm::cont::CellSetStructured<2>>() ||
       cellset.IsType<vtkm::cont::CellSetStructured<3>>());
  }

public:
  void SetCellSet(const vtkm::cont::DynamicCellSet& cellset) { this->CellSet = cellset; }
  const vtkm::cont::DynamicCellSet& GetCellSet() const { return this->CellSet; }

  void SetCoordinates(const vtkm::cont::CoordinateSystem& coords) { this->Coordinates = coords; }
  const vtkm::cont::CoordinateSystem& GetCoordinates() const { return this->Coordinates; }

  /// Builds the cell locator lookup structure
  ///
  template <typename DeviceAdapter, typename CellSetList = VTKM_DEFAULT_CELL_SET_LIST_TAG>
  void Build(DeviceAdapter device, CellSetList cellSetTypes = CellSetList())
  {
    if (IsUniformGrid(this->CellSet, this->Coordinates))
    {
      // nothing to build for uniform grid
    }
    else
    {
      this->Locator.SetCellSet(this->CellSet);
      this->Locator.SetCoordinates(this->Coordinates);
      this->Locator.Build(device, cellSetTypes);
    }
  }

  class FindCellWorklet : public vtkm::worklet::WorkletMapField
  {
  private:
    template <vtkm::IdComponent DIM>
    using ConnectivityType = vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                                                vtkm::TopologyElementTagCell,
                                                                DIM>;

  public:
    using ControlSignature = void(FieldIn<Vec3> points,
                                  WholeCellSetIn<> cellSet,
                                  WholeArrayIn<Vec3> coordinates,
                                  FieldOut<IdType> cellIds,
                                  FieldOut<Vec3> parametricCoordinates);
    using ExecutionSignature = void(_1, _2, _3, _4, _5);

    template <typename CoordsPortalType, vtkm::IdComponent DIM>
    VTKM_EXEC void operator()(const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                              const ConnectivityType<DIM>& cellset,
                              const CoordsPortalType& coordinates,
                              vtkm::Id& cellId,
                              vtkm::Vec<vtkm::FloatDefault, 3>& pc) const
    {
      auto coords = coordinates.GetPortal();
      static_assert(
        std::is_same<decltype(coords), vtkm::internal::ArrayPortalUniformPointCoordinates>::value,
        "expecting ArrayHandleUniformPointCoordinates for coordinates");

      auto cellId3 = static_cast<vtkm::Id3>((point - coords.GetOrigin()) / coords.GetSpacing());
      auto cellDim = vtkm::Max(vtkm::Id3(1), coords.GetDimensions() - vtkm::Id3(1));
      if (cellId3[0] < 0 || cellId3[0] >= cellDim[0] || cellId3[1] < 0 ||
          cellId3[1] >= cellDim[1] || cellId3[2] < 0 || cellId3[2] >= cellDim[2])
      {
        cellId = -1;
      }
      else
      {
        cellId = cellId3[0] + cellDim[0] * (cellId3[1] + cellDim[1] * cellId3[2]);
        auto cellPoints =
          vtkm::VecAxisAlignedPointCoordinates<DIM>(coords.Get(cellId3), coords.GetSpacing());
        bool success;
        pc = vtkm::exec::WorldCoordinatesToParametricCoordinates(
          cellPoints, point, cellset.GetCellShape(cellId), success, *this);
        VTKM_ASSERT(success);
      }
    }
  };

  /// Finds the containing cells for the given array of points. Returns the cell ids
  /// in the `cellIds` arrays. If a cell could not be found due to the point being
  /// outside all the cells or due to numerical errors, the cell id is set to -1.
  /// Parametric coordinates of the point inside the cell is returned in the
  /// `parametricCoords` array.
  ///
  template <typename PointComponentType,
            typename PointStorageType,
            typename DeviceAdapter,
            typename CellSetList = VTKM_DEFAULT_CELL_SET_LIST_TAG>
  void FindCells(
    const vtkm::cont::ArrayHandle<vtkm::Vec<PointComponentType, 3>, PointStorageType>& points,
    vtkm::cont::ArrayHandle<vtkm::Id>& cellIds,
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& parametricCoords,
    DeviceAdapter device,
    CellSetList cellSetTypes = CellSetList()) const
  {
    if (IsUniformGrid(this->CellSet, this->Coordinates))
    {
      auto coordinates =
        this->Coordinates.GetData().Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      auto cellset = this->CellSet.ResetCellSetList(StructuredCellSetList());
      vtkm::worklet::DispatcherMapField<FindCellWorklet, DeviceAdapter>().Invoke(
        points, cellset, coordinates, cellIds, parametricCoords);
    }
    else
    {
      this->Locator.FindCells(points, cellIds, parametricCoords, device, cellSetTypes);
    }
  }

private:
  vtkm::cont::DynamicCellSet CellSet;
  vtkm::cont::CoordinateSystem Coordinates;
  vtkm::cont::CellLocatorTwoLevelUniformGrid Locator;
};
}
} // vtkm::cont

#endif // vtk_m_cont_CellLocatorHelper_h
