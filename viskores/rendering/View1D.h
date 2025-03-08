//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_rendering_View1D_h
#define viskores_rendering_View1D_h

#include <viskores/rendering/AxisAnnotation2D.h>
#include <viskores/rendering/ColorLegendAnnotation.h>
#include <viskores/rendering/View.h>

namespace viskores
{
namespace rendering
{

/// @brief A view for a 1D data set.
///
/// 1D data are rendered as an X-Y plot with the values shone on the Y axis.
class VISKORES_RENDERING_EXPORT View1D : public viskores::rendering::View
{
public:
  View1D(
    const viskores::rendering::Scene& scene,
    const viskores::rendering::Mapper& mapper,
    const viskores::rendering::Canvas& canvas,
    const viskores::rendering::Color& backgroundColor = viskores::rendering::Color(0, 0, 0, 1),
    const viskores::rendering::Color& foregroundColor = viskores::rendering::Color(1, 1, 1, 1));

  View1D(
    const viskores::rendering::Scene& scene,
    const viskores::rendering::Mapper& mapper,
    const viskores::rendering::Canvas& canvas,
    const viskores::rendering::Camera& camera,
    const viskores::rendering::Color& backgroundColor = viskores::rendering::Color(0, 0, 0, 1),
    const viskores::rendering::Color& foregroundColor = viskores::rendering::Color(1, 1, 1, 1));

  void Paint() override;
  void RenderScreenAnnotations() override;
  void RenderWorldAnnotations() override;
  void RenderColorLegendAnnotations();

  void EnableLegend();
  void DisableLegend();
  void SetLegendLabelColor(viskores::rendering::Color c) { this->Legend.SetLabelColor(c); }

  /// @brief Specify whether log scaling should be used on the X axis.
  void SetLogX(bool l)
  {
    this->GetMapper().SetLogarithmX(l);
    this->LogX = l;
  }

  /// @brief Specify whether log scaling should be used on the Y axis.
  void SetLogY(bool l)
  {
    this->GetMapper().SetLogarithmY(l);
    this->LogY = l;
  }

private:
  void UpdateCameraProperties();

  // 1D-specific annotations
  viskores::rendering::AxisAnnotation2D HorizontalAxisAnnotation;
  viskores::rendering::AxisAnnotation2D VerticalAxisAnnotation;
  viskores::rendering::ColorLegendAnnotation Legend;
  bool LegendEnabled = true;
  bool LogX = false;
  bool LogY = false;
};
}
} // namespace viskores::rendering

#endif //viskores_rendering_View1D_h
