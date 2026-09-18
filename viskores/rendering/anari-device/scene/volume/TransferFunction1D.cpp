//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "TransferFunction1D.h"
#include "array/ArrayConversion.h"
// Viskores
#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleStride.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/VolumeRendererStructured.h>
#include <viskores/worklet/WorkletMapField.h>

namespace
{

struct ResizeArrayWorklet : viskores::worklet::WorkletMapField
{
  ResizeArrayWorklet(viskores::Id inputSize, viskores::Id outputSize)
    : IndexScale(static_cast<viskores::Float32>(inputSize - 1) /
                 static_cast<viskores::Float32>(outputSize - 1))
  {
  }

  using ControlSignature = void(WholeArrayIn inputArray, FieldOut outputArray);
  using ExecutionSignature = void(OutputIndex, _1, _2);
  using InputDomain = _2;

  template <typename InputPortalType, typename T>
  VISKORES_EXEC void operator()(viskores::Id outputIndex,
                                const InputPortalType& inputPortal,
                                T& outputValue) const
  {
    viskores::Float32 scaledIndex = outputIndex * this->IndexScale;
    viskores::Id inputIndex = static_cast<viskores::Id>(viskores::Floor(scaledIndex));
    T leftValue = inputPortal.Get(inputIndex);
    T rightValue =
      inputPortal.Get(viskores::Min(inputIndex + 1, inputPortal.GetNumberOfValues() - 1));
    viskores::Float32 interp = scaledIndex - inputIndex;
    outputValue = viskores::Lerp(leftValue, rightValue, interp);
  }

  viskores::Float32 IndexScale;
};

template <typename T>
void ResizeArray(viskores::cont::ArrayHandle<T>& array, viskores::Id newSize)
{
  viskores::Id oldSize = array.GetNumberOfValues();

  if (oldSize == newSize)
  {
    return;
  }

  if (oldSize == 1)
  {
    T value = array.ReadPortal().Get(0);
    array.AllocateAndFill(newSize, value);
    return;
  }

  viskores::cont::ArrayHandle<T> resizedArray;
  resizedArray.Allocate(newSize);

  viskores::cont::Invoker invoke;
  invoke(ResizeArrayWorklet(array.GetNumberOfValues(), newSize), array, resizedArray);
  array = resizedArray;
}

struct AdjustAlphaWorklet : viskores::worklet::WorkletMapField
{
  AdjustAlphaWorklet(viskores::Float32 sampleDistance, viskores::Float32 unitDistance)
    : m_alphaSampleDistance(sampleDistance / unitDistance)
  {
  }

  using ControlSignature = void(FieldIn opacities, FieldInOut colors);
  VISKORES_EXEC void operator()(viskores::Float32 opacityParameter, viskores::Vec4f_32& color) const
  {
    // Multiply the opacity from the color parameter to the opacity from the opacity parameter.
    // Typically only one is set, in which case the other is set to 1.0. Setting both is weird,
    // but this is a rational response.
    viskores::Float32 opacity = opacityParameter * color[3];

    // The opacity given is based on the unit distance. We need to scale that
    // to be the opacity for the sampling distance of the ray caster. This
    // scaling is nonlinear.
    //
    // Opacity (α) scales exponentially with respect to the distance the ray
    // travels through the material (d).
    //
    // α = 1 - exp(-τ·d)
    //
    // where τ is the "transparency coefficient" based on the absorption of the
    // material, which is independent of the distance. Flipping this relationship,
    // we get
    //
    // τ = -(1/d)·ln(1-α).
    //
    // We are given a unit distance, d_u, and an opacity based on that
    // sampling distance, α_u. That means τ = -(1/d_u)·ln(1-α_u). Returning to
    // the first equation defining α and substituting this τ and the ray caster's
    // sample distance, d_s, we get the following opacity for the sample
    // distance.
    //
    // α_s = 1 - (1 - α_u)^(d_s/d_u)
    opacity =
      opacity >= 1.f ? 1.f : 1.f - viskores::Pow(1.f - opacity, this->m_alphaSampleDistance);

    // Store the opacity back in the color, which will be used for the transfer
    // function lookup.
    color[3] = opacity;
  }

  const viskores::Float32 m_alphaSampleDistance;
};

} // namespace

