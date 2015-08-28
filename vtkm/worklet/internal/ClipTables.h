//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_ClipTables_h
#define vtk_m_ClipTables_h

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm {
namespace worklet {
namespace internal {

// table format:
// ncells, {{celltype, nverts, {edge/verts(>=100), ...}}, ...}, -1 padding
// values < 100 represent edges where the corresponding vertex lies
// values >= 100 reresent existing vertices of the input cell (vertex = value - 100)
static vtkm::Id ClipTablesData[] = {
  // vtkm::CELL_SHAPE_VERTEX
  0,  -1,  -1,  -1, // 0
  1,   1,   1, 100, // 1
  // vtkm::CELL_SHAPE_LINE
  0,  -1,  -1,  -1,  -1, // 0
  1,   3,   2, 100,   1, // 1
  1,   3,   2,   0, 101, // 2
  1,   3,   2, 100, 101, // 3
  // vtkm::CELL_SHAPE_TRIANGLE
  0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 0
  1,   5,   3,   0,   2, 100,   0,  -1,  -1,  -1,  -1, // 1
  1,   5,   3,   1,   0, 101,   0,  -1,  -1,  -1,  -1, // 2
  2,   5,   3,   1,   2, 100,   5,   3,   1, 100, 101, // 3
  1,   5,   3,   2,   1, 102,   0,  -1,  -1,  -1,  -1, // 4
  2,   5,   3,   0,   1, 102,   5,   3, 102, 100,   0, // 5
  2,   5,   3,   0, 101,   2,   5,   3,   2, 101, 102, // 6
  1,   5,   3, 100, 101, 102,   0,  -1,  -1,  -1,  -1, // 7
  // vtkm::CELL_SHAPE_QUAD
  0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 0
  1,   5,   3, 100,   0,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 1
  1,   5,   3, 101,   1,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 2
  1,   9,   4, 100, 101,   1,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 3
  1,   5,   3, 102,   2,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 4
  3,   5,   3, 100,   0,   3,   5,   3, 102,   2,   1,   9,   4,   0,   1,   2,   3, // 5
  1,   9,   4, 101, 102,   2,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 6
  3,   5,   3, 100, 101,   3,   5,   3, 101,   2,   3,   5,   3, 101, 102,   2,  -1, // 7
  1,   5,   3, 103,   3,   2,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 8
  1,   9,   4, 100,   0,   2, 103,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 9
  3,   5,   3, 101,   1,   0,   5,   3, 103,   3,   2,   9,   4,   0,   1,   2,   3, // 10
  3,   5,   3, 100, 101,   1,   5,   3, 100,   1,   2,   5,   3, 100,   2, 103,  -1, // 11
  1,   9,   4, 102, 103,   3,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 12
  3,   5,   3, 100,   0, 103,   5,   3,   0,   1, 103,   5,   3,   1, 102, 103,  -1, // 13
  3,   5,   3,   0, 101, 102,   5,   3,   0, 102,   3,   5,   3, 102, 103,   3,  -1, // 14
  1,   9,   4, 100, 101, 102, 103,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 15
  // vtkm::CELL_SHAPE_TETRA
  0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 0
  1,  10,   4,   0,   3,   2, 100,  -1,  -1, // 1
  1,  10,   4,   0,   1,   4, 101,  -1,  -1, // 2
  1,  13,   6, 101,   1,   4, 100,   2,   3, // 3
  1,  10,   4,   1,   2,   5, 102,  -1,  -1, // 4
  1,  13,   6, 102,   5,   1, 100,   3,   0, // 5
  1,  13,   6, 102,   2,   5, 101,   0,   4, // 6
  1,  13,   6,   3,   4,   5, 100, 101, 102, // 7
  1,  10,   4,   3,   4,   5, 103,  -1,  -1, // 8
  1,  13,   6, 103,   4,   5, 100,   0,   2, // 9
  1,  13,   6, 103,   5,   3, 101,   1,   0, // 10
  1,  13,   6, 100, 101, 103,   2,   1,   5, // 11
  1,  13,   6,   2, 102,   1,   3, 103,   4, // 12
  1,  13,   6,   0,   1,   4, 100, 102, 103, // 13
  1,  13,   6,   0,   3,   2, 101, 103, 102, // 14
  1,  10,   4, 100, 101, 102, 103,  -1,  -1  // 15
};

static vtkm::IdComponent CellEdges[] = {
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 1,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 1,  1, 2,  2, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 1,  1, 2,  3, 2,  0, 3,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 1,  1, 2,  2, 0,  0, 3,  1, 3,  2, 3,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
};

enum {
  CLIP_TABLES_SIZE = sizeof(ClipTablesData)/sizeof(vtkm::Id),
  EDGE_TABLES_SIZE = sizeof(CellEdges)/sizeof(vtkm::IdComponent)
};

static vtkm::Id ShapeToTableWidthMap[vtkm::NUMBER_OF_CELL_SHAPES] = {
  -1, // vtkm::CELL_SHAPE_EMPTY_CELL
   4, // vtkm::CELL_SHAPE_VERTEX
  -1, // vtkm::CELL_SHAPE_POLY_VERTEX
   5, // vtkm::CELL_SHAPE_LINE
  -1, // vtkm::CELL_SHAPE_POLY_LINE
  11, // vtkm::CELL_SHAPE_TRIANGLE
  -1, // vtkm::CELL_SHAPE_TRIANGLE_STRIP
  -1, // vtkm::CELL_SHAPE_POLYGON
  -1, // vtkm::CELL_SHAPE_PIXEL
  17, // vtkm::CELL_SHAPE_QUAD
   9, // vtkm::CELL_SHAPE_TETRA
  -1, // vtkm::CELL_SHAPE_VOXEL
  -1, // vtkm::CELL_SHAPE_HEXAHEDRON
  -1, // vtkm::CELL_SHAPE_WEDGE
  -1  // vtkm::CELL_SHAPE_PYRAMID
};

static vtkm::Id ShapeToTableIndexMap[vtkm::NUMBER_OF_CELL_SHAPES] = {
   -1, // vtkm::CELL_SHAPE_EMPTY_CELL
    0, // vtkm::CELL_SHAPE_VERTEX
   -1, // vtkm::CELL_SHAPE_POLY_VERTEX
    8, // vtkm::CELL_SHAPE_LINE
   -1, // vtkm::CELL_SHAPE_POLY_LINE
   28, // vtkm::CELL_SHAPE_TRIANGLE
   -1, // vtkm::CELL_SHAPE_TRIANGLE_STRIP
   -1, // vtkm::CELL_SHAPE_POLYGON
   -1, // vtkm::CELL_SHAPE_PIXEL
  116, // vtkm::CELL_SHAPE_QUAD
  388, // vtkm::CELL_SHAPE_TETRA
   -1, // vtkm::CELL_SHAPE_VOXEL
   -1, // vtkm::CELL_SHAPE_HEXAHEDRON
   -1, // vtkm::CELL_SHAPE_WEDGE
   -1  // vtkm::CELL_SHAPE_PYRAMID
};


class ClipTables
{
private:
  typedef vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
  typedef vtkm::cont::ArrayHandle<vtkm::IdComponent> IdComponentArrayHandle;

public:
  template<typename DeviceAdapter>
  class DevicePortal
  {
  public:
    VTKM_EXEC_EXPORT
    vtkm::Id GetCaseIndex(vtkm::Id shape, vtkm::Id caseId) const
    {
      return this->ShapeToTableIndexMapPortal.Get(shape) +
             (this->ShapeToTableWidthMapPortal.Get(shape) * caseId);
    }

    VTKM_EXEC_EXPORT
    vtkm::Id ValueAt(vtkm::Id idx) const
    {
      return this->ClipTablesPortal.Get(idx);
    }

    VTKM_EXEC_EXPORT
    vtkm::Vec<vtkm::IdComponent, 2> GetEdge(vtkm::Id shape, vtkm::Id edgeId) const
    {
      vtkm::Id index = (shape * 24) + (edgeId * 2);
      return vtkm::make_Vec(this->EdgeTablesPortal.Get(index),
                            this->EdgeTablesPortal.Get(index + 1));
    }

  private:
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst
        IdPortalConst;
    typedef typename IdComponentArrayHandle::ExecutionTypes<DeviceAdapter>::
        PortalConst IdComponentPortalConst;

    IdPortalConst ClipTablesPortal;
    IdPortalConst ShapeToTableWidthMapPortal;
    IdPortalConst ShapeToTableIndexMapPortal;
    IdComponentPortalConst EdgeTablesPortal;

    friend class ClipTables;
  };

  ClipTables()
    : ClipTablesArray(vtkm::cont::make_ArrayHandle(ClipTablesData, CLIP_TABLES_SIZE)),
      ShapeToTableWidthMapArray(vtkm::cont::make_ArrayHandle(
         ShapeToTableWidthMap, vtkm::NUMBER_OF_CELL_SHAPES)),
      ShapeToTableIndexMapArray(vtkm::cont::make_ArrayHandle(
         ShapeToTableIndexMap, vtkm::NUMBER_OF_CELL_SHAPES)),
      EdgeTablesArray(vtkm::cont::make_ArrayHandle(CellEdges, EDGE_TABLES_SIZE))
  {
  }

  template<typename DeviceAdapter>
  DevicePortal<DeviceAdapter> GetDevicePortal(DeviceAdapter)
  {
    DevicePortal<DeviceAdapter> portal;
    portal.ClipTablesPortal = this->ClipTablesArray.PrepareForInput(DeviceAdapter());
    portal.ShapeToTableWidthMapPortal =
        this->ShapeToTableWidthMapArray.PrepareForInput(DeviceAdapter());
    portal.ShapeToTableIndexMapPortal =
        this->ShapeToTableIndexMapArray.PrepareForInput(DeviceAdapter());
    portal.EdgeTablesPortal = this->EdgeTablesArray.PrepareForInput(DeviceAdapter());

    return portal;
  }

private:
  IdArrayHandle ClipTablesArray;
  IdArrayHandle ShapeToTableWidthMapArray;
  IdArrayHandle ShapeToTableIndexMapArray;
  IdComponentArrayHandle EdgeTablesArray;
};

}
}
} // namespace vtkm::worklet::internal

#endif // vtk_m_ClipTables_h
