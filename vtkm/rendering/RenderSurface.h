//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
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
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_RenderSurface_h
#define vtk_m_rendering_RenderSurface_h

#include <vtkm/Types.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/ColorTable.h>

#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class RenderSurface
{
public:
    VTKM_CONT_EXPORT
    RenderSurface(std::size_t width=1024, std::size_t height=1024,
                  const vtkm::rendering::Color &color=vtkm::rendering::Color(0.0f,0.0f,0.0f,1.0f))
        : Width(width), Height(height), BackgroundColor(color)
    {
      this->ColorBuffer.resize(width*height*4);
      this->DepthBuffer.resize(width*height);
    }

    VTKM_CONT_EXPORT
    virtual void Initialize() {}
    VTKM_CONT_EXPORT
    virtual void Activate() {}
    VTKM_CONT_EXPORT
    virtual void Clear() {}
    VTKM_CONT_EXPORT
    virtual void Finish() {}

    VTKM_CONT_EXPORT
    virtual void SetViewToWorldSpace(vtkm::rendering::View &, bool) {}
    VTKM_CONT_EXPORT
    virtual void SetViewToScreenSpace(vtkm::rendering::View &, bool) {}
    VTKM_CONT_EXPORT
    void SetViewportClipping(vtkm::rendering::View &, bool) {}

    VTKM_CONT_EXPORT
    virtual void SaveAs(const std::string &) {}

    virtual void AddLine(vtkm::Float64, vtkm::Float64,
                         vtkm::Float64, vtkm::Float64,
                         vtkm::Float32,
                         const vtkm::rendering::Color &) {}
    virtual void AddColorBar(vtkm::Float32, vtkm::Float32,
                             vtkm::Float32, vtkm::Float32, 
                             const vtkm::rendering::ColorTable &,
                             bool) {}
    virtual void AddText(vtkm::Float32, vtkm::Float32,
                         vtkm::Float32,
                         vtkm::Float32,
                         vtkm::Float32,
                         vtkm::Float32, vtkm::Float32,
                         Color,
                         std::string) {}

    std::size_t Width, Height;
    vtkm::rendering::Color BackgroundColor;
    std::vector<vtkm::Float32> ColorBuffer;
    std::vector<vtkm::Float32> DepthBuffer;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_RenderSurface_h
