//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_internal_TriangulateTables_h
#define vtk_m_worklet_internal_TriangulateTables_h

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>

#include <vtkm/exec/ExecutionObjectBase.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageBasic.h>

namespace vtkm
{
namespace worklet
{
namespace internal
{

typedef vtkm::cont::ArrayHandle<vtkm::IdComponent, vtkm::cont::StorageTagBasic>
  TriangulateArrayHandle;

static vtkm::IdComponent TriangleCountData[vtkm::NUMBER_OF_CELL_SHAPES] = {
  0,  //  0 = vtkm::CELL_SHAPE_EMPTY_CELL
  0,  //  1 = vtkm::CELL_SHAPE_VERTEX
  0,  //  2 = vtkm::CELL_SHAPE_POLY_VERTEX
  0,  //  3 = vtkm::CELL_SHAPE_LINE
  0,  //  4 = vtkm::CELL_SHAPE_POLY_LINE
  1,  //  5 = vtkm::CELL_SHAPE_TRIANGLE
  0,  //  6 = vtkm::CELL_SHAPE_TRIANGLE_STRIP
  -1, //  7 = vtkm::CELL_SHAPE_POLYGON
  0,  //  8 = vtkm::CELL_SHAPE_PIXEL
  2,  //  9 = vtkm::CELL_SHAPE_QUAD
  0,  // 10 = vtkm::CELL_SHAPE_TETRA
  0,  // 11 = vtkm::CELL_SHAPE_VOXEL
  0,  // 12 = vtkm::CELL_SHAPE_HEXAHEDRON
  0,  // 13 = vtkm::CELL_SHAPE_WEDGE
  0   // 14 = vtkm::CELL_SHAPE_PYRAMID
};

static vtkm::IdComponent TriangleOffsetData[vtkm::NUMBER_OF_CELL_SHAPES] = {
  -1, //  0 = vtkm::CELL_SHAPE_EMPTY_CELL
  -1, //  1 = vtkm::CELL_SHAPE_VERTEX
  -1, //  2 = vtkm::CELL_SHAPE_POLY_VERTEX
  -1, //  3 = vtkm::CELL_SHAPE_LINE
  -1, //  4 = vtkm::CELL_SHAPE_POLY_LINE
  0,  //  5 = vtkm::CELL_SHAPE_TRIANGLE
  -1, //  6 = vtkm::CELL_SHAPE_TRIANGLE_STRIP
  -1, //  7 = vtkm::CELL_SHAPE_POLYGON
  -1, //  8 = vtkm::CELL_SHAPE_PIXEL
  1,  //  9 = vtkm::CELL_SHAPE_QUAD
  -1, // 10 = vtkm::CELL_SHAPE_TETRA
  -1, // 11 = vtkm::CELL_SHAPE_VOXEL
  -1, // 12 = vtkm::CELL_SHAPE_HEXAHEDRON
  -1, // 13 = vtkm::CELL_SHAPE_WEDGE
  -1  // 14 = vtkm::CELL_SHAPE_PYRAMID
};

static vtkm::IdComponent TriangleIndexData[] = {
  // vtkm::CELL_SHAPE_TRIANGLE
  0,
  1,
  2,
  // vtkm::CELL_SHAPE_QUAD
  0,
  1,
  2,
  0,
  2,
  3
};

template <typename Device>
class TriangulateTablesExecutionObject : public vtkm::exec::ExecutionObjectBase
{
public:
  typedef typename TriangulateArrayHandle::ExecutionTypes<Device>::PortalConst PortalType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  TriangulateTablesExecutionObject() {}

  VTKM_CONT
  TriangulateTablesExecutionObject(const TriangulateArrayHandle& counts,
                                   const TriangulateArrayHandle& offsets,
                                   const TriangulateArrayHandle& indices)
    : Counts(counts.PrepareForInput(Device()))
    , Offsets(offsets.PrepareForInput(Device()))
    , Indices(indices.PrepareForInput(Device()))
  {
  }

