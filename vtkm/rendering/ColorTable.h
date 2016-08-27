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
#ifndef vtk_m_rendering_ColorTable_h
#define vtk_m_rendering_ColorTable_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Color.h>

#include <string>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/shared_ptr.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace rendering {

namespace detail {

struct ColorTableInternals;

}

/// \brief It's a color table!
///
/// This class provides the basic representation of a color table. This class was
/// Ported from EAVL. Originally created by Jeremy Meredith, Dave Pugmire,
/// and Sean Ahern. This class uses seperate RGB and alpha control points and can
/// be used as a transfer function.
///
class ColorTable
{
private:
  boost::shared_ptr<detail::ColorTableInternals> Internals;

public:
  VTKM_RENDERING_EXPORT
  ColorTable();

  /// Constructs a \c ColorTable using the name of a pre-defined color set.
  VTKM_RENDERING_EXPORT
  ColorTable(const std::string &name);

  VTKM_RENDERING_EXPORT
  const std::string &GetName() const;

  VTKM_RENDERING_EXPORT
  bool GetSmooth() const;

  VTKM_RENDERING_EXPORT
  void Sample(int numSamples,
              vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > &colors) const;

  VTKM_RENDERING_EXPORT
  vtkm::rendering::Color MapRGB(vtkm::Float32 scalar) const;

  VTKM_RENDERING_EXPORT
  vtkm::Float32 MapAlpha(vtkm::Float32 scalar) const;

  VTKM_RENDERING_EXPORT
  void Clear();

  VTKM_RENDERING_EXPORT
  void Reverse();

  VTKM_RENDERING_EXPORT
  void AddControlPoint(vtkm::Float32 position,
                       const vtkm::rendering::Color &color);

  VTKM_RENDERING_EXPORT
  void AddControlPoint(vtkm::Float32 position,
                       const vtkm::rendering::Color &color,
                       vtkm::Float32 alpha);

  VTKM_RENDERING_EXPORT
  void AddAlphaControlPoint(vtkm::Float32 position, vtkm::Float32 alpha);
};
}}//namespace vtkm::rendering
#endif //vtk_m_rendering_ColorTable_h


