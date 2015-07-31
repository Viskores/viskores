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
#ifndef vtk_m_cont_CellSetExplicit_h
#define vtk_m_cont_CellSetExplicit_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/ConnectivityExplicit.h>

namespace vtkm {
namespace cont {

template<typename ShapeStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename IndexStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename ConnectivityStorageTag  = VTKM_DEFAULT_STORAGE_TAG >
class CellSetExplicit : public CellSet
{
public:
  typedef ConnectivityExplicit<ShapeStorageTag,
                               IndexStorageTag,
                               ConnectivityStorageTag
                               > ExplicitConnectivityType;

  VTKM_CONT_EXPORT
  CellSetExplicit(const std::string &name = std::string(),
                  vtkm::IdComponent dimensionality = 3)
    : CellSet(name, dimensionality)
  {
  }

  VTKM_CONT_EXPORT
  CellSetExplicit(int dimensionality)
    : CellSet(std::string(), dimensionality)
  {
  }

  virtual vtkm::Id GetNumCells() const
  {
    return this->NodesOfCellsConnectivity.GetNumberOfElements();
  }

  template<typename FromTopology, typename ToTopology>
  struct ConnectivityType {
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);
    // This type is really only valid for Point to Cell connectivity. When
    // other connectivity types are supported, these will need to be added.
    typedef ExplicitConnectivityType Type;
  };

  const ExplicitConnectivityType &GetNodeToCellConnectivity() const
  {
    return this->NodesOfCellsConnectivity;
  }

  ExplicitConnectivityType &GetNodeToCellConnectivity()
  {
    return this->NodesOfCellsConnectivity;
  }

  virtual void PrintSummary(std::ostream &out) const
  {
      out << "   ExplicitCellSet: " << this->Name
          << " dim= " << this->Dimensionality << std::endl;
      this->NodesOfCellsConnectivity.PrintSummary(out);
  }

public:
  ExplicitConnectivityType NodesOfCellsConnectivity;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetExplicit_h
