//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/TextureGL.h>

#include <vtkm/rendering/internal/OpenGLHeaders.h>

namespace vtkm
{
namespace rendering
{

struct TextureGL::InternalsType
{
  GLuint Id;
  int Dimension;
  bool MIPMap;
  bool Linear2D;
  bool LinearMIP;

  VTKM_CONT
  InternalsType()
    : Id(0)
    , Dimension(0)
    , MIPMap(false)
    , Linear2D(true)
    , LinearMIP(true)
  {
  }

  VTKM_CONT
  ~InternalsType()
  {
    if (this->Id != 0)
    {
      glDeleteTextures(1, &this->Id);
    }
  }
};

TextureGL::TextureGL()
  : Internals(new InternalsType)
{
}

TextureGL::~TextureGL()
{
}

bool TextureGL::Valid() const
{
  return (this->Internals->Id != 0);
}

void TextureGL::Enable() const
{
  if (!this->Valid())
  {
    return;
  }

  if (this->Internals->Dimension == 1)
  {
    // no this->Internals->MIPMapping for 1D (at the moment)
    glBindTexture(GL_TEXTURE_1D, this->Internals->Id);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    if (this->Internals->Linear2D)
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
  else if (this->Internals->Dimension == 2)
  {
    glBindTexture(GL_TEXTURE_2D, this->Internals->Id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    if (this->Internals->Linear2D)
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      if (!this->Internals->MIPMap)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      else if (this->Internals->LinearMIP)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    }
    else
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      if (!this->Internals->MIPMap)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      else if (this->Internals->LinearMIP)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
      else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    }
    glEnable(GL_TEXTURE_2D);
  }
  else
  {
    // Fail silently for invalid dimension.
  }
}

void TextureGL::Disable() const
{
  if (this->Internals->Dimension == 1)
  {
    glDisable(GL_TEXTURE_1D);
  }
  else if (this->Internals->Dimension == 2)
  {
    glDisable(GL_TEXTURE_2D);
  }
  else
  {
    // Fail silently for invalid dimension
  }
}

void TextureGL::CreateAlphaFromRGBA(vtkm::Id width,
                                    vtkm::Id height,
                                    const std::vector<unsigned char>& rgba)
{
  this->Internals->Dimension = 2;
  std::vector<unsigned char> alpha(rgba.size() / 4);
  VTKM_ASSERT(width * height == static_cast<vtkm::Id>(alpha.size()));
  for (std::size_t i = 0; i < alpha.size(); i++)
  {
    alpha[i] = rgba[i * 4 + 3];
  }

  if (this->Internals->Id == 0)
  {
    glGenTextures(1, &this->Internals->Id);
  }

  if (this->Internals->Dimension == 1)
  {
    glBindTexture(GL_TEXTURE_1D, this->Internals->Id);
  }
  else if (this->Internals->Dimension == 2)
  {
    glBindTexture(GL_TEXTURE_2D, this->Internals->Id);
//#define HW_MIPMAPS
#ifdef HW_MIPMAPS
    mpimap = true;
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
#endif
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_ALPHA,
                 static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height),
                 0,
                 GL_ALPHA,
                 GL_UNSIGNED_BYTE,
                 (void*)(&(alpha[0])));
  }
}
}
} // namespace vtkm::rendering
