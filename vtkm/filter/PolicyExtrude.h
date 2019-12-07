//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtk_m_filter_PolicyExtrude_h
#define vtk_m_filter_PolicyExtrude_h

#include <vtkm/cont/ArrayHandleExtrudeCoords.h>
#include <vtkm/cont/CellSetExtrude.h>

#include <vtkm/List.h>
#include <vtkm/filter/PolicyDefault.h>

namespace vtkm
{
namespace filter
{

struct VTKM_ALWAYS_EXPORT PolicyExtrude : vtkm::filter::PolicyBase<PolicyExtrude>
{
public:
  using UnstructuredCellSetList = vtkm::List<vtkm::cont::CellSetExtrude>;
  using AllCellSetList = vtkm::List<vtkm::cont::CellSetExtrude>;
  //Todo: add in Cylinder storage tag when it is written
  using CoordinateStorageList =
    vtkm::List<vtkm::cont::StorageTagBasic, vtkm::cont::internal::StorageTagExtrude>;
};
}
}

#endif
