//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_thirdparty_diy_diy_h
#define vtk_m_thirdparty_diy_diy_h


#include "pre-include.h"
// clang-format off
#include VTKM_DIY_INCLUDE(assigner.hpp)
#include VTKM_DIY_INCLUDE(decomposition.hpp)
#include VTKM_DIY_INCLUDE(master.hpp)
#include VTKM_DIY_INCLUDE(mpi.hpp)
#include VTKM_DIY_INCLUDE(partners/all-reduce.hpp)
#include VTKM_DIY_INCLUDE(partners/broadcast.hpp)
#include VTKM_DIY_INCLUDE(partners/swap.hpp)
#include VTKM_DIY_INCLUDE(reduce.hpp)
#include VTKM_DIY_INCLUDE(reduce-operations.hpp)
#include VTKM_DIY_INCLUDE(resolve.hpp)
#include VTKM_DIY_INCLUDE(serialization.hpp)
// clang-format on
#include "post-include.h"

#endif // vtk_m_thirdparty_diy_diy_h
