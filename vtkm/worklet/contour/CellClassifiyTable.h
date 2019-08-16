//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_CellClassification_h
#define vtk_m_CellClassification_h

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
namespace vtkm
{
namespace worklet
{
namespace internal
{

// clang-format off
VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent NumVerticesPerCellTable[] = {
  0, //  CELL_SHAPE_EMPTY = 0,
  0, //  CELL_SHAPE_VERTEX = 1,
  0, //  CELL_SHAPE_POLY_VERTEX = 2,
  0, //  CELL_SHAPE_LINE = 3,
  0, //  CELL_SHAPE_POLY_LINE = 4,
  3, //  CELL_SHAPE_TRIANGLE = 5,
  0, //  CELL_SHAPE_TRIANGLE_STRIP   = 6,
  0, //  CELL_SHAPE_POLYGON = 7,
  0, //  CELL_SHAPE_PIXEL = 8,
  4, //  CELL_SHAPE_QUAD = 9,
  4, //  CELL_SHAPE_TETRA = 10,
  0, //  CELL_SHAPE_VOXEL = 11,
  8, //  CELL_SHAPE_HEXAHEDRON = 12,
  6, //  CELL_SHAPE_WEDGE = 13,
  5  //  CELL_SHAPE_PYRAMID = 14,
};

VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent NumTrianglesTableOffset[] = {
  0, //  CELL_SHAPE_EMPTY = 0,
  0, //  CELL_SHAPE_VERTEX = 1,
  0, //  CELL_SHAPE_POLY_VERTEX = 2,
  0, //  CELL_SHAPE_LINE = 3,
  0, //  CELL_SHAPE_POLY_LINE = 4,
  0, //  CELL_SHAPE_TRIANGLE = 5,
  0, //  CELL_SHAPE_TRIANGLE_STRIP   = 6,
  0, //  CELL_SHAPE_POLYGON = 7,
  0, //  CELL_SHAPE_PIXEL = 8,
  0, //  CELL_SHAPE_QUAD = 9,
  0, //  CELL_SHAPE_TETRA = 10,
  0, //  CELL_SHAPE_VOXEL = 11,
  16, //  CELL_SHAPE_HEXAHEDRON = 12,
  6, //  CELL_SHAPE_WEDGE = 13,
  5  //  CELL_SHAPE_PYRAMID = 14,
};

VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent NumTriangleTable[] = {
  // CELL_SHAPE_TETRA, case 0 - 15
  0, 1, 1, 2, 1, 2, 2, 1, 1, 2, 2, 1, 2, 1, 1, 0,
  // CELL_SHAPE_HEXAHEDRON, case 0 - 255
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 2,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
  2, 3, 3, 2, 3, 4, 4, 3, 3, 4, 4, 3, 4, 5, 5, 2,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4,
  2, 3, 3, 4, 3, 4, 2, 3, 3, 4, 4, 5, 4, 5, 3, 2,
  3, 4, 4, 3, 4, 5, 3, 2, 4, 5, 5, 4, 5, 2, 4, 1,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 2, 4, 3, 4, 3, 5, 2,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4,
  3, 4, 4, 3, 4, 5, 5, 4, 4, 3, 5, 2, 5, 4, 2, 1,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 2, 3, 3, 2,
  3, 4, 4, 5, 4, 5, 5, 2, 4, 3, 5, 4, 3, 2, 4, 1,
  3, 4, 4, 5, 4, 5, 3, 4, 4, 5, 5, 2, 3, 4, 2, 1,
  2, 3, 3, 2, 3, 4, 2, 1, 3, 2, 4, 1, 2, 1, 1, 0,
  //  CELL_SHAPE_WEDGE, case 0 - 63
  //  CELL_SHAPE_PYRAMID, case 0 -31
};

class CellClassTable : public vtkm::cont::ExecutionObjectBase
{
public:
  template <typename DeviceAdapter>
  class DevicePortal
  {
  public:
    VTKM_EXEC
    vtkm::IdComponent GetNumVerticesPerCell(vtkm::Id shape) const
    {
      return this->NumVerticesPerCellPortal.Get(shape);
    }

    VTKM_EXEC
    vtkm::IdComponent GetNumTriangle(vtkm::Id shape, vtkm::IdComponent caseNumber) const
    {
      vtkm::IdComponent offset = this->NumTriangleTableOffsetPortal.Get(shape);
      return this->NumTriangleTablePortal.Get(offset + caseNumber);
    }

  private:
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<DeviceAdapter>::PortalConst
      NumVerticesPerCellPortal;
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<DeviceAdapter>::PortalConst
      NumTriangleTablePortal;
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<DeviceAdapter>::PortalConst
      NumTriangleTableOffsetPortal;

    friend class CellClassTable;
  };

  CellClassTable()
    : NumVerticesPerCellArray(vtkm::cont::make_ArrayHandle(NumVerticesPerCellTable, vtkm::NUMBER_OF_CELL_SHAPES)),
    NumTrianglesTableOffsetArray(vtkm::cont::make_ArrayHandle(NumTrianglesTableOffset,vtkm::NUMBER_OF_CELL_SHAPES)),
    NumTrianglesTableArray(vtkm::cont::make_ArrayHandle(NumTriangleTable, 272))
  {}

  template <typename DeviceAdapter>
  DevicePortal<DeviceAdapter> PrepareForExecution(DeviceAdapter)
  {
    DevicePortal<DeviceAdapter> portal;
    portal.NumVerticesPerCellPortal = this->NumVerticesPerCellArray.PrepareForInput(DeviceAdapter());
    portal.NumTriangleTableOffsetPortal = this->NumTrianglesTableOffsetArray.PrepareForInput(DeviceAdapter());
    portal.NumTriangleTablePortal = this->NumTrianglesTableArray.PrepareForInput(DeviceAdapter());
    return portal;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumVerticesPerCellArray;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumTrianglesTableOffsetArray;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumTrianglesTableArray;

};

}
}
}
#endif // vtk_m_CellClassification_h
