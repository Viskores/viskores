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
    GLuint id;
    int dim;
    bool mipmap;
    bool linear2D;
    bool linearMip;
  public:
    TextureGL()
    {
        id = 0;
        dim = 0;
        mipmap = false;
        linear2D = true;
        linearMip = true;
    }
    void Enable()
    {
        if (id == 0)
            return;

        if (dim == 1)
        {
            // no mipmapping for 1D (at the moment)
            glBindTexture(GL_TEXTURE_1D, id);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            if (linear2D)
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
        else if (dim == 2)
        {
            std::cerr << "Binding 2D, id="<<id<<"\n";
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            if (linear2D)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                if (!mipmap)
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                else if (linearMip)
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                else
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                if (!mipmap)
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                else if (linearMip)
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                else
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            }
            glEnable(GL_TEXTURE_2D);
        }
    }
    void Disable()
    {
        if (dim == 1)
            glDisable(GL_TEXTURE_1D);
        else if (dim == 2)
            glDisable(GL_TEXTURE_2D);
    }
    void CreateAlphaFromRGBA(int w, int h, std::vector<unsigned char> &rgba)
    {
        dim = 2;
        std::vector<unsigned char> alpha(rgba.size()/4);
        for (unsigned int i=0; i<alpha.size(); i++)
        {
            alpha[i] = rgba[i*4+3];
        }

        if (id == 0)
        {
            glGenTextures(1, &id);
        }

        if (dim == 1)
        {
            glBindTexture(GL_TEXTURE_1D, id);
        }
        else if (dim == 2)
        {
            glBindTexture(GL_TEXTURE_2D, id);
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
