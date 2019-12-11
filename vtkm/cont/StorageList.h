//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_StorageList_h
#define vtk_m_cont_StorageList_h

#ifndef VTKM_DEFAULT_STORAGE_LIST
#define VTKM_DEFAULT_STORAGE_LIST ::vtkm::cont::StorageListBasic
#endif

#include <vtkm/List.h>

#include <vtkm/cont/Storage.h>
#include <vtkm/cont/StorageBasic.h>

namespace vtkm
{
namespace cont
{

using StorageListBasic = vtkm::List<vtkm::cont::StorageTagBasic>;

// If we want to compile VTK-m with support of memory layouts other than the basic layout, then
// add the appropriate storage tags here.
using StorageListSupported = vtkm::List<vtkm::cont::StorageTagBasic>;
}
} // namespace vtkm::cont

#endif //vtk_m_cont_StorageList_h
