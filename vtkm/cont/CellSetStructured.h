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
#ifndef vtk_m_cont_CellSetStructured_h
#define vtk_m_cont_CellSetStructured_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/RegularConnectivity.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/internal/ConnectivityStructuredInternals.h>

namespace vtkm {
namespace cont {



template<vtkm::IdComponent DIMENSION>
class CellSetStructured : public CellSet
{
public:
  static const vtkm::IdComponent Dimension=DIMENSION;

  VTKM_CONT_EXPORT
  CellSetStructured(const std::string &name = std::string())
    : CellSet(name,Dimension)
  {
  }


  virtual vtkm::Id GetNumCells() const
  {
    return this->Structure.GetNumberOfCells();
  }

  template<typename FromTopology, typename ToTopology>
  struct ConnectivityType {
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);
    typedef vtkm::RegularConnectivity<FromTopology,ToTopology,Dimension> Type;
  };

  VTKM_CONT_EXPORT
  vtkm::RegularConnectivity<
      vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell,Dimension>
  GetNodeToCellConnectivity() const
  {
    typedef vtkm::RegularConnectivity<vtkm::TopologyElementTagPoint,
                                      vtkm::TopologyElementTagCell,
                                      Dimension> NodeToCellConnectivity;
    return NodeToCellConnectivity(this->Structure);
  }

  VTKM_CONT_EXPORT
  vtkm::RegularConnectivity<
      vtkm::TopologyElementTagCell,vtkm::TopologyElementTagPoint,Dimension>
  GetCellToNodeConnectivity() const
  {
    typedef vtkm::RegularConnectivity<vtkm::TopologyElementTagCell,
                                      vtkm::TopologyElementTagPoint,
                                      Dimension> CellToNodeConnectivity;
    return CellToNodeConnectivity(this->Structure);
  }

  virtual void PrintSummary(std::ostream &out) const
  {
      out << "  StructuredCellSet: " << this->GetName()
          << " dim= " << this->GetDimensionality() << std::endl;
      this->Structure.PrintSummary(out);
  }

public:
  vtkm::internal::ConnectivityStructuredInternals<Dimension> Structure;
};


}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetStructured_h
