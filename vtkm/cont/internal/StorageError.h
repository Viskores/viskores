//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_cont_internal_StorageError_h
#define vtkm_cont_internal_StorageError_h

#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// This is an invalid Storage. The point of this class is to include the
/// header file to make this invalid class the default Storage. From that
/// point, you have to specify an appropriate Storage or else get a compile
/// error.
///
struct VTKM_ALWAYS_EXPORT StorageTagError
{
  // Not implemented.
};
}
}
} // namespace vtkm::cont::internal

#endif //vtkm_cont_internal_StorageError_h
