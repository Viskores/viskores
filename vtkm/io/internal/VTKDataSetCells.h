//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_internal_VTKDataSetCells_h
#define vtk_m_io_internal_VTKDataSetCells_h

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/io/ErrorIO.h>

#include <algorithm>
#include <vector>

namespace vtkm
{
namespace io
{
namespace internal
{

enum UnsupportedVTKCells
{
  CELL_SHAPE_POLY_VERTEX = 2,
  CELL_SHAPE_POLY_LINE = 4,
  CELL_SHAPE_TRIANGLE_STRIP = 6,
  CELL_SHAPE_PIXEL = 8,
  CELL_SHAPE_VOXEL = 11
};

inline void FixupCellSet(vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
                         vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
                         vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                         vtkm::cont::ArrayHandle<vtkm::Id>& permutation)
{
  std::vector<vtkm::Id> newConnectivity;
  std::vector<vtkm::IdComponent> newNumIndices;
  std::vector<vtkm::UInt8> newShapes;
  std::vector<vtkm::Id> permutationVec;

  vtkm::Id connIdx = 0;
  for (vtkm::Id i = 0; i < shapes.GetNumberOfValues(); ++i)
  {
    vtkm::UInt8 shape = shapes.GetPortalConstControl().Get(i);
    vtkm::IdComponent numInds = numIndices.GetPortalConstControl().Get(i);
    auto connPortal = connectivity.GetPortalConstControl();
    switch (shape)
    {
      case vtkm::CELL_SHAPE_VERTEX:
      case vtkm::CELL_SHAPE_LINE:
      case vtkm::CELL_SHAPE_TRIANGLE:
      case vtkm::CELL_SHAPE_QUAD:
      case vtkm::CELL_SHAPE_TETRA:
      case vtkm::CELL_SHAPE_HEXAHEDRON:
      case vtkm::CELL_SHAPE_WEDGE:
      case vtkm::CELL_SHAPE_PYRAMID:
      {
        newShapes.push_back(shape);
        newNumIndices.push_back(numInds);
        for (vtkm::IdComponent j = 0; j < numInds; ++j)
        {
          newConnectivity.push_back(connPortal.Get(connIdx++));
        }
        permutationVec.push_back(i);
        break;
      }
      case vtkm::CELL_SHAPE_POLYGON:
      {
        vtkm::IdComponent numVerts = numInds;
        vtkm::UInt8 newShape = vtkm::CELL_SHAPE_POLYGON;
        if (numVerts == 3)
        {
          newShape = vtkm::CELL_SHAPE_TRIANGLE;
        }
        else if (numVerts == 4)
        {
          newShape = vtkm::CELL_SHAPE_QUAD;
        }
        newShapes.push_back(newShape);
        newNumIndices.push_back(numVerts);
        for (vtkm::IdComponent j = 0; j < numVerts; ++j)
        {
          newConnectivity.push_back(connPortal.Get(connIdx++));
        }
        permutationVec.push_back(i);
        break;
      }
      case CELL_SHAPE_POLY_VERTEX:
      {
        vtkm::IdComponent numVerts = numInds;
        for (vtkm::IdComponent j = 0; j < numVerts; ++j)
        {
          newShapes.push_back(vtkm::CELL_SHAPE_VERTEX);
          newNumIndices.push_back(1);
          newConnectivity.push_back(connPortal.Get(connIdx));
          permutationVec.push_back(i);
          ++connIdx;
        }
        break;
      }
      case CELL_SHAPE_POLY_LINE:
      {
        vtkm::IdComponent numLines = numInds - 1;
        for (vtkm::IdComponent j = 0; j < numLines; ++j)
        {
          newShapes.push_back(vtkm::CELL_SHAPE_LINE);
          newNumIndices.push_back(2);
          newConnectivity.push_back(connPortal.Get(connIdx));
          newConnectivity.push_back(connPortal.Get(connIdx + 1));
          permutationVec.push_back(i);
          ++connIdx;
        }
        connIdx += 1;
        break;
      }
      case CELL_SHAPE_TRIANGLE_STRIP:
      {
        vtkm::IdComponent numTris = numInds - 2;
        for (vtkm::IdComponent j = 0; j < numTris; ++j)
        {
          newShapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
          newNumIndices.push_back(3);
          if (j % 2)
          {
            newConnectivity.push_back(connPortal.Get(connIdx));
            newConnectivity.push_back(connPortal.Get(connIdx + 1));
            newConnectivity.push_back(connPortal.Get(connIdx + 2));
          }
          else
          {
            newConnectivity.push_back(connPortal.Get(connIdx + 2));
            newConnectivity.push_back(connPortal.Get(connIdx + 1));
            newConnectivity.push_back(connPortal.Get(connIdx));
          }
          permutationVec.push_back(i);
          ++connIdx;
        }
        connIdx += 2;
        break;
      }
      case CELL_SHAPE_PIXEL:
      {
        newShapes.push_back(vtkm::CELL_SHAPE_QUAD);
        newNumIndices.push_back(numInds);
        newConnectivity.push_back(connPortal.Get(connIdx + 0));
        newConnectivity.push_back(connPortal.Get(connIdx + 1));
        newConnectivity.push_back(connPortal.Get(connIdx + 3));
        newConnectivity.push_back(connPortal.Get(connIdx + 2));
        permutationVec.push_back(i);
        connIdx += 4;
        break;
      }
      case CELL_SHAPE_VOXEL:
      {
        newShapes.push_back(vtkm::CELL_SHAPE_HEXAHEDRON);
        newNumIndices.push_back(numInds);
        newConnectivity.push_back(connPortal.Get(connIdx + 0));
        newConnectivity.push_back(connPortal.Get(connIdx + 1));
        newConnectivity.push_back(connPortal.Get(connIdx + 3));
        newConnectivity.push_back(connPortal.Get(connIdx + 2));
        newConnectivity.push_back(connPortal.Get(connIdx + 4));
        newConnectivity.push_back(connPortal.Get(connIdx + 5));
        newConnectivity.push_back(connPortal.Get(connIdx + 7));
        newConnectivity.push_back(connPortal.Get(connIdx + 6));
        permutationVec.push_back(i);
        connIdx += 6;
        break;
      }
      default:
      {
        throw vtkm::io::ErrorIO("Encountered unsupported cell type");
      }
    }
  }

  if (newShapes.size() == static_cast<std::size_t>(shapes.GetNumberOfValues()))
  {
    permutationVec.clear();
  }
  else
  {
    permutation.Allocate(static_cast<vtkm::Id>(permutationVec.size()));
    std::copy(permutationVec.begin(),
              permutationVec.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(permutation.GetPortalControl()));
  }

  shapes.Allocate(static_cast<vtkm::Id>(newShapes.size()));
  std::copy(newShapes.begin(),
            newShapes.end(),
            vtkm::cont::ArrayPortalToIteratorBegin(shapes.GetPortalControl()));
  numIndices.Allocate(static_cast<vtkm::Id>(newNumIndices.size()));
  std::copy(newNumIndices.begin(),
            newNumIndices.end(),
            vtkm::cont::ArrayPortalToIteratorBegin(numIndices.GetPortalControl()));
  connectivity.Allocate(static_cast<vtkm::Id>(newConnectivity.size()));
  std::copy(newConnectivity.begin(),
            newConnectivity.end(),
            vtkm::cont::ArrayPortalToIteratorBegin(connectivity.GetPortalControl()));
}

inline bool IsSingleShape(const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes)
{
  auto shapesPortal = shapes.GetPortalConstControl();
  vtkm::UInt8 shape0 = shapesPortal.Get(0);
  for (vtkm::Id i = 1; i < shapes.GetNumberOfValues(); ++i)
  {
    if (shapesPortal.Get(i) != shape0)
      return false;
  }

  return true;
}
}
}
} // vtkm::io::internal

#endif // vtk_m_io_internal_VTKDataSetCells_h
