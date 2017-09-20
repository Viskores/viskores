//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_internal_DeviceAdapterError_h
#define vtk_m_cont_internal_DeviceAdapterError_h

#include <vtkm/cont/internal/DeviceAdapterTag.h>

/// This is an invalid DeviceAdapter. The point of this class is to include the
/// header file to make this invalid class the default DeviceAdapter. From that
/// point, you have to specify an appropriate DeviceAdapter or else get a
/// compile error.
///
VTKM_INVALID_DEVICE_ADAPTER(Error, VTKM_DEVICE_ADAPTER_ERROR);

#endif //vtk_m_cont_internal_DeviceAdapterError_h
