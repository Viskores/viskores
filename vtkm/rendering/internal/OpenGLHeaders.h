//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_rendering_internal_OpenGLHeaders_h
#define vtk_m_rendering_internal_OpenGLHeaders_h

#include <vtkm/internal/Windows.h>

#include <GL/glew.h>

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#endif //vtk_m_rendering_internal_OpenGLHeaders_h
