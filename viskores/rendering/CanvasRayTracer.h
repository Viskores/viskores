//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_rendering_CanvasRayTracer_h
#define viskores_rendering_CanvasRayTracer_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/raytracing/Ray.h>

namespace viskores
{
namespace rendering
{

/// Represents the image space that is the target of rendering using the internal ray
/// tracing code.
class VISKORES_RENDERING_EXPORT CanvasRayTracer : public Canvas
{
public:
  /// Construct a canvas of a given width and height.
  CanvasRayTracer(viskores::Id width = 1024, viskores::Id height = 1024);

  ~CanvasRayTracer();

  viskores::rendering::Canvas* NewCopy() const override;

  void Clear() override;

  void WriteToCanvas(const viskores::rendering::raytracing::Ray<viskores::Float32>& rays,
                     const viskores::cont::ArrayHandle<viskores::Float32>& colors,
                     const viskores::rendering::Camera& camera,
                     bool writeDepth = true);

  void WriteToCanvas(const viskores::rendering::raytracing::Ray<viskores::Float64>& rays,
                     const viskores::cont::ArrayHandle<viskores::Float64>& colors,
                     const viskores::rendering::Camera& camera,
                     bool writeDepth = true);

  /// @brief Get the distances to the camera for each pixel.
  VISKORES_CONT
  const DepthBufferType& GetDistancesToCamera() const;

  /// @copydoc GetDistancesToCamera
  VISKORES_CONT
  DepthBufferType& GetDistancesToCamera();

  /// @brief Change the size of the image.
  VISKORES_CONT
  void ResizeBuffers(viskores::Id width, viskores::Id height) override;

private:
  DepthBufferType DistancesToCamera;
}; // class CanvasRayTracer
}
} // namespace viskores::rendering

#endif //viskores_rendering_CanvasRayTracer_h
