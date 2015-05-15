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

#ifndef vtk_m_cont_RegularStructure_h
#define vtk_m_cont_RegularStructure_h

#include <vtkm/Extent.h>
#include <vtkm/CellType.h>

namespace vtkm {
namespace cont {

template<vtkm::IdComponent> class RegularStructure;

//1 D specialization.
template<>
class RegularStructure<1>
{
public:
  void SetNodeDimension(int node_i, int, int)
  {
      cellDims.Min[0] = nodeDims.Min[0] = 0;
      cellDims.Max[0] = node_i-1;
      nodeDims.Max[0] = node_i;
  }
  vtkm::Id GetNumberOfElements() const {return cellDims.Max[0];}
  vtkm::Id GetNumberOfIndices() const {return 2;}
  vtkm::CellType GetElementShapeType() const {return VTKM_LINE;}

  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,2> &ids) const
  {
      ids[0] = index;
      ids[1] = ids[0] + 1;
  }
    
private:
    Extent<1> cellDims;
    Extent<1> nodeDims;
};

//2 D specialization.
template<>
class RegularStructure<2>
{
public:
  void SetNodeDimension(int node_i, int node_j, int)
  {
      cellDims.Min[0] = cellDims.Min[1] = 0;
      nodeDims.Min[0] = nodeDims.Min[1] = 0;
      cellDims.Max[0] = node_i-1;
      nodeDims.Max[0] = node_i;
      cellDims.Max[1] = node_j-1;
      nodeDims.Max[1] = node_j;
  }
  vtkm::Id GetNumberOfElements() const {return cellDims.Max[0]*cellDims.Max[1];}
  vtkm::Id GetNumberOfIndices() const {return 4;}
  vtkm::CellType GetElementShapeType() const {return VTKM_PIXEL;}

  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,4> &ids) const
  {
      int i = index % cellDims.Max[0];
      int j = index / cellDims.Max[0];

      ids[0] = j*nodeDims.Max[0] + i;
      ids[1] = ids[0] + 1;
      ids[2] = ids[0] + nodeDims.Max[0];
      ids[3] = ids[2] + 1;
  }
    
private:
    Extent<2> cellDims;
    Extent<2> nodeDims;
};

//3 D specialization.
template<>
class RegularStructure<3>
{
public:
  void SetNodeDimension(int node_i, int node_j, int node_k)
  {
      cellDims.Min[0] = cellDims.Min[1] = cellDims.Min[2] = 0;
      nodeDims.Min[0] = nodeDims.Min[1] = nodeDims.Min[2] = 0;
      cellDims.Max[0] = node_i-1;
      nodeDims.Max[0] = node_i;
      cellDims.Max[1] = node_j-1;
      nodeDims.Max[1] = node_j;
      cellDims.Max[2] = node_k-1;
      nodeDims.Max[2] = node_k;
  }
  vtkm::Id GetNumberOfElements() const {return cellDims.Max[0]*cellDims.Max[1]*cellDims.Max[2];}
  vtkm::Id GetNumberOfIndices() const {return 8;}
  vtkm::CellType GetElementShapeType() const {return VTKM_VOXEL;}

  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,8> &ids) const
  {
      int cellDims01 = cellDims.Max[0] * cellDims.Max[1];
      int k = index / cellDims01;
      int indexij = index % cellDims01;
      int j = indexij / cellDims.Max[0];
      int i = indexij % cellDims.Max[0];

      ids[0] = (k * nodeDims.Max[1] + j) * nodeDims.Max[0] + i;
      ids[1] = ids[0] + 1;
      ids[2] = ids[0] + nodeDims.Max[0];
      ids[3] = ids[2] + 1;
      ids[4] = ids[0] + nodeDims.Max[0]*nodeDims.Max[1];
      ids[5] = ids[4] + 1;
      ids[6] = ids[4] + nodeDims.Max[0];
      ids[7] = ids[6] + 1;
  }
    
private:
    Extent<3> cellDims;
    Extent<3> nodeDims;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_RegularStructure_h
