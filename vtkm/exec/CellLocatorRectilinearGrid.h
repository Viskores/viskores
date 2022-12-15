//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_exec_celllocatorrectilineargrid_h
#define vtkm_exec_celllocatorrectilineargrid_h

#include <vtkm/Bounds.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/VecFromPortalPermute.h>

#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/CellSetStructured.h>

#include <vtkm/exec/CellInside.h>
#include <vtkm/exec/ConnectivityStructured.h>
#include <vtkm/exec/ParametricCoordinates.h>

namespace vtkm
{

namespace exec
{

class VTKM_ALWAYS_EXPORT CellLocatorRectilinearGrid
{
private:
  using AxisHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearType =
    vtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using AxisPortalType = typename AxisHandle::ReadPortalType;
  using RectilinearPortalType = typename RectilinearType::ReadPortalType;

  VTKM_CONT static vtkm::Id3&& ToId3(vtkm::Id3&& src) { return std::move(src); }
  VTKM_CONT static vtkm::Id3 ToId3(vtkm::Id2&& src) { return vtkm::Id3(src[0], src[1], 1); }
  VTKM_CONT static vtkm::Id3 ToId3(vtkm::Id&& src) { return vtkm::Id3(src, 1, 1); }

public:
  struct LastCell
  {
  };

  template <vtkm::IdComponent dimensions>
  VTKM_CONT CellLocatorRectilinearGrid(const vtkm::Id planeSize,
                                       const vtkm::Id rowSize,
                                       const vtkm::cont::CellSetStructured<dimensions>& cellSet,
                                       const RectilinearType& coords,
                                       vtkm::cont::DeviceAdapterId device,
                                       vtkm::cont::Token& token)
    : PlaneSize(planeSize)
    , RowSize(rowSize)
    , PointDimensions(ToId3(cellSet.GetPointDimensions()))
    , Dimensions(dimensions)
  {
    auto coordsContPortal = coords.ReadPortal();
    RectilinearPortalType coordsExecPortal = coords.PrepareForInput(device, token);
    this->AxisPortals[0] = coordsExecPortal.GetFirstPortal();
    this->MinPoint[0] = coordsContPortal.GetFirstPortal().Get(0);
    this->MaxPoint[0] = coordsContPortal.GetFirstPortal().Get(this->PointDimensions[0] - 1);

    this->AxisPortals[1] = coordsExecPortal.GetSecondPortal();
    this->MinPoint[1] = coordsContPortal.GetSecondPortal().Get(0);
    this->MaxPoint[1] = coordsContPortal.GetSecondPortal().Get(this->PointDimensions[1] - 1);
    if (dimensions == 3)
    {
      this->AxisPortals[2] = coordsExecPortal.GetThirdPortal();
      this->MinPoint[2] = coordsContPortal.GetThirdPortal().Get(0);
      this->MaxPoint[2] = coordsContPortal.GetThirdPortal().Get(this->PointDimensions[2] - 1);
    }
  }

  VTKM_EXEC
  inline bool IsInside(const vtkm::Vec3f& point) const
  {
    bool inside = true;
    if (point[0] < this->MinPoint[0] || point[0] > this->MaxPoint[0])
      inside = false;
    if (point[1] < this->MinPoint[1] || point[1] > this->MaxPoint[1])
      inside = false;
    if (this->Dimensions == 3)
    {
      if (point[2] < this->MinPoint[2] || point[2] > this->MaxPoint[2])
        inside = false;
    }
    return inside;
  }

  VTKM_EXEC
  vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                           vtkm::Id& cellId,
                           vtkm::Vec3f& parametric,
                           LastCell& vtkmNotUsed(lastCell)) const
  {
    return this->FindCell(point, cellId, parametric);
  }

  VTKM_EXEC
  vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                           vtkm::Id& cellId,
                           vtkm::Vec3f& parametric) const
  {
    if (!this->IsInside(point))
    {
      cellId = -1;
      return vtkm::ErrorCode::CellNotFound;
    }

    // Get the Cell Id from the point.
    vtkm::Id3 logicalCell(0, 0, 0);
    for (vtkm::Int32 dim = 0; dim < this->Dimensions; ++dim)
    {
      //
      // When searching for points, we consider the max value of the cell
      // to be apart of the next cell. If the point falls on the boundary of the
      // data set, then it is technically inside a cell. This checks for that case
      //
      if (point[dim] == MaxPoint[dim])
      {
        logicalCell[dim] = this->PointDimensions[dim] - 2;
        parametric[dim] = static_cast<vtkm::FloatDefault>(1);
        continue;
      }

      vtkm::Id minIndex = 0;
      vtkm::Id maxIndex = this->PointDimensions[dim] - 1;
      vtkm::FloatDefault minVal;
      vtkm::FloatDefault maxVal;
      minVal = this->AxisPortals[dim].Get(minIndex);
      maxVal = this->AxisPortals[dim].Get(maxIndex);
      while (maxIndex > minIndex + 1)
      {
        vtkm::Id midIndex = (minIndex + maxIndex) / 2;
        vtkm::FloatDefault midVal = this->AxisPortals[dim].Get(midIndex);
        if (point[dim] <= midVal)
        {
          maxIndex = midIndex;
          maxVal = midVal;
        }
        else
        {
          minIndex = midIndex;
          minVal = midVal;
        }
      }
      logicalCell[dim] = minIndex;
      parametric[dim] = (point[dim] - minVal) / (maxVal - minVal);
    }
    // Get the actual cellId, from the logical cell index of the cell
    cellId = logicalCell[2] * this->PlaneSize + logicalCell[1] * this->RowSize + logicalCell[0];

    return vtkm::ErrorCode::Success;
  }

private:
  vtkm::Id PlaneSize;
  vtkm::Id RowSize;

  AxisPortalType AxisPortals[3];
  vtkm::Id3 PointDimensions;
  vtkm::Vec3f MinPoint;
  vtkm::Vec3f MaxPoint;
  vtkm::Id Dimensions;
};
} //namespace exec
} //namespace vtkm

#endif //vtkm_exec_celllocatorrectilineargrid_h
