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

#include <vtkm/rendering/CanvasRayTracer.h>

#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm {
namespace rendering {

namespace raytracerworklets {

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

} // namespace raytracerworklets

CanvasRayTracer::CanvasRayTracer(vtkm::Id width, vtkm::Id height)
  : Canvas(width, height)
{  }

void CanvasRayTracer::Initialize()
{
  // Nothing to initialize
}

void CanvasRayTracer::Activate()
{
  // Nothing to activate
}

void CanvasRayTracer::Finish()
{
  // Nothing to finish
}

void CanvasRayTracer::Clear()
{
  using raytracerworklets::ClearBuffers;

  // TODO: This should be put in a TryExecute and compiled with CUDA if
  // available. Perhaps policies should be created, too.
  vtkm::worklet::DispatcherMapField< ClearBuffers >(
        ClearBuffers( this->GetBackgroundColor() ) )
    .Invoke( this->GetColorBuffer(), this->GetDepthBuffer());
}

void CanvasRayTracer::AddLine(const vtkm::Vec<vtkm::Float64,2> &,
                              const vtkm::Vec<vtkm::Float64,2> &,
                              vtkm::Float32,
                              const vtkm::rendering::Color &) const
{
  // Not implemented
}

void CanvasRayTracer::AddColorBar(const vtkm::Bounds &,
                                  const vtkm::rendering::ColorTable &,
                                  bool ) const
{
  // Not implemented
}

void CanvasRayTracer::AddText(const vtkm::Vec<vtkm::Float32,2> &,
                              vtkm::Float32,
                              vtkm::Float32,
                              vtkm::Float32,
                              const vtkm::Vec<vtkm::Float32,2> &,
                              const vtkm::rendering::Color &,
                              const std::string &) const
{
  // Not implemented
}

}
}
