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

// Everything in this header file is deprecated and movded to StorageList.h.

#ifndef VTKM_DEFAULT_STORAGE_LIST_TAG
#define VTKM_DEFAULT_STORAGE_LIST_TAG ::vtkm::cont::detail::StorageListTagDefault
#endif

#include <vtkm/ListTag.h>

#include <vtkm/cont/StorageList.h>

namespace vtkm
{
namespace cont
{

struct VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(
  1.6,
  "StorageListTagBasic replaced by StorageListBasic. "
  "Note that the new StorageListBasic cannot be subclassed.") StorageListTagBasic
  : vtkm::internal::ListAsListTag<StorageListBasic>
{
};

struct VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(
  1.6,
  "StorageListTagSupported replaced by StorageListSupported. "
  "Note that the new StorageListSupported cannot be subclassed.") StorageListTagSupported
  : vtkm::internal::ListAsListTag<StorageListSupported>
{
};

namespace detail
{

struct VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(
  1.6,
  "VTKM_DEFAULT_STORAGE_LIST_TAG replaced by VTKM_DEFAULT_STORAGE_LIST. "
  "Note that the new VTKM_DEFAULT_STORAGE_LIST cannot be subclassed.") StorageListTagDefault
  : vtkm::internal::ListAsListTag<VTKM_DEFAULT_STORAGE_LIST>
{
};

} // namespace detail
}
} // namespace vtkm::cont

#endif //vtk_m_cont_StorageListTag_h
