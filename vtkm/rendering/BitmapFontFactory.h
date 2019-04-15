//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_BitmapFontFactory_h
#define vtk_m_BitmapFontFactory_h

#include <vtkm/rendering/BitmapFont.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT BitmapFontFactory
{
public:
  static vtkm::rendering::BitmapFont CreateLiberation2Sans();
};
}
} //namespace vtkm::rendering

#endif
