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
    this->NodeDimensions = dims;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,1> GetNodeDimensions() const
  { return this->NodeDimensions;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetSchedulingDimensions() const
  {
    return this->GetNumberOfCells();
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfNodes() const {return this->NodeDimensions[0];}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfElements() const {return this->GetNumberOfCells();}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const {return this->NodeDimensions[0]-1;}
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
    vtkm::IdComponent idx = 0;
    if (index > 0)
	   ids[idx++] = index-1;
    if (index < this->NodeDimensions[0]-1)
	   ids[idx++] = index;
  }

  virtual void PrintSummary(std::ostream &out) const
  {
    out<<"   RegularConnectivity<1> ";
    out<<"this->NodeDimensions["<<this->NodeDimensions[0]<<"] ";
    out<<"\n";
  }

private:
  vtkm::Vec<vtkm::Id,1> NodeDimensions;
};

//2 D specialization.
template<>
class RegularStructure<2>
{
public:
  VTKM_EXEC_CONT_EXPORT
  void SetNodeDimension(vtkm::Vec<vtkm::Id,2> dims)
  {
    this->NodeDimensions = dims;
    this->CellDimensions = dims - vtkm::Id2(1);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,2> GetNodeDimensions() const { return this->NodeDimensions; }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfNodes() const
  {
    return vtkm::internal::VecProduct<2>()(this->NodeDimensions);
  }

  //returns an id2 to signal what kind of scheduling to use
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id2 GetSchedulingDimensions() const {return this->CellDimensions;}

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfElements() const {return GetNumberOfCells();}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const
  {
    return vtkm::internal::VecProduct<2>()(this->CellDimensions);
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfIndices() const { return 4; }
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetElementShapeType() const { return VTKM_PIXEL; }

  template <int IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 4);

    vtkm::Id i, j;
    CalculateLogicalNodeIndices(index, i, j);

    ids[0] = j*this->NodeDimensions[0] + i;
    ids[1] = ids[0] + 1;
    ids[2] = ids[0] + this->NodeDimensions[0];
    ids[3] = ids[2] + 1;
  }

  template <int IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetCellsOfNode(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 4);

    ids[0] = ids[1] = ids[2] = ids[3] = -1;
    vtkm::Id i, j;
    vtkm::IdComponent idx = 0;
    CalculateLogicalNodeIndices(index, i, j);
    if ((i > 0) && (j > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j-1);
    }
    if ((i < this->NodeDimensions[0]-1) && (j > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j-1);
    }
    if ((i > 0) && (j < this->NodeDimensions[1]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j  );
    }
    if ((i < this->NodeDimensions[0]-1) && (j < this->NodeDimensions[1]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j  );
    }
  }

  virtual void PrintSummary(std::ostream &out) const
  {
      out<<"   RegularConnectivity<2> ";
      out<<"cellDim["<<this->CellDimensions[0]<<" "<<this->CellDimensions[1]<<"] ";
      out<<"nodeDim["<<this->NodeDimensions[0]<<" "<<this->NodeDimensions[1]<<"] ";
      out<<std::endl;
  }

private:
  vtkm::Id2 CellDimensions;
  vtkm::Id2 NodeDimensions;

