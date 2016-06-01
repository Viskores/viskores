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

#include <GL/gl.h>
#include <vtkm/rendering/ColorTable.h>

#include <vector>

namespace vtkm {
namespace rendering {

class TextureGL
{
public:
  GLuint ID;
  int    Dimension;
  bool   MIPMap;
  bool   Linear2D;
  bool   LinearMIP;
public:
  TextureGL()
  {
    this->ID = 0;
    this->Dimension = 0;
    this->MIPMap = false;
    this->Linear2D = true;
    this->LinearMIP = true;
  }
  void Enable()
  {
    if (this->ID == 0)
      return;

    if (this->Dimension == 1)
    {
      // no this->MIPMapping for 1D (at the moment)
      glBindTexture(GL_TEXTURE_1D, this->ID);
      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      if (this->Linear2D)
      {
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      }
      else
      {
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      }
      glEnable(GL_TEXTURE_1D);
    }
    else if (this->Dimension == 2)
    {
      glBindTexture(GL_TEXTURE_2D, this->ID);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      if (this->Linear2D)
      {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        if (!this->MIPMap)
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        else if (this->LinearMIP)
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        else
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
      }
      else
      {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        if (!this->MIPMap)
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        else if (this->LinearMIP)
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        else
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
      }
      glEnable(GL_TEXTURE_2D);
    }
  }
  void Disable()
  {
    if (this->Dimension == 1)
      glDisable(GL_TEXTURE_1D);
    else if (this->Dimension == 2)
      glDisable(GL_TEXTURE_2D);
  }
  void CreateAlphaFromRGBA(int w, int h, std::vector<unsigned char> &rgba)
  {
    this->Dimension = 2;
    std::vector<unsigned char> alpha(rgba.size()/4);
    for (unsigned int i=0; i<alpha.size(); i++)
    {
      alpha[i] = rgba[i*4+3];
    }

    if (this->ID == 0)
    {
      glGenTextures(1, &this->ID);
    }

    if (this->Dimension == 1)
    {
      glBindTexture(GL_TEXTURE_1D, this->ID);
    }
    else if (this->Dimension == 2)
    {
      glBindTexture(GL_TEXTURE_2D, this->ID);
//#define HW_MIPMAPS
#ifdef HW_MIPMAPS
      mpimap = true;
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
#endif
      glTexImage2D(GL_TEXTURE_2D, 0,
                   GL_ALPHA,
                   w, h,
                   0,
                   GL_ALPHA,
                   GL_UNSIGNED_BYTE,
                   (void*)(&(alpha[0])));
    }
  }
};


}} //namespace vtkm::rendering

#endif //vtk_m_rendering_TextureGL_h
