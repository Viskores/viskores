//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_rendering_DecodePNG_h
#define vtk_m_rendering_DecodePNG_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vector>

namespace vtkm
{
namespace rendering
{

VTKM_RENDERING_EXPORT
int DecodePNG(std::vector<unsigned char>& out_image,
              unsigned long& image_width,
              unsigned long& image_height,
              const unsigned char* in_png,
              std::size_t in_size,
              bool convert_to_rgba32 = true);
}
} // vtkm::rendering

#endif //vtk_m_rendering_DecodePNG_h
