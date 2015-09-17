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
#ifndef vtk_m_cont_tbb_internal_DeviceAdapterTagTBB_h
#define vtk_m_cont_tbb_internal_DeviceAdapterTagTBB_h

#include <vtkm/cont/internal/DeviceAdapterTag.h>

//We always create the tbb tag when included, but we only mark it as
//a valid tag when VTKM_ENABLE_TBB is true. This is for easier development
//of multi-backend systems
#ifdef VTKM_ENABLE_TBB
VTKM_VALID_DEVICE_ADAPTER(TBB);
#else
VTKM_INVALID_DEVICE_ADAPTER(TBB);
#endif

#endif //vtk_m_cont_tbb_internal_DeviceAdapterTagTBB_h
