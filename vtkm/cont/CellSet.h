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
#ifndef vtk_m_cont_CellSet_h
#define vtk_m_cont_CellSet_h

#include <vtkm/CellType.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/LogicalStructure.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {
namespace cont {

class CellSet
{
public:
  VTKM_CONT_EXPORT
  CellSet(const std::string &name, vtkm::IdComponent dimensionality)
    : Name(name), Dimensionality(dimensionality), LogicalStructure()
  {
  }

  virtual ~CellSet()
  {
  }

  virtual std::string GetName() const
  {
    return this->Name;
  }
  virtual vtkm::IdComponent GetDimensionality() const
  {
    return this->Dimensionality;
  }

  virtual vtkm::Id GetNumCells() const = 0;

  virtual vtkm::Id GetNumFaces() const
  {
    return 0;
  }

  virtual vtkm::Id GetNumEdges() const
  {
    return 0;
  }

  virtual void PrintSummary(std::ostream&) const = 0;

protected:
    std::string Name;
    vtkm::IdComponent Dimensionality;
    vtkm::cont::LogicalStructure LogicalStructure;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSet_h
