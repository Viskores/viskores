//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_TextureGL_h
#define vtk_m_TextureGL_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/ColorTable.h>

#include <vector>

namespace vtkm {
namespace rendering {

class TextureGL
{
public:

public:
  VTKM_RENDERING_EXPORT
  TextureGL();

  VTKM_RENDERING_EXPORT
  ~TextureGL();

  VTKM_RENDERING_EXPORT
  bool Valid() const;

  VTKM_RENDERING_EXPORT
  void Enable() const;

  VTKM_RENDERING_EXPORT
  void Disable() const;

  VTKM_RENDERING_EXPORT
  void CreateAlphaFromRGBA(vtkm::Id width,
                           vtkm::Id height,
                           const std::vector<unsigned char> &rgba);

private:
  TextureGL(const TextureGL &); // Not implemented
  void operator=(const TextureGL &); // Not implemented

  struct InternalsType;
  InternalsType *Internals;
};


}} //namespace vtkm::rendering

#endif //vtk_m_rendering_TextureGL_h
