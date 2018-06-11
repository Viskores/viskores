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
#ifndef vtk_m_cont_tbb_DeviceAdapterTBB_h
#define vtk_m_cont_tbb_DeviceAdapterTBB_h

#include <vtkm/cont/tbb/internal/DeviceAdapterRuntimeDetectorTBB.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>

#ifdef VTKM_ENABLE_TBB
#include <vtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterAlgorithmTBB.h>
#include <vtkm/cont/tbb/internal/VirtualObjectTransferTBB.h>
#endif

#endif //vtk_m_cont_tbb_DeviceAdapterTBB_h
