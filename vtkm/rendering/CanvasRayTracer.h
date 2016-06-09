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
#ifndef vtk_m_rendering_CanvasRayTracer_h
#define vtk_m_rendering_CanvasRayTracer_h

#include <vtkm/Types.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class CanvasRayTracer : public Canvas
{
public:
  VTKM_CONT_EXPORT
  CanvasRayTracer(vtkm::Id width=1024,
                  vtkm::Id height=1024,
                  const vtkm::rendering::Color &color =
                    vtkm::rendering::Color(0.0f,0.0f,0.0f,1.0f))
    : Canvas(width,height,color)
  {
  }

  class ClearBuffers : public vtkm::worklet::WorkletMapField
  {
    vtkm::rendering::Color ClearColor;
  public:
    VTKM_CONT_EXPORT
    ClearBuffers(const vtkm::rendering::Color &clearColor)
      : ClearColor(clearColor)
    {}
    typedef void ControlSignature(FieldOut<>, FieldOut<>);
    typedef void ExecutionSignature(_1, _2);
    VTKM_EXEC_EXPORT
    void operator()(vtkm::Vec<vtkm::Float32,4> &color,
                    vtkm::Float32 &depth) const
    {
      color = this->ClearColor.Components;
      depth = 1.001f;
    }
  }; //class ClearBuffers

  virtual void Initialize() {  }
  virtual void Activate() {  }
  virtual void Finish() {  }

  VTKM_CONT_EXPORT
  virtual void Clear()
  {
    vtkm::worklet::DispatcherMapField< ClearBuffers >(
          ClearBuffers( this->GetBackgroundColor() ) )
      .Invoke( this->GetColorBuffer(), this->GetDepthBuffer());
  }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasRayTracer_h
