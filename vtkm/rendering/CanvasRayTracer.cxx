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

#include <vtkm/cont/TryExecute.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm {
namespace rendering {

namespace internal {

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

struct ClearBuffersInvokeFunctor
{
  typedef vtkm::rendering::Canvas::ColorBufferType ColorBufferType;
  typedef vtkm::rendering::Canvas::DepthBufferType DepthBufferType;

  ClearBuffers Worklet;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;

  ClearBuffersInvokeFunctor(const vtkm::rendering::Color &backgroundColor,
                            const ColorBufferType &colorBuffer,
                            const DepthBufferType &depthBuffer)
    : Worklet(backgroundColor),
      ColorBuffer(colorBuffer),
      DepthBuffer(depthBuffer)
  {  }

  template<typename Device>
  bool operator()(Device) const
  {
    vtkm::worklet::DispatcherMapField<ClearBuffers, Device>
        dispatcher(this->Worklet);
    dispatcher.Invoke( this->ColorBuffer, this->DepthBuffer);
    return true;
  }
};

} // namespace internal

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
  // TODO: Should the rendering library support policies or some other wayt to
  // configure with custom devices?
  vtkm::cont::TryExecute(
        internal::ClearBuffersInvokeFunctor(this->GetBackgroundColor(),
                                            this->GetColorBuffer(),
                                            this->GetDepthBuffer()));
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
