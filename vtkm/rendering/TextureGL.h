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
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_TextureGL_h
#define vtk_m_TextureGL_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/Types.h>

#include <memory>
#include <vector>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT TextureGL
{
public:
  TextureGL();

  ~TextureGL();

  bool Valid() const;

  void Enable() const;

  void Disable() const;

  void CreateAlphaFromRGBA(vtkm::Id width, vtkm::Id height, const std::vector<unsigned char>& rgba);

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_TextureGL_h