  template <typename CellShape>
  VTKM_EXEC vtkm::IdComponent GetCount(CellShape shape, vtkm::IdComponent numPoints) const
  {
    if (shape.Id == vtkm::CELL_SHAPE_POLYGON)
    {
      return numPoints - 2;
    }
    else
    {
      return this->Counts.Get(shape.Id);
    }
  }

  template <typename CellShape>
  VTKM_EXEC vtkm::Vec<vtkm::IdComponent, 3> GetIndices(CellShape shape,
                                                       vtkm::IdComponent triangleIndex) const
  {
    vtkm::Vec<vtkm::IdComponent, 3> triIndices;
    if (shape.Id == vtkm::CELL_SHAPE_POLYGON)
    {
      triIndices[0] = 0;
      triIndices[1] = triangleIndex + 1;
      triIndices[2] = triangleIndex + 2;
    }
    else
    {
      vtkm::IdComponent offset = 3 * (this->Offsets.Get(shape.Id) + triangleIndex);
      triIndices[0] = this->Indices.Get(offset + 0);
      triIndices[1] = this->Indices.Get(offset + 1);
      triIndices[2] = this->Indices.Get(offset + 2);
    }
    return triIndices;
  }

private:
  PortalType Counts;
  PortalType Offsets;
  PortalType Indices;
};

class TriangulateTables
{
public:
  VTKM_CONT
  TriangulateTables()
    : Counts(vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::TriangleCountData,
                                          vtkm::NUMBER_OF_CELL_SHAPES))
    , Offsets(vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::TriangleOffsetData,
                                           vtkm::NUMBER_OF_CELL_SHAPES))
    , Indices(vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::TriangleIndexData, vtkm::Id(9)))
  {
  }

  template <typename Device>
  vtkm::worklet::internal::TriangulateTablesExecutionObject<Device> PrepareForInput(Device) const
  {
    return vtkm::worklet::internal::TriangulateTablesExecutionObject<Device>(
      this->Counts, this->Offsets, this->Indices);
  }

private:
  TriangulateArrayHandle Counts;
  TriangulateArrayHandle Offsets;
  TriangulateArrayHandle Indices;
};

static vtkm::IdComponent TetrahedronCountData[vtkm::NUMBER_OF_CELL_SHAPES] = {
  0, //  0 = vtkm::CELL_SHAPE_EMPTY_CELL
  0, //  1 = vtkm::CELL_SHAPE_VERTEX
  0, //  2 = vtkm::CELL_SHAPE_POLY_VERTEX
  0, //  3 = vtkm::CELL_SHAPE_LINE
  0, //  4 = vtkm::CELL_SHAPE_POLY_LINE
  0, //  5 = vtkm::CELL_SHAPE_TRIANGLE
  0, //  6 = vtkm::CELL_SHAPE_TRIANGLE_STRIP
  0, //  7 = vtkm::CELL_SHAPE_POLYGON
  0, //  8 = vtkm::CELL_SHAPE_PIXEL
  0, //  9 = vtkm::CELL_SHAPE_QUAD
  1, // 10 = vtkm::CELL_SHAPE_TETRA
  0, // 11 = vtkm::CELL_SHAPE_VOXEL
  5, // 12 = vtkm::CELL_SHAPE_HEXAHEDRON
  3, // 13 = vtkm::CELL_SHAPE_WEDGE
  2  // 14 = vtkm::CELL_SHAPE_PYRAMID
};

static vtkm::IdComponent TetrahedronOffsetData[vtkm::NUMBER_OF_CELL_SHAPES] = {
  -1, //  0 = vtkm::CELL_SHAPE_EMPTY_CELL
  -1, //  1 = vtkm::CELL_SHAPE_VERTEX
  -1, //  2 = vtkm::CELL_SHAPE_POLY_VERTEX
  -1, //  3 = vtkm::CELL_SHAPE_LINE
  -1, //  4 = vtkm::CELL_SHAPE_POLY_LINE
  -1, //  5 = vtkm::CELL_SHAPE_TRIANGLE
  -1, //  6 = vtkm::CELL_SHAPE_TRIANGLE_STRIP
  -1, //  7 = vtkm::CELL_SHAPE_POLYGON
  -1, //  8 = vtkm::CELL_SHAPE_PIXEL
  -1, //  9 = vtkm::CELL_SHAPE_QUAD
  0,  // 10 = vtkm::CELL_SHAPE_TETRA
  -1, // 11 = vtkm::CELL_SHAPE_VOXEL
  1,  // 12 = vtkm::CELL_SHAPE_HEXAHEDRON
  6,  // 13 = vtkm::CELL_SHAPE_WEDGE
  9   // 14 = vtkm::CELL_SHAPE_PYRAMID
};

