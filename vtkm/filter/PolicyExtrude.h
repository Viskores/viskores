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

#include <vtkm/ListTag.h>
#include <vtkm/filter/PolicyDefault.h>

struct VTKM_ALWAYS_EXPORT ExtrudeUnstructuredCellSets
  : vtkm::ListTagBase<vtkm::cont::CellSetExtrude>
{
};

//Todo: add in Cylinder storage tag when it is written
struct VTKM_ALWAYS_EXPORT ExtrudeCoordinateStorage
  : vtkm::ListTagBase<vtkm::cont::StorageTagBasic, vtkm::cont::internal::StorageTagExtrude>
{
};

struct VTKM_ALWAYS_EXPORT PolicyExtrude : vtkm::filter::PolicyBase<PolicyExtrude>
{
public:
  using UnstructuredCellSetList = ExtrudeUnstructuredCellSets;
  using AllCellSetList = ExtrudeUnstructuredCellSets;
  using CoordinateStorageList = ExtrudeCoordinateStorage;
};

#endif
