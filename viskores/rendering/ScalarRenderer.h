//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_rendering_ScalarRenderer_h
#define viskores_rendering_ScalarRenderer_h

#include <viskores/cont/DataSet.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/raytracing/Ray.h>

#include <memory>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT ScalarRenderer
{
public:
  ScalarRenderer();

  // Disable copying due to unique_ptr;
  ScalarRenderer(const ScalarRenderer&) = delete;
  ScalarRenderer& operator=(const ScalarRenderer&) = delete;

  ScalarRenderer(ScalarRenderer&&) noexcept;
  ScalarRenderer& operator=(ScalarRenderer&&) noexcept;
  // Default destructor implemented in source file to support PIMPL idiom.
  ~ScalarRenderer();

  void SetInput(viskores::cont::DataSet& dataSet);

  void SetWidth(viskores::Int32 width);
  void SetHeight(viskores::Int32 height);
  void SetDefaultValue(viskores::Float32 value);
  void SetLightPosition(const viskores::Vec3f_32& lightPosition);
  viskores::Vec3f_32 GetLightPosition() const;

  struct VISKORES_RENDERING_EXPORT Result
  {
    viskores::Int32 Width;
    viskores::Int32 Height;
    viskores::cont::ArrayHandle<viskores::Float32> Depths;
    std::vector<viskores::cont::ArrayHandle<viskores::Float32>> Scalars;
    std::vector<std::string> ScalarNames;
    std::map<std::string, viskores::Range> Ranges;

    viskores::cont::DataSet ToDataSet();
  };

  /// Render the scalar fields of the input as seen from a camera, with surface
  /// shading in the field "shading".
  ScalarRenderer::Result Render(const viskores::rendering::Camera& camera);

  /// Render the scalar fields of the input along rays that the caller created, e.g.,
  /// for a subset of the pixels. The pixel indices of the rays refer to an image of
  /// the width and height of this renderer. Without a camera, there is no surface
  /// shading.
  ScalarRenderer::Result Render(viskores::rendering::raytracing::Ray<viskores::Float32>& rays);

private:
  ScalarRenderer::Result RenderRays(viskores::rendering::raytracing::Ray<viskores::Float32>& rays,
                                    viskores::rendering::raytracing::Camera* camera);

  struct InternalsType;
  std::unique_ptr<InternalsType> Internals;
};
}
} //namespace viskores::rendering

#endif //viskores_rendering_ScalarRenderer_h
