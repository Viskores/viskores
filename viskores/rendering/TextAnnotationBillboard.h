//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_rendering_TextAnnotationBillboard_h
#define viskores_rendering_TextAnnotationBillboard_h

#include <viskores/rendering/TextAnnotation.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT TextAnnotationBillboard : public TextAnnotation
{
protected:
  viskores::Vec3f_32 Position;
  viskores::Float32 Angle;

public:
  TextAnnotationBillboard(const std::string& text,
                          const viskores::rendering::Color& color,
                          viskores::Float32 scalar,
                          const viskores::Vec3f_32& position,
                          viskores::Float32 angleDegrees = 0);

  void SetPosition(const viskores::Vec3f_32& position);

  void SetPosition(viskores::Float32 posx, viskores::Float32 posy, viskores::Float32 posz);

  void Render(const viskores::rendering::Camera& camera,
              const viskores::rendering::WorldAnnotator& worldAnnotator,
              viskores::rendering::Canvas& canvas) const override;
};
}
} // namespace viskores::rendering

#endif //viskores_rendering_TextAnnotationBillboard_h
