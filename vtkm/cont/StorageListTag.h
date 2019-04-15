//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_StorageListTag_h
#define vtk_m_cont_StorageListTag_h

#ifndef VTKM_DEFAULT_STORAGE_LIST_TAG
#define VTKM_DEFAULT_STORAGE_LIST_TAG ::vtkm::cont::StorageListTagBasic
#endif

#include <vtkm/ListTag.h>

#include <vtkm/cont/Storage.h>
#include <vtkm/cont/StorageBasic.h>

namespace vtkm
{
namespace cont
{

struct VTKM_ALWAYS_EXPORT StorageListTagBasic : vtkm::ListTagBase<vtkm::cont::StorageTagBasic>
{
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_StorageListTag_h