namespace viskores_device
{

TransferFunction1D::TransferFunction1D(ViskoresDeviceGlobalState* d)
  : Volume(d)
  , m_spatialField(this)
  , m_colorArray(this)
  , m_opacityArray(this)
{
}

void TransferFunction1D::commitParameters()
{
  this->Volume::commitParameters();

  this->m_spatialField = getParamObject<SpatialField>("value");

  this->m_unitDistance = getParam("unitDistance", 1.0f);

  this->m_colorArray = this->getParamObject<Array1D>("color");
  this->m_color = { 1, 1, 1, 1 };
  this->getParam("color", ANARI_FLOAT32_VEC4, &this->m_color);
  this->getParam("color", ANARI_FLOAT32_VEC3, &this->m_color);

  this->m_opacityArray = this->getParamObject<Array1D>("opacity");
  this->m_alpha = 1.0f;
  this->getParam("opacity", ANARI_FLOAT32, &this->m_alpha);

  box1 range = { 0, 1 };
  this->getParam("valueRange", ANARI_FLOAT32_BOX1, &range);
  this->m_valueRange = { range.lower, range.upper };
}

void TransferFunction1D::finalize()
{
  Volume::finalize();

  if (!this->m_spatialField)
  {
    reportMessage(ANARI_SEVERITY_WARNING, "'transferFunction1D' volume missing 'value' parameter");
    return;
  }

  // Determine sample distance (which also affects colors).
  viskores::cont::DataSet dataSet = this->m_spatialField->getDataSet();
  viskores::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  viskores::Float32 diagonalLength =
    static_cast<viskores::Float32>(viskores::Magnitude(bounds.MaxCorner() - bounds.MinCorner()));
  constexpr viskores::IdComponent numberOfSamples = 200;
  this->m_sampleDistance = static_cast<viskores::Float32>(diagonalLength / numberOfSamples);
  if (this->m_unitDistance <= 0.f)
  {
    this->reportMessage(ANARI_SEVERITY_WARNING,
                        "'unitDistance' must be positive; using a small positive value");
    this->m_unitDistance = 1e-6f;
  }

  if (this->m_colorArray)
  {
    // Convert to Viskores colors
    this->m_colorMap = ANARIColorsToViskoresColors(*this->m_colorArray);
  }
  else
  {
    this->m_colorMap.AllocateAndFill(1, this->m_color);
  }

  viskores::cont::Invoker invoke;
  if (this->m_opacityArray)
  {
    viskores::cont::ArrayHandle<viskores::Float32> alphaMap;
    this->m_opacityArray->dataAsViskoresArray().AsArrayHandle(alphaMap);

    if (alphaMap.GetNumberOfValues() < this->m_colorMap.GetNumberOfValues())
    {
      ResizeArray(alphaMap, this->m_colorMap.GetNumberOfValues());
    }
    else
    {
      ResizeArray(this->m_colorMap, alphaMap.GetNumberOfValues());
    }

    invoke(
      AdjustAlphaWorklet(this->m_sampleDistance, this->m_unitDistance), alphaMap, this->m_colorMap);
  }
  else
  {
    invoke(
      AdjustAlphaWorklet(this->m_sampleDistance, this->m_unitDistance),
      viskores::cont::make_ArrayHandleConstant(this->m_alpha, this->m_colorMap.GetNumberOfValues()),
      this->m_colorMap);
  }
}

void TransferFunction1D::render(viskores::rendering::Canvas& canvas,
                                const viskores::rendering::Camera& camera) const
{
  viskores::cont::DataSet dataSet = this->m_spatialField->getDataSet();
  const viskores::cont::Field& field = dataSet.GetField("data");
  viskores::cont::CoordinateSystem coords = dataSet.GetCoordinateSystem();

  viskores::cont::CellSetStructured<3> cellSet;
  try
  {
    dataSet.GetCellSet().AsCellSet(cellSet);
  }
  catch (viskores::cont::ErrorBadType)
  {
    this->reportMessage(ANARI_SEVERITY_ERROR,
                        "Transfer function 1D volume has bad cell set type from spatial field");
    return;
  }

  viskores::rendering::CanvasRayTracer* canvasRT =
    dynamic_cast<viskores::rendering::CanvasRayTracer*>(&canvas);
  if (canvasRT == nullptr)
  {
    this->reportMessage(ANARI_SEVERITY_ERROR, "Bad canvas detected for TransferFunction1D.");
    return;
  }

  viskores::rendering::raytracing::VolumeRendererStructured tracer;

  viskores::Int32 width = (viskores::Int32)canvas.GetWidth();
  viskores::Int32 height = (viskores::Int32)canvas.GetHeight();
  viskores::rendering::raytracing::Camera rayCamera = camera.CreateRaytracingCamera(width, height);

  viskores::rendering::raytracing::Ray<viskores::Float32> rays;
  rayCamera.CreateRays(rays, coords.GetBounds());
  rays.Buffers.at(0).InitConst(0.f);
  viskores::rendering::raytracing::RayOperations::MapCanvasToRays(
    rays, camera.CreateRaytracingCamera(width, height), canvasRT->GetDepthBuffer());

  tracer.SetSampleDistance(this->m_sampleDistance);

  tracer.SetData(coords, field, cellSet, this->m_valueRange);
  tracer.SetColorMap(this->m_colorMap);

  tracer.Render(rays);

  canvasRT->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera, false);
}

const SpatialField* TransferFunction1D::spatialField() const
{
  return this->m_spatialField.get();
}

viskores::Bounds TransferFunction1D::bounds() const
{
  return isValid() ? this->m_spatialField->getDataSet().GetCoordinateSystem().GetBounds()
                   : viskores::Bounds();
}

bool TransferFunction1D::isValid() const
{
  return this->m_spatialField;
}

} // namespace viskores_device
