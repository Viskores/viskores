//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/Color.h>

namespace vtkm
{
namespace rendering
{

vtkm::rendering::Color vtkm::rendering::Color::black(0, 0, 0, 1);
vtkm::rendering::Color vtkm::rendering::Color::white(1, 1, 1, 1);

vtkm::rendering::Color vtkm::rendering::Color::red(1, 0, 0, 1);
vtkm::rendering::Color vtkm::rendering::Color::green(0, 1, 0, 1);
vtkm::rendering::Color vtkm::rendering::Color::blue(0, 0, 1, 1);

vtkm::rendering::Color vtkm::rendering::Color::cyan(0, 1, 1, 1);
vtkm::rendering::Color vtkm::rendering::Color::magenta(1, 0, 1, 1);
vtkm::rendering::Color vtkm::rendering::Color::yellow(1, 1, 0, 1);

vtkm::rendering::Color vtkm::rendering::Color::gray10(.1f, .1f, .1f, 1);
vtkm::rendering::Color vtkm::rendering::Color::gray20(.2f, .2f, .2f, 1);
vtkm::rendering::Color vtkm::rendering::Color::gray30(.3f, .3f, .3f, 1);
vtkm::rendering::Color vtkm::rendering::Color::gray40(.4f, .4f, .4f, 1);
vtkm::rendering::Color vtkm::rendering::Color::gray50(.5f, .5f, .5f, 1);
vtkm::rendering::Color vtkm::rendering::Color::gray60(.6f, .6f, .6f, 1);
vtkm::rendering::Color vtkm::rendering::Color::gray70(.7f, .7f, .7f, 1);
vtkm::rendering::Color vtkm::rendering::Color::gray80(.8f, .8f, .8f, 1);
vtkm::rendering::Color vtkm::rendering::Color::gray90(.9f, .9f, .9f, 1);
}
} // namespace vtkm::rendering
