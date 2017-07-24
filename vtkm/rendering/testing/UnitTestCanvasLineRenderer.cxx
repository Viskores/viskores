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

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/CanvasLineRenderer.h>

namespace
{

template <typename T, vtkm::IdComponent FromSize, vtkm::IdComponent ToSize>
vtkm::Vec<T, ToSize> ShrinkVec(const vtkm::Vec<T, FromSize>& inputVec)
{
  vtkm::Vec<T, ToSize> ret;
  for (vtkm::IdComponent idx = 0; idx < ToSize; ++idx)
  {
    ret[idx] = inputVec[idx];
  }
  return ret;
}

void LineTests()
{
  vtkm::rendering::CanvasLineRenderer canvas(1000, 1000);
  canvas.Initialize();
  canvas.Activate();
  canvas.SetBackgroundColor(vtkm::rendering::Color::white);
  canvas.Clear();
  canvas.AddLine(
    vtkm::make_Vec(0.0f, 0.0f), vtkm::make_Vec(999.0f, 999.0f), 1.0f, vtkm::rendering::Color::red);
  canvas.Finish();

  vtkm::rendering::Canvas::ColorBufferType colorBuffer = canvas.GetColorBuffer();
  vtkm::Id idx = 500 * canvas.GetWidth() + 500;
  vtkm::Vec<vtkm::Float32, 3> actualColor =
    ShrinkVec<vtkm::Float32, 4, 3>(colorBuffer.GetPortalConstControl().Get(idx));
  vtkm::Vec<vtkm::Float32, 3> expectedColor =
    ShrinkVec<vtkm::Float32, 4, 3>(vtkm::rendering::Color::red.Components);
  VTKM_TEST_ASSERT(actualColor == expectedColor, "Line color not correct");
}

} //namespace

int UnitTestCanvasLineRenderer(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(LineTests);
}
