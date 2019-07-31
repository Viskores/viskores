//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_internal_TriangulateTables_h
#define vtk_m_worklet_internal_TriangulateTables_h

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>

#include <vtkm/cont/ExecutionObjectBase.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageBasic.h>

namespace vtkm
{
namespace worklet
{
namespace internal
{

using TriangulateArrayHandle =
  vtkm::cont::ArrayHandle<vtkm::IdComponent, vtkm::cont::StorageTagBasic>;

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

template <typename DeviceAdapter>
class TriangulateTablesExecutionObject
{
public:
  using PortalType = typename TriangulateArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst;
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  TriangulateTablesExecutionObject() {}

  VTKM_CONT
  TriangulateTablesExecutionObject(const TriangulateArrayHandle& counts,
                                   const TriangulateArrayHandle& offsets,
                                   const TriangulateArrayHandle& indices)
    : Counts(counts.PrepareForInput(DeviceAdapter()))
    , Offsets(offsets.PrepareForInput(DeviceAdapter()))
    , Indices(indices.PrepareForInput(DeviceAdapter()))
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
  VTKM_EXEC vtkm::IdComponent3 GetIndices(CellShape shape, vtkm::IdComponent triangleIndex) const
  {
    vtkm::IdComponent3 triIndices;
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

class TriangulateTablesExecutionObjectFactory : public vtkm::cont::ExecutionObjectBase
{
public:
  template <typename Device>
  VTKM_CONT TriangulateTablesExecutionObject<Device> PrepareForExecution(Device) const
  {
    if (BasicImpl)
    {
      return TriangulateTablesExecutionObject<Device>();
    }
    return TriangulateTablesExecutionObject<Device>(this->Counts, this->Offsets, this->Indices);
  }
  VTKM_CONT
  TriangulateTablesExecutionObjectFactory()
    : BasicImpl(true)
  {
  }

  VTKM_CONT
  TriangulateTablesExecutionObjectFactory(const TriangulateArrayHandle& counts,
                                          const TriangulateArrayHandle& offsets,
                                          const TriangulateArrayHandle& indices)
    : BasicImpl(false)
    , Counts(counts)
    , Offsets(offsets)
    , Indices(indices)
  {
  }

private:
  bool BasicImpl;
  TriangulateArrayHandle Counts;
  TriangulateArrayHandle Offsets;
  TriangulateArrayHandle Indices;
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

  vtkm::worklet::internal::TriangulateTablesExecutionObjectFactory PrepareForInput() const
  {
    return vtkm::worklet::internal::TriangulateTablesExecutionObjectFactory(
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

template <typename DeviceAdapter>
class TetrahedralizeTablesExecutionObject
{
public:
  using PortalType = typename TriangulateArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst;
  template <typename Device>
  VTKM_CONT TetrahedralizeTablesExecutionObject PrepareForExecution(Device) const
  {
    return *this;
  }
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  TetrahedralizeTablesExecutionObject() {}

  VTKM_CONT
  TetrahedralizeTablesExecutionObject(const TriangulateArrayHandle& counts,
                                      const TriangulateArrayHandle& offsets,
                                      const TriangulateArrayHandle& indices)
    : Counts(counts.PrepareForInput(DeviceAdapter()))
    , Offsets(offsets.PrepareForInput(DeviceAdapter()))
    , Indices(indices.PrepareForInput(DeviceAdapter()))
  {
  }

  template <typename CellShape>
  VTKM_EXEC vtkm::IdComponent GetCount(CellShape shape) const
  {
    return this->Counts.Get(shape.Id);
  }

  template <typename CellShape>
  VTKM_EXEC vtkm::IdComponent4 GetIndices(CellShape shape, vtkm::IdComponent tetrahedronIndex) const
  {
    vtkm::IdComponent4 tetIndices;
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

class TetrahedralizeTablesExecutionObjectFactory : public vtkm::cont::ExecutionObjectBase
{
public:
  template <typename Device>
  VTKM_CONT TetrahedralizeTablesExecutionObject<Device> PrepareForExecution(Device) const
  {
    if (BasicImpl)
    {
      return TetrahedralizeTablesExecutionObject<Device>();
    }
    return TetrahedralizeTablesExecutionObject<Device>(this->Counts, this->Offsets, this->Indices);
  }

  VTKM_CONT
  TetrahedralizeTablesExecutionObjectFactory()
    : BasicImpl(true)
  {
  }

  VTKM_CONT
  TetrahedralizeTablesExecutionObjectFactory(const TriangulateArrayHandle& counts,
                                             const TriangulateArrayHandle& offsets,
                                             const TriangulateArrayHandle& indices)
    : BasicImpl(false)
    , Counts(counts)
    , Offsets(offsets)
    , Indices(indices)
  {
  }

private:
  bool BasicImpl;
  TriangulateArrayHandle Counts;
  TriangulateArrayHandle Offsets;
  TriangulateArrayHandle Indices;
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

  vtkm::worklet::internal::TetrahedralizeTablesExecutionObjectFactory PrepareForInput() const
  {
    return vtkm::worklet::internal::TetrahedralizeTablesExecutionObjectFactory(
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
