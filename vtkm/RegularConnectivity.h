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


#ifndef vtk_m_RegularConnectivity_h
#define vtk_m_RegularConnectivity_h

#include <vtkm/Types.h>
#include <vtkm/RegularStructure.h>
#include <vtkm/cont/TopologyType.h>

#include <boost/static_assert.hpp>

namespace vtkm {

template<vtkm::cont::TopologyType From, vtkm::cont::TopologyType To, vtkm::IdComponent Dimension>
struct IndexLookupHelper
{
  // We want an unconditional failure if this unspecialized class ever gets
  // instantiated, because it means someone missed a topology mapping type.
  // We need to create a test which depends on the templated types so
  // it doesn't get picked up without a concrete instantiation.
  BOOST_STATIC_ASSERT_MSG(From != To && From == To,
                          "Missing Specialization for Topologies");
};

template<vtkm::IdComponent Dimension>
struct IndexLookupHelper<vtkm::cont::NODE,vtkm::cont::CELL,Dimension>
{
  template <vtkm::IdComponent ItemTupleLength>
  VTKM_EXEC_CONT_EXPORT
  static void GetIndices(RegularStructure<Dimension> &rs,
                  vtkm::Id index, vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    rs.GetNodesOfCells(index,ids);
  }
};

template<vtkm::IdComponent Dimension>
struct IndexLookupHelper<vtkm::cont::CELL,vtkm::cont::NODE,Dimension>
{
  template <vtkm::IdComponent ItemTupleLength>
  VTKM_EXEC_CONT_EXPORT
  static void GetIndices(RegularStructure<Dimension> &rs,
                  vtkm::Id index, vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    rs.GetCellsOfNode(index,ids);
  }
};

template<vtkm::cont::TopologyType FromTopology, vtkm::cont::TopologyType ToTopoogy,
         vtkm::IdComponent Dimension>
class RegularConnectivity
{
public:
  VTKM_EXEC_CONT_EXPORT
  void SetNodeDimension(int node_i, int node_j=0, int node_k=0)
  {
     rs.SetNodeDimension(node_i, node_j, node_k);
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfElements() const {return rs.GetNumberOfElements();}
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfIndices(vtkm::Id=0) const {return rs.GetNumberOfIndices();}
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetElementShapeType(vtkm::Id=0) const {return rs.GetElementShapeType();}

  template <vtkm::IdComponent ItemTupleLength>
  VTKM_EXEC_CONT_EXPORT
  void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    IndexLookupHelper<FromTopology,ToTopoogy,Dimension>::GetIndices(rs,index,ids);
  }
private:
  RegularStructure<Dimension> rs;
};
    
} // namespace vtkm

#endif //vtk_m_RegularConnectivity_h
