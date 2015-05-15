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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_RegularStructure_h
#define vtk_m_RegularStructure_h

#include <vtkm/Types.h>
#include <vtkm/CellType.h>

namespace vtkm {

template<vtkm::IdComponent> class RegularStructure;

//1 D specialization.
template<>
class RegularStructure<1>
{
public:
  VTKM_EXEC_CONT_EXPORT
  void SetNodeDimension(int node_i, int, int)
  {
      cellDims[0] = node_i-1;
      nodeDims[0] = node_i;
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const {return cellDims[0];}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfIndices() const {return 2;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetElementShapeType() const {return VTKM_LINE;}

  VTKM_EXEC_CONT_EXPORT
  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,2> &ids) const
  {
      ids[0] = index;
      ids[1] = ids[0] + 1;
  }
    
private:
    vtkm::Id cellDims[1];
    vtkm::Id nodeDims[1];
};

//2 D specialization.
template<>
class RegularStructure<2>
{
public:
  VTKM_EXEC_CONT_EXPORT
  void SetNodeDimension(int node_i, int node_j, int)
  {
      cellDims[0] = node_i-1;
      nodeDims[0] = node_i;
      cellDims[1] = node_j-1;
      nodeDims[1] = node_j;
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const {return cellDims[0]*cellDims[1];}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfIndices() const {return 4;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetElementShapeType() const {return VTKM_PIXEL;}

  VTKM_EXEC_CONT_EXPORT
  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,4> &ids) const
  {
      int i = index % cellDims[0];
      int j = index / cellDims[0];

      ids[0] = j*nodeDims[0] + i;
      ids[1] = ids[0] + 1;
      ids[2] = ids[0] + nodeDims[0];
      ids[3] = ids[2] + 1;
  }
    
private:
    vtkm::Id cellDims[2];
    vtkm::Id nodeDims[2];
};

//3 D specialization.
template<>
class RegularStructure<3>
{
public:
  VTKM_EXEC_CONT_EXPORT
  void SetNodeDimension(int node_i, int node_j, int node_k)
  {
      cellDims[0] = node_i-1;
      nodeDims[0] = node_i;
      cellDims[1] = node_j-1;
      nodeDims[1] = node_j;
      cellDims[2] = node_k-1;
      nodeDims[2] = node_k;
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const {return cellDims[0]*cellDims[1]*cellDims[2];}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfIndices() const {return 8;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetElementShapeType() const {return VTKM_VOXEL;}

  VTKM_EXEC_CONT_EXPORT
  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,8> &ids) const
  {
      int cellDims01 = cellDims[0] * cellDims[1];
      int k = index / cellDims01;
      int indexij = index % cellDims01;
      int j = indexij / cellDims[0];
      int i = indexij % cellDims[0];

      ids[0] = (k * nodeDims[1] + j) * nodeDims[0] + i;
      ids[1] = ids[0] + 1;
      ids[2] = ids[0] + nodeDims[0];
      ids[3] = ids[2] + 1;
      ids[4] = ids[0] + nodeDims[0]*nodeDims[1];
      ids[5] = ids[4] + 1;
      ids[6] = ids[4] + nodeDims[0];
      ids[7] = ids[6] + 1;
  }
    
private:
    vtkm::Id cellDims[3];
    vtkm::Id nodeDims[3];
};

} // namespace vtkm

#endif //vtk_m_RegularStructure_h
