//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  vtkm::rendering::Canvas canvas;
  canvas.SetBackgroundColor(vtkm::rendering::Color::white);
  canvas.Initialize();
  canvas.Activate();
  canvas.Clear();
  canvas.AddLine(-0.8, 0.8, 0.8, 0.8, 1.0f, vtkm::rendering::Color::black);
  canvas.AddLine(0.8, 0.8, 0.8, -0.8, 1.0f, vtkm::rendering::Color::black);
  canvas.AddLine(0.8, -0.8, -0.8, -0.8, 1.0f, vtkm::rendering::Color::black);
  canvas.AddLine(-0.8, -0.8, -0.8, 0.8, 1.0f, vtkm::rendering::Color::black);
  canvas.AddLine(-0.8, -0.8, 0.8, 0.8, 1.0f, vtkm::rendering::Color::black);
  canvas.AddLine(-0.8, 0.8, 0.8, -0.8, 1.0f, vtkm::rendering::Color::black);
  canvas.BlendBackground();
  canvas.SaveAs("canvas.pnm");
}

} //namespace

int UnitTestCanvas(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RenderTests);
}
