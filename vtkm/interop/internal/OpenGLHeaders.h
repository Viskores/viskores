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
#ifndef vtk_m_interop_internal_OpenGLHeaders_h
#define vtk_m_interop_internal_OpenGLHeaders_h

#include <vtkm/internal/ExportMacros.h>

#if defined(__APPLE__)

#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifdef VTKM_CUDA
#include <cuda_runtime.h>

#include <cuda_gl_interop.h>
#endif

#endif //vtk_m_interop_internal_OpenGLHeaders_h
