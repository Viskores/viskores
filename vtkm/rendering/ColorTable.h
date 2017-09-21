//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_ColorTable_h
#define vtk_m_rendering_ColorTable_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

namespace detail
{

struct ColorTableInternals;
}

/// \brief It's a color table!
///
/// This class provides the basic representation of a color table. This class was
/// Ported from EAVL. Originally created by Jeremy Meredith, Dave Pugmire,
/// and Sean Ahern. This class uses seperate RGB and alpha control points and can
/// be used as a transfer function.
///
class VTKM_RENDERING_EXPORT ColorTable
{
private:
  std::shared_ptr<detail::ColorTableInternals> Internals;

public:
  ColorTable();

  /// Constructs a \c ColorTable using the name of a pre-defined color set.
  ColorTable(const std::string& name);

  // Make a single color ColorTable.
  ColorTable(const vtkm::rendering::Color& color);

  const std::string& GetName() const;

  bool GetSmooth() const;

  void SetSmooth(bool smooth);

  void Sample(int numSamples, vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colors) const;

  vtkm::rendering::Color MapRGB(vtkm::Float32 scalar) const;

  vtkm::Float32 MapAlpha(vtkm::Float32 scalar) const;

  void Clear();

  void Reverse();

  ColorTable CorrectOpacity(const vtkm::Float32& factor) const;

  void AddControlPoint(vtkm::Float32 position, const vtkm::rendering::Color& color);

  void AddControlPoint(vtkm::Float32 position,
                       const vtkm::rendering::Color& color,
                       vtkm::Float32 alpha);

  void AddAlphaControlPoint(vtkm::Float32 position, vtkm::Float32 alpha);
};
}
} //namespace vtkm::rendering
#endif //vtk_m_rendering_ColorTable_h
