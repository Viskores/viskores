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

#ifndef vtk_m_filter_CellFilter_h
#define vtk_m_filter_CellFilter_h

#include <vtkm/filter/FieldFilter.h>

namespace vtkm {
namespace filter {

template<class Derived>
class CellFilter : public vtkm::filter::FieldFilter< Derived >
{
public:
  VTKM_CONT_EXPORT
  CellFilter();

  VTKM_CONT_EXPORT
  void SetActiveCellSet(vtkm::Id index)
    { this->CellSetIndex = index; }

  VTKM_CONT_EXPORT
  vtkm::Id GetActiveCellSetIndex() const
    { return this->CellSetIndex; }

protected:
  vtkm::Id CellSetIndex;
};

}
} // namespace vtkm::filter


#include <vtkm/filter/CellFilter.hxx>

#endif // vtk_m_filter_CellFilter_h