static vtkm::IdComponent TetrahedronIndexData[] = {
  // vtkm::CELL_SHAPE_TETRA
  0,
  1,
  2,
  3,
  // vtkm::CELL_SHAPE_HEXAHEDRON
  0,
  1,
  3,
  4,
  1,
  4,
  5,
  6,
  1,
  4,
  6,
  3,
  1,
  3,
  6,
  2,
  3,
  6,
  7,
  4,
  // vtkm::CELL_SHAPE_WEDGE
  0,
  1,
  2,
  4,
  3,
  4,
  5,
  2,
  0,
  2,
  3,
  4,
  // vtkm::CELL_SHAPE_PYRAMID
  0,
  1,
  2,
  4,
  0,
  2,
  3,
  4
};

template <typename Device>
class TetrahedralizeTablesExecutionObject : public vtkm::exec::ExecutionObjectBase
{
public:
  typedef typename TriangulateArrayHandle::ExecutionTypes<Device>::PortalConst PortalType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  TetrahedralizeTablesExecutionObject() {}

  VTKM_CONT
  TetrahedralizeTablesExecutionObject(const TriangulateArrayHandle& counts,
                                      const TriangulateArrayHandle& offsets,
                                      const TriangulateArrayHandle& indices)
    : Counts(counts.PrepareForInput(Device()))
    , Offsets(offsets.PrepareForInput(Device()))
    , Indices(indices.PrepareForInput(Device()))
  {
  }

  template <typename CellShape>
  VTKM_EXEC vtkm::IdComponent GetCount(CellShape shape) const
  {
    return this->Counts.Get(shape.Id);
  }

  template <typename CellShape>
  VTKM_EXEC vtkm::Vec<vtkm::IdComponent, 4> GetIndices(CellShape shape,
                                                       vtkm::IdComponent tetrahedronIndex) const
  {
    vtkm::Vec<vtkm::IdComponent, 4> tetIndices;
    vtkm::IdComponent offset = 4 * (this->Offsets.Get(shape.Id) + tetrahedronIndex);
    tetIndices[0] = this->Indices.Get(offset + 0);
    tetIndices[1] = this->Indices.Get(offset + 1);
    tetIndices[2] = this->Indices.Get(offset + 2);
    tetIndices[3] = this->Indices.Get(offset + 3);
    return tetIndices;
  }

private:
  PortalType Counts;
  PortalType Offsets;
  PortalType Indices;
};

class TetrahedralizeTables
{
public:
  VTKM_CONT
  TetrahedralizeTables()
    : Counts(vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::TetrahedronCountData,
                                          vtkm::NUMBER_OF_CELL_SHAPES))
    , Offsets(vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::TetrahedronOffsetData,
                                           vtkm::NUMBER_OF_CELL_SHAPES))
    , Indices(
        vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::TetrahedronIndexData, vtkm::Id(44)))
  {
  }

  template <typename Device>
  vtkm::worklet::internal::TetrahedralizeTablesExecutionObject<Device> PrepareForInput(Device) const
  {
    return vtkm::worklet::internal::TetrahedralizeTablesExecutionObject<Device>(
      this->Counts, this->Offsets, this->Indices);
  }

private:
  TriangulateArrayHandle Counts;
  TriangulateArrayHandle Offsets;
  TriangulateArrayHandle Indices;
};
}
}
}

#endif //vtk_m_worklet_internal_TriangulateTables_h
