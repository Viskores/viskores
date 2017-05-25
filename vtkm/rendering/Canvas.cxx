//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/Canvas.h>

#include <fstream>
#include <iostream>

namespace vtkm
{
namespace rendering
{

Canvas::Canvas(vtkm::Id width, vtkm::Id height)
  : Width(0)
  , Height(0)
{
  this->ResizeBuffers(width, height);
}

Canvas::~Canvas()
{
}

void Canvas::SaveAs(const std::string& fileName) const
{
  this->RefreshColorBuffer();
  std::ofstream of(fileName.c_str());
  of << "P6" << std::endl << this->Width << " " << this->Height << std::endl << 255 << std::endl;
  ColorBufferType::PortalConstControl colorPortal = this->ColorBuffer.GetPortalConstControl();
  for (vtkm::Id yIndex = this->Height - 1; yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < this->Width; xIndex++)
    {
      vtkm::Vec<vtkm::Float32, 4> tuple = colorPortal.Get(yIndex * this->Width + xIndex);
      of << (unsigned char)(tuple[0] * 255);
      of << (unsigned char)(tuple[1] * 255);
      of << (unsigned char)(tuple[2] * 255);
    }
  }
  of.close();
}

vtkm::rendering::WorldAnnotator* Canvas::CreateWorldAnnotator() const
{
  return new vtkm::rendering::WorldAnnotator;
}
}
} // vtkm::rendering
