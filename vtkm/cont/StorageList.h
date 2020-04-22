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

#include <vtkm/List.h>

#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/Storage.h>

namespace vtkm
{
namespace cont
{

using StorageListBasic = vtkm::List<vtkm::cont::StorageTagBasic>;
}
} // namespace vtkm::cont

#endif //vtk_m_cont_StorageList_h
