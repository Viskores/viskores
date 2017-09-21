//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#define vtk_m_cont_CellSetExplicit_cxx

#include <vtkm/cont/CellSetExplicit.h>

namespace vtkm
{
namespace cont
{

template class VTKM_CONT_EXPORT CellSetExplicit<>; // default
template class VTKM_CONT_EXPORT
  CellSetExplicit<typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag,
                  typename vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>::StorageTag,
                  VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
                  typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag>;
}
}
