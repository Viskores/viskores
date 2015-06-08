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
//  Copyright 2014 Los Alamos National Security.
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
#include <vtkm/cont/testing/Testing.h>
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
  void SetNodeDimension(vtkm::Vec<vtkm::Id,1> dims)
  {
    nodeDim = dims;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,1> GetNodeDimensions() const { return nodeDim; }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfNodes() const {return nodeDim[0];}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfElements() const {return GetNumberOfCells();}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const {return nodeDim[0]-1;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfIndices() const {return 2;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetElementShapeType() const {return VTKM_LINE;}

  template <int IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 2);
    ids[0] = index;
    ids[1] = ids[0] + 1;
  }

  template <int IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetCellsOfNode(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 2);
    ids[0] = ids[1] = -1;
    vtkm::Id idx = 0;
    if (index > 0)
	   ids[idx++] = index-1;
    if (index < nodeDim[0]-1)
	   ids[idx++] = index;
  }

  virtual void PrintSummary(std::ostream &out)
  {
    out<<"   RegularConnectivity<1> ";
    out<<"nodeDim["<<nodeDim[0]<<"] ";
    out<<"\n";
  }

private:
  vtkm::Vec<vtkm::Id,1> nodeDim;
};

//2 D specialization.
template<>
class RegularStructure<2>
{
public:
  VTKM_EXEC_CONT_EXPORT
  void SetNodeDimension(vtkm::Vec<vtkm::Id,2> dims)
  {
    nodeDims = dims;
    cellDims = dims - vtkm::Id2(1);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,2> GetNodeDimensions() const { return nodeDims; }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfNodes() const {return vtkm::internal::VecProduct<2>()(nodeDims);}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfElements() const {return GetNumberOfCells();}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const {return vtkm::internal::VecProduct<2>()(cellDims);}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfIndices() const {return 4;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetElementShapeType() const {return VTKM_PIXEL;}

  template <int IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 4);

    vtkm::Id i, j;
    CalculateLogicalNodeIndices(index, i, j);

    ids[0] = j*nodeDims[0] + i;
    ids[1] = ids[0] + 1;
    ids[2] = ids[0] + nodeDims[0];
    ids[3] = ids[2] + 1;
  }

  template <int IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetCellsOfNode(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 4);

    ids[0] = ids[1] = ids[2] = ids[3] = -1;
    vtkm::Id i, j, idx = 0;
    CalculateLogicalNodeIndices(index, i, j);
    if (i > 0 && j > 0)
      ids[idx++] = CalculateCellIndex(i-1, j-1);
    if (i < nodeDims[0]-1 && j > 0)
      ids[idx++] = CalculateCellIndex(i  , j-1);
    if (i > 0 && j < nodeDims[1]-1)
      ids[idx++] = CalculateCellIndex(i-1, j  );
    if (i < nodeDims[0]-1 && j < nodeDims[1]-1)
      ids[idx++] = CalculateCellIndex(i  , j  );
  }

  virtual void PrintSummary(std::ostream &out)
  {
      out<<"   RegularConnectivity<2> ";
      out<<"cellDim["<<cellDims[0]<<" "<<cellDims[1]<<"] ";
      out<<"nodeDim["<<nodeDims[0]<<" "<<nodeDims[1]<<"] ";
      out<<"\n";
  }

private:
  vtkm::Id2 cellDims;
  vtkm::Id2 nodeDims;

private:
  VTKM_EXEC_CONT_EXPORT
  void CalculateLogicalNodeIndices(vtkm::Id index, vtkm::Id &i, vtkm::Id &j) const
  {
      i = index % cellDims[0];
      j = index / cellDims[0];
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id CalculateCellIndex(vtkm::Id i, vtkm::Id j) const
  {
      return j*cellDims[0] + i;
  }
};

