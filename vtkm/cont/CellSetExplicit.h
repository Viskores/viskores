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

  CellSetExplicit(const std::string &n, vtkm::Id d)
    : CellSet(n,d)
  {
  }

  virtual vtkm::Id GetNumCells()
  {
    return nodesOfCellsConnectivity.GetNumberOfElements();
  }

  ExplicitConnectivityType &GetNodeToCellConnectivity()
  {
    return nodesOfCellsConnectivity;
  }

  virtual void PrintSummary(std::ostream &out)
  {
      out<<"   ExplicitCellSet: "<<name<<" dim= "<<dimensionality<<std::endl;
      nodesOfCellsConnectivity.PrintSummary(out);
  }

public:
  ExplicitConnectivityType nodesOfCellsConnectivity;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetExplicit_h
