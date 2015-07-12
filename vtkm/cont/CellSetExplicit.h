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

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/ExplicitConnectivity.h>

namespace vtkm {
namespace cont {

template<typename ShapeStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename IndiceStorageTag        = VTKM_DEFAULT_STORAGE_TAG,
         typename ConnectivityStorageTag  = VTKM_DEFAULT_STORAGE_TAG >
class CellSetExplicit : public CellSet
{
public:
  typedef ExplicitConnectivity<ShapeStorageTag,
                               IndiceStorageTag,
                               ConnectivityStorageTag
                               > ExplicitConnectivityType;

  VTKM_CONT_EXPORT
  CellSetExplicit(const std::string &name, vtkm::IdComponent dimensionality)
    : CellSet(name, dimensionality)
  {
  }

  virtual vtkm::Id GetNumCells() const
  {
    return this->NodesOfCellsConnectivity.GetNumberOfElements();
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