//3 D specialization.
template<>
class RegularStructure<3>
{
public:
  VTKM_EXEC_CONT_EXPORT
  void SetNodeDimension(vtkm::Vec<vtkm::Id,3> dims)
  {
    nodeDims = dims;
    cellDims = dims - vtkm::Id3(1);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,3> GetNodeDimensions() const { return nodeDims; }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfNodes() const {return vtkm::internal::VecProduct<3>()(nodeDims);}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfElements() const {return GetNumberOfCells();}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const {return vtkm::internal::VecProduct<3>()(cellDims);}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfIndices() const {return 8;}
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetElementShapeType() const {return VTKM_VOXEL;}

  template <int IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 8);

    vtkm::Id cellDims01 = cellDims[0] * cellDims[1];
    vtkm::Id k = index / cellDims01;
    vtkm::Id indexij = index % cellDims01;
    vtkm::Id j = indexij / cellDims[0];
    vtkm::Id i = indexij % cellDims[0];

    ids[0] = (k * nodeDims[1] + j) * nodeDims[0] + i;
    ids[1] = ids[0] + 1;
    ids[2] = ids[0] + nodeDims[0];
    ids[3] = ids[2] + 1;
    ids[4] = ids[0] + nodeDims[0]*nodeDims[1];
    ids[5] = ids[4] + 1;
    ids[6] = ids[4] + nodeDims[0];
    ids[7] = ids[6] + 1;
  }

  template <int IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetCellsOfNode(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 8);

    ids[0]=ids[1]=ids[2]=ids[3]=ids[4]=ids[5]=ids[6]=ids[7]=-1;

    vtkm::Id i, j, k, idx=0;

    CalculateLogicalNodeIndices(index, i, j, k);
    if (i > 0 && j > 0 && k > 0)
      ids[idx++] = CalculateCellIndex(i-1, j-1, k-1);
    if (i < nodeDims[0]-1 && j > 0 && k > 0)
      ids[idx++] = CalculateCellIndex(i  , j-1, k-1);
    if (i > 0 && j < nodeDims[1]-1 && k > 0)
      ids[idx++] = CalculateCellIndex(i-1, j  , k-1);
    if (i < nodeDims[0]-1 && j < nodeDims[1]-1 && k > 0)
      ids[idx++] = CalculateCellIndex(i  , j  , k-1);

    if (i > 0 && j > 0 && k < nodeDims[2]-1)
      ids[idx++] = CalculateCellIndex(i-1, j-1, k);
    if (i < nodeDims[0]-1 && j > 0 && k < nodeDims[2]-1)
      ids[idx++] = CalculateCellIndex(i  , j-1, k);
    if (i > 0 && j < nodeDims[1]-1 && k < nodeDims[2]-1)
      ids[idx++] = CalculateCellIndex(i-1, j  , k);
    if (i < nodeDims[0]-1 && j < nodeDims[1]-1 && k < nodeDims[2]-1)
      ids[idx++] = CalculateCellIndex(i  , j  , k);
  }

  virtual void PrintSummary(std::ostream &out)
  {
    out<<"   RegularConnectivity<3> ";
    out<<"cellDim["<<cellDims[0]<<" "<<cellDims[1]<<" "<<cellDims[2]<<"] ";
    out<<"nodeDim["<<nodeDims[0]<<" "<<nodeDims[1]<<" "<<nodeDims[2]<<"] ";
    out<<"\n";
  }

private:
  vtkm::Id3 cellDims;
  vtkm::Id3 nodeDims;


private:
  VTKM_EXEC_CONT_EXPORT
  void CalculateLogicalNodeIndices(vtkm::Id index, vtkm::Id &i, vtkm::Id &j, vtkm::Id &k) const
  {
    vtkm::Id nodeDims01 = nodeDims[0] * nodeDims[1];
    k = index / nodeDims01;
    vtkm::Id indexij = index % nodeDims01;
    j = indexij / nodeDims[0];
    i = indexij % nodeDims[0];
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id CalculateCellIndex(vtkm::Id i, vtkm::Id j, vtkm::Id k) const
  {
    return (k * cellDims[1] + j) * cellDims[0] + i;
  }
};

} // namespace vtkm

#endif //vtk_m_RegularStructure_h
