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

#ifndef vtk_m_filter_DefaultPolicy_h
#define vtk_m_filter_DefaultPolicy_h

#include <vtkm/filter/PolicyBase.h>
#include <vtkm/cont/CellSetListTag.h>
#include <vtkm/cont/StorageListTag.h>
#include <vtkm/TypeListTag.h>

namespace vtkm {
namespace filter {

class DefaultPolicy : public vtkm::filter::PolicyBase< DefaultPolicy >
{
public:
  typedef VTKM_DEFAULT_TYPE_LIST_TAG    FieldTypeList;
  typedef VTKM_DEFAULT_STORAGE_LIST_TAG FieldStorageList;

  typedef vtkm::cont::CellSetListTagStructured StructuredCellSetList;
  typedef vtkm::cont::CellSetListTagUnstructured UnstructuredCellSetList;
  typedef VTKM_DEFAULT_CELL_SET_LIST_TAG AllCellSetList;

  typedef VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG CoordinateTypeList;
  typedef VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG CoordinateStorageList;
};

}
}


#endif //vtk_m_filter_DefaultPolicy_h
