//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define vtkm_cont_CellSetStructured_cxx
#include <vtkm/cont/CellSetStructured.h>

namespace vtkm
{
namespace cont
{

template class VTKM_CONT_EXPORT CellSetStructured<1>;
template class VTKM_CONT_EXPORT CellSetStructured<2>;
template class VTKM_CONT_EXPORT CellSetStructured<3>;
}
}