private:
  VTKM_EXEC_CONT_EXPORT
  void CalculateLogicalNodeIndices(vtkm::Id index, vtkm::Id &i, vtkm::Id &j) const
  {
      i = index % this->CellDimensions[0];
      j = index / this->CellDimensions[0];
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id CalculateCellIndex(vtkm::Id i, vtkm::Id j) const
  {
      return j*this->CellDimensions[0] + i;
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
    this->NodeDimensions = dims;
    this->CellDimensions = dims - vtkm::Id3(1);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Vec<vtkm::Id,3> GetNodeDimensions() const
  {
    return this->NodeDimensions;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfNodes() const
  {
    return vtkm::internal::VecProduct<3>()(this->NodeDimensions);
  }

  //returns an id3 to signal what kind of scheduling to use
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id3 GetSchedulingDimensions() const { return this->CellDimensions;}

  vtkm::Id GetNumberOfElements() const { return this->GetNumberOfCells(); }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const
  {
    return vtkm::internal::VecProduct<3>()(this->CellDimensions);
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfIndices() const { return 8; }
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetElementShapeType() const { return VTKM_VOXEL; }

  template <int IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetNodesOfCells(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 8);

    vtkm::Id cellDims01 = this->CellDimensions[0] * this->CellDimensions[1];
    vtkm::Id k = index / cellDims01;
    vtkm::Id indexij = index % cellDims01;
    vtkm::Id j = indexij / this->CellDimensions[0];
    vtkm::Id i = indexij % this->CellDimensions[0];

    ids[0] = (k * this->NodeDimensions[1] + j) * this->NodeDimensions[0] + i;
    ids[1] = ids[0] + 1;
    ids[2] = ids[0] + this->NodeDimensions[0];
    ids[3] = ids[2] + 1;
    ids[4] = ids[0] + this->NodeDimensions[0]*this->NodeDimensions[1];
    ids[5] = ids[4] + 1;
    ids[6] = ids[4] + this->NodeDimensions[0];
    ids[7] = ids[6] + 1;
  }

  template <int IdsLength>
  VTKM_EXEC_CONT_EXPORT
  void GetCellsOfNode(vtkm::Id index, vtkm::Vec<vtkm::Id,IdsLength> &ids) const
  {
    BOOST_STATIC_ASSERT(IdsLength >= 8);

    ids[0]=ids[1]=ids[2]=ids[3]=ids[4]=ids[5]=ids[6]=ids[7]=-1;

    vtkm::Id i, j, k;
    vtkm::IdComponent idx=0;

    CalculateLogicalNodeIndices(index, i, j, k);
    if ((i > 0) && (j > 0) && (k > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j-1, k-1);
    }
    if ((i < this->NodeDimensions[0]-1) && (j > 0) && (k > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j-1, k-1);
    }
    if ((i > 0) && (j < this->NodeDimensions[1]-1) && (k > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j  , k-1);
    }
    if ((i < this->NodeDimensions[0]-1) &&
        (j < this->NodeDimensions[1]-1) &&
        (k > 0))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j  , k-1);
    }

    if ((i > 0) && (j > 0) && (k < this->NodeDimensions[2]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j-1, k);
    }
    if ((i < this->NodeDimensions[0]-1) &&
        (j > 0) &&
        (k < this->NodeDimensions[2]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j-1, k);
    }
    if ((i > 0) &&
        (j < this->NodeDimensions[1]-1) &&
        (k < this->NodeDimensions[2]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i-1, j  , k);
    }
    if ((i < this->NodeDimensions[0]-1) &&
        (j < this->NodeDimensions[1]-1) &&
        (k < this->NodeDimensions[2]-1))
    {
      ids[idx++] = this->CalculateCellIndex(i  , j  , k);
    }
  }

  virtual void PrintSummary(std::ostream &out) const
  {
    out<<"   RegularConnectivity<3> ";
    out<<"cellDim["<<this->CellDimensions[0]<<" "<<this->CellDimensions[1]<<" "<<this->CellDimensions[2]<<"] ";
    out<<"nodeDim["<<this->NodeDimensions[0]<<" "<<this->NodeDimensions[1]<<" "<<this->NodeDimensions[2]<<"] ";
    out<<std::endl;
  }

private:
  vtkm::Id3 CellDimensions;
  vtkm::Id3 NodeDimensions;


private:
  VTKM_EXEC_CONT_EXPORT
  void CalculateLogicalNodeIndices(vtkm::Id index, vtkm::Id &i, vtkm::Id &j, vtkm::Id &k) const
  {
    vtkm::Id nodeDims01 = this->NodeDimensions[0] * this->NodeDimensions[1];
    k = index / nodeDims01;
    vtkm::Id indexij = index % nodeDims01;
    j = indexij / this->NodeDimensions[0];
    i = indexij % this->NodeDimensions[0];
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id CalculateCellIndex(vtkm::Id i, vtkm::Id j, vtkm::Id k) const
  {
    return (k * this->CellDimensions[1] + j) * this->CellDimensions[0] + i;
  }
};

} // namespace vtkm

#endif //vtk_m_RegularStructure_h
