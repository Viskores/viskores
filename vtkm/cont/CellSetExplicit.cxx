//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
                  VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
                  typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag>;
}
}
