//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
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
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/ColorTable.h>

#include <vtkm/Math.h>

#include <string>
#include <vector>

namespace vtkm
{
namespace rendering
{

namespace detail
{

struct ColorControlPoint
{
  vtkm::Float32 Position;
  vtkm::rendering::Color RGBA;
  ColorControlPoint(vtkm::Float32 position, const vtkm::rendering::Color& rgba)
    : Position(position)
    , RGBA(rgba)
  {
  }
};

struct AlphaControlPoint
{
  vtkm::Float32 Position;
  vtkm::Float32 AlphaValue;
  AlphaControlPoint(vtkm::Float32 position, const vtkm::Float32& alphaValue)
    : Position(position)
    , AlphaValue(alphaValue)
  {
  }
};

struct ColorTableInternals
{
  std::string UniqueName;
  bool Smooth;
  std::vector<ColorControlPoint> RGBPoints;
  std::vector<AlphaControlPoint> AlphaPoints;
};

} // namespace detail

ColorTable::ColorTable()
  : Internals(new detail::ColorTableInternals)
{
  this->Internals->UniqueName = "";
  this->Internals->Smooth = false;
}

const std::string& ColorTable::GetName() const
{
  return this->Internals->UniqueName;
}

bool ColorTable::GetSmooth() const
{
  return this->Internals->Smooth;
}

void ColorTable::SetSmooth(bool smooth)
{
  this->Internals->Smooth = smooth;
}

void ColorTable::Sample(int numSamples,
                        vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colors) const
{
  colors.Allocate(numSamples);

  for (vtkm::Id i = 0; i < numSamples; i++)
  {
    vtkm::Vec<vtkm::Float32, 4> color;
    Color c = MapRGB(static_cast<vtkm::Float32>(i) / static_cast<vtkm::Float32>(numSamples - 1));
    color[0] = c.Components[0];
    color[1] = c.Components[1];
    color[2] = c.Components[2];
    color[3] = MapAlpha(static_cast<vtkm::Float32>(i) / static_cast<vtkm::Float32>(numSamples - 1));
    colors.GetPortalControl().Set(i, color);
  }
}

vtkm::rendering::Color ColorTable::MapRGB(vtkm::Float32 scalar) const
{
  std::size_t numControlPoints = this->Internals->RGBPoints.size();
  if (numControlPoints == 0)
  {
    return Color(0.5f, 0.5f, 0.5f);
  }
  if ((numControlPoints == 1) || (scalar <= this->Internals->RGBPoints[0].Position))
  {
    return this->Internals->RGBPoints[0].RGBA;
  }
  if (scalar >= this->Internals->RGBPoints[numControlPoints - 1].Position)
  {
    return this->Internals->RGBPoints[numControlPoints - 1].RGBA;
  }

  std::size_t secondColorIndex;
  for (secondColorIndex = 1; secondColorIndex < numControlPoints - 1; secondColorIndex++)
  {
    if (scalar < this->Internals->RGBPoints[secondColorIndex].Position)
    {
      break;
    }
  }

  std::size_t firstColorIndex = secondColorIndex - 1;
  vtkm::Float32 seg = this->Internals->RGBPoints[secondColorIndex].Position -
    this->Internals->RGBPoints[firstColorIndex].Position;
  vtkm::Float32 alpha;
  if (seg == 0.f)
  {
    alpha = .5f;
  }
  else
  {
    alpha = (scalar - this->Internals->RGBPoints[firstColorIndex].Position) / seg;
  }

  const vtkm::rendering::Color& firstColor = this->Internals->RGBPoints[firstColorIndex].RGBA;
  const vtkm::rendering::Color& secondColor = this->Internals->RGBPoints[secondColorIndex].RGBA;
  if (this->Internals->Smooth)
  {
    return vtkm::rendering::Color(firstColor.Components * (1.0f - alpha) +
                                  secondColor.Components * alpha);
  }
  else
  {
    if (alpha < .5)
    {
      return firstColor;
    }
    else
    {
      return secondColor;
    }
  }
}

vtkm::Float32 ColorTable::MapAlpha(vtkm::Float32 scalar) const
{
  std::size_t numControlPoints = this->Internals->AlphaPoints.size();
  // If no alpha control points were set, just return full opacity
  if (numControlPoints == 0)
  {
    return 1.f;
  }
  if ((numControlPoints == 1) || (scalar <= this->Internals->AlphaPoints[0].Position))
  {
    return this->Internals->AlphaPoints[0].AlphaValue;
  }
  if (scalar >= this->Internals->AlphaPoints[numControlPoints - 1].Position)
  {
    return this->Internals->AlphaPoints[numControlPoints - 1].AlphaValue;
  }

  std::size_t secondColorIndex;
  for (secondColorIndex = 1; secondColorIndex < numControlPoints - 1; secondColorIndex++)
  {
    if (scalar < this->Internals->AlphaPoints[secondColorIndex].Position)
    {
      break;
    }
  }

  std::size_t firstColorIndex = secondColorIndex - 1;
  vtkm::Float32 seg = this->Internals->AlphaPoints[secondColorIndex].Position -
    this->Internals->AlphaPoints[firstColorIndex].Position;
  vtkm::Float32 alpha;
  if (seg == 0.f)
  {
    alpha = .5;
  }
  else
  {
    alpha = (scalar - this->Internals->AlphaPoints[firstColorIndex].Position) / seg;
  }

  vtkm::Float32 firstAlpha = this->Internals->AlphaPoints[firstColorIndex].AlphaValue;
  vtkm::Float32 secondAlpha = this->Internals->AlphaPoints[secondColorIndex].AlphaValue;
  if (this->Internals->Smooth)
  {
    return (firstAlpha * (1.f - alpha) + secondAlpha * alpha);
  }
  else
  {
    if (alpha < .5)
    {
      return firstAlpha;
    }
    else
    {
      return secondAlpha;
    }
  }
}

void ColorTable::Clear()
{
  this->Internals.reset(new detail::ColorTableInternals);
  this->Internals->UniqueName = "";
  this->Internals->Smooth = false;
}

ColorTable ColorTable::CorrectOpacity(const vtkm::Float32& factor) const
{
  ColorTable corrected;
  corrected.SetSmooth(this->Internals->Smooth);
  size_t rgbSize = this->Internals->RGBPoints.size();
  for (size_t i = 0; i < rgbSize; ++i)
  {
    detail::ColorControlPoint point = this->Internals->RGBPoints.at(i);
    corrected.AddControlPoint(point.Position, point.RGBA);
  }

  size_t alphaSize = this->Internals->AlphaPoints.size();
  for (size_t i = 0; i < alphaSize; ++i)
  {
    detail::AlphaControlPoint point = this->Internals->AlphaPoints.at(i);
    vtkm::Float32 alpha = 1.f - vtkm::Pow((1.f - point.AlphaValue), factor);
    corrected.AddAlphaControlPoint(point.Position, alpha);
  }

  return corrected;
}

void ColorTable::Reverse()
{
  std::shared_ptr<detail::ColorTableInternals> oldInternals = this->Internals;

  this->Clear();

  std::size_t vectorSize = oldInternals->RGBPoints.size();
  for (std::size_t i = 0; i < vectorSize; --i)
  {
    std::size_t oldIndex = vectorSize - i - 1;
    AddControlPoint(1.0f - oldInternals->RGBPoints[oldIndex].Position,
                    oldInternals->RGBPoints[oldIndex].RGBA);
  }

  vectorSize = oldInternals->AlphaPoints.size();
  for (std::size_t i = 0; i < vectorSize; --i)
  {
    std::size_t oldIndex = vectorSize - i - 1;
    AddAlphaControlPoint(1.0f - oldInternals->AlphaPoints[oldIndex].Position,
                         oldInternals->AlphaPoints[oldIndex].AlphaValue);
  }

  this->Internals->Smooth = oldInternals->Smooth;
  this->Internals->UniqueName = oldInternals->UniqueName;
  if (this->Internals->UniqueName[1] == '0')
  {
    this->Internals->UniqueName[1] = '1';
  }
  else
  {
    this->Internals->UniqueName[1] = '0';
  }
}

void ColorTable::AddControlPoint(vtkm::Float32 position, const vtkm::rendering::Color& color)
{
  this->Internals->RGBPoints.push_back(detail::ColorControlPoint(position, color));
}

void ColorTable::AddControlPoint(vtkm::Float32 position,
                                 const vtkm::rendering::Color& color,
                                 vtkm::Float32 alpha)
{
  this->Internals->RGBPoints.push_back(detail::ColorControlPoint(position, color));
  this->Internals->AlphaPoints.push_back(detail::AlphaControlPoint(position, alpha));
}

void ColorTable::AddAlphaControlPoint(vtkm::Float32 position, vtkm::Float32 alpha)
{
  this->Internals->AlphaPoints.push_back(detail::AlphaControlPoint(position, alpha));
}

ColorTable::ColorTable(const std::string& name_)
  : Internals(new detail::ColorTableInternals)
{
  std::string name = name_;
  if (name == "" || name == "default")
  {
    name = "cool2warm";
  }

  this->Internals->Smooth = true;
  if (name == "grey" || name == "gray")
  {
    AddControlPoint(0.0f, Color(0.f, 0.f, 0.f));
    AddControlPoint(1.0f, Color(1.f, 1.f, 1.f));
  }
  else if (name == "blue")
  {
    AddControlPoint(0.00f, Color(0.f, 0.f, 0.f));
    AddControlPoint(0.33f, Color(0.f, 0.f, .5f));
    AddControlPoint(0.66f, Color(0.f, .5f, 1.f));
    AddControlPoint(1.00f, Color(1.f, 1.f, 1.f));
  }
  else if (name == "orange")
  {
    AddControlPoint(0.00f, Color(0.f, 0.f, 0.f));
    AddControlPoint(0.33f, Color(.5f, 0.f, 0.f));
    AddControlPoint(0.66f, Color(1.f, .5f, 0.f));
    AddControlPoint(1.00f, Color(1.f, 1.f, 1.f));
  }
  else if (name == "cool2warm")
  {
    AddControlPoint(0.0f, Color(0.3347f, 0.2830f, 0.7564f));
    AddControlPoint(0.0039f, Color(0.3389f, 0.2901f, 0.7627f));
    AddControlPoint(0.0078f, Color(0.3432f, 0.2972f, 0.7688f));
    AddControlPoint(0.0117f, Color(0.3474f, 0.3043f, 0.7749f));
    AddControlPoint(0.0156f, Color(0.3516f, 0.3113f, 0.7809f));
    AddControlPoint(0.0196f, Color(0.3558f, 0.3183f, 0.7869f));
    AddControlPoint(0.0235f, Color(0.3600f, 0.3253f, 0.7928f));
    AddControlPoint(0.0274f, Color(0.3642f, 0.3323f, 0.7986f));
    AddControlPoint(0.0313f, Color(0.3684f, 0.3392f, 0.8044f));
    AddControlPoint(0.0352f, Color(0.3727f, 0.3462f, 0.8101f));
    AddControlPoint(0.0392f, Color(0.3769f, 0.3531f, 0.8157f));
    AddControlPoint(0.0431f, Color(0.3811f, 0.3600f, 0.8213f));
    AddControlPoint(0.0470f, Color(0.3853f, 0.3669f, 0.8268f));
    AddControlPoint(0.0509f, Color(0.3896f, 0.3738f, 0.8322f));
    AddControlPoint(0.0549f, Color(0.3938f, 0.3806f, 0.8375f));
    AddControlPoint(0.0588f, Color(0.3980f, 0.3874f, 0.8428f));
    AddControlPoint(0.0627f, Color(0.4023f, 0.3942f, 0.8480f));
    AddControlPoint(0.0666f, Color(0.4065f, 0.4010f, 0.8531f));
    AddControlPoint(0.0705f, Color(0.4108f, 0.4078f, 0.8582f));
    AddControlPoint(0.0745f, Color(0.4151f, 0.4145f, 0.8632f));
    AddControlPoint(0.0784f, Color(0.4193f, 0.4212f, 0.8680f));
    AddControlPoint(0.0823f, Color(0.4236f, 0.4279f, 0.8729f));
    AddControlPoint(0.0862f, Color(0.4279f, 0.4346f, 0.8776f));
    AddControlPoint(0.0901f, Color(0.4321f, 0.4412f, 0.8823f));
    AddControlPoint(0.0941f, Color(0.4364f, 0.4479f, 0.8868f));
    AddControlPoint(0.0980f, Color(0.4407f, 0.4544f, 0.8913f));
    AddControlPoint(0.1019f, Color(0.4450f, 0.4610f, 0.8957f));
    AddControlPoint(0.1058f, Color(0.4493f, 0.4675f, 0.9001f));
    AddControlPoint(0.1098f, Color(0.4536f, 0.4741f, 0.9043f));
    AddControlPoint(0.1137f, Color(0.4579f, 0.4805f, 0.9085f));
    AddControlPoint(0.1176f, Color(0.4622f, 0.4870f, 0.9126f));
    AddControlPoint(0.1215f, Color(0.4666f, 0.4934f, 0.9166f));
    AddControlPoint(0.1254f, Color(0.4709f, 0.4998f, 0.9205f));
    AddControlPoint(0.1294f, Color(0.4752f, 0.5061f, 0.9243f));
    AddControlPoint(0.1333f, Color(0.4796f, 0.5125f, 0.9280f));
    AddControlPoint(0.1372f, Color(0.4839f, 0.5188f, 0.9317f));
    AddControlPoint(0.1411f, Color(0.4883f, 0.5250f, 0.9352f));
    AddControlPoint(0.1450f, Color(0.4926f, 0.5312f, 0.9387f));
    AddControlPoint(0.1490f, Color(0.4970f, 0.5374f, 0.9421f));
    AddControlPoint(0.1529f, Color(0.5013f, 0.5436f, 0.9454f));
    AddControlPoint(0.1568f, Color(0.5057f, 0.5497f, 0.9486f));
    AddControlPoint(0.1607f, Color(0.5101f, 0.5558f, 0.9517f));
    AddControlPoint(0.1647f, Color(0.5145f, 0.5618f, 0.9547f));
    AddControlPoint(0.1686f, Color(0.5188f, 0.5678f, 0.9577f));
    AddControlPoint(0.1725f, Color(0.5232f, 0.5738f, 0.9605f));
    AddControlPoint(0.1764f, Color(0.5276f, 0.5797f, 0.9633f));
    AddControlPoint(0.1803f, Color(0.5320f, 0.5856f, 0.9659f));
    AddControlPoint(0.1843f, Color(0.5364f, 0.5915f, 0.9685f));
    AddControlPoint(0.1882f, Color(0.5408f, 0.5973f, 0.9710f));
    AddControlPoint(0.1921f, Color(0.5452f, 0.6030f, 0.9733f));
    AddControlPoint(0.1960f, Color(0.5497f, 0.6087f, 0.9756f));
    AddControlPoint(0.2f, Color(0.5541f, 0.6144f, 0.9778f));
    AddControlPoint(0.2039f, Color(0.5585f, 0.6200f, 0.9799f));
    AddControlPoint(0.2078f, Color(0.5629f, 0.6256f, 0.9819f));
    AddControlPoint(0.2117f, Color(0.5673f, 0.6311f, 0.9838f));
    AddControlPoint(0.2156f, Color(0.5718f, 0.6366f, 0.9856f));
    AddControlPoint(0.2196f, Color(0.5762f, 0.6420f, 0.9873f));
    AddControlPoint(0.2235f, Color(0.5806f, 0.6474f, 0.9890f));
    AddControlPoint(0.2274f, Color(0.5850f, 0.6528f, 0.9905f));
    AddControlPoint(0.2313f, Color(0.5895f, 0.6580f, 0.9919f));
    AddControlPoint(0.2352f, Color(0.5939f, 0.6633f, 0.9932f));
    AddControlPoint(0.2392f, Color(0.5983f, 0.6685f, 0.9945f));
    AddControlPoint(0.2431f, Color(0.6028f, 0.6736f, 0.9956f));
    AddControlPoint(0.2470f, Color(0.6072f, 0.6787f, 0.9967f));
    AddControlPoint(0.2509f, Color(0.6116f, 0.6837f, 0.9976f));
    AddControlPoint(0.2549f, Color(0.6160f, 0.6887f, 0.9985f));
    AddControlPoint(0.2588f, Color(0.6205f, 0.6936f, 0.9992f));
    AddControlPoint(0.2627f, Color(0.6249f, 0.6984f, 0.9999f));
    AddControlPoint(0.2666f, Color(0.6293f, 0.7032f, 1.0004f));
    AddControlPoint(0.2705f, Color(0.6337f, 0.7080f, 1.0009f));
    AddControlPoint(0.2745f, Color(0.6381f, 0.7127f, 1.0012f));
    AddControlPoint(0.2784f, Color(0.6425f, 0.7173f, 1.0015f));
    AddControlPoint(0.2823f, Color(0.6469f, 0.7219f, 1.0017f));
    AddControlPoint(0.2862f, Color(0.6513f, 0.7264f, 1.0017f));
    AddControlPoint(0.2901f, Color(0.6557f, 0.7308f, 1.0017f));
    AddControlPoint(0.2941f, Color(0.6601f, 0.7352f, 1.0016f));
    AddControlPoint(0.2980f, Color(0.6645f, 0.7395f, 1.0014f));
    AddControlPoint(0.3019f, Color(0.6688f, 0.7438f, 1.0010f));
    AddControlPoint(0.3058f, Color(0.6732f, 0.7480f, 1.0006f));
    AddControlPoint(0.3098f, Color(0.6775f, 0.7521f, 1.0001f));
    AddControlPoint(0.3137f, Color(0.6819f, 0.7562f, 0.9995f));
    AddControlPoint(0.3176f, Color(0.6862f, 0.7602f, 0.9988f));
    AddControlPoint(0.3215f, Color(0.6905f, 0.7641f, 0.9980f));
    AddControlPoint(0.3254f, Color(0.6948f, 0.7680f, 0.9971f));
    AddControlPoint(0.3294f, Color(0.6991f, 0.7718f, 0.9961f));
    AddControlPoint(0.3333f, Color(0.7034f, 0.7755f, 0.9950f));
    AddControlPoint(0.3372f, Color(0.7077f, 0.7792f, 0.9939f));
    AddControlPoint(0.3411f, Color(0.7119f, 0.7828f, 0.9926f));
    AddControlPoint(0.3450f, Color(0.7162f, 0.7864f, 0.9912f));
    AddControlPoint(0.3490f, Color(0.7204f, 0.7898f, 0.9897f));
    AddControlPoint(0.3529f, Color(0.7246f, 0.7932f, 0.9882f));
    AddControlPoint(0.3568f, Color(0.7288f, 0.7965f, 0.9865f));
    AddControlPoint(0.3607f, Color(0.7330f, 0.7998f, 0.9848f));
    AddControlPoint(0.3647f, Color(0.7372f, 0.8030f, 0.9829f));
    AddControlPoint(0.3686f, Color(0.7413f, 0.8061f, 0.9810f));
    AddControlPoint(0.3725f, Color(0.7455f, 0.8091f, 0.9789f));
    AddControlPoint(0.3764f, Color(0.7496f, 0.8121f, 0.9768f));
    AddControlPoint(0.3803f, Color(0.7537f, 0.8150f, 0.9746f));
    AddControlPoint(0.3843f, Color(0.7577f, 0.8178f, 0.9723f));
    AddControlPoint(0.3882f, Color(0.7618f, 0.8205f, 0.9699f));
    AddControlPoint(0.3921f, Color(0.7658f, 0.8232f, 0.9674f));
    AddControlPoint(0.3960f, Color(0.7698f, 0.8258f, 0.9648f));
    AddControlPoint(0.4f, Color(0.7738f, 0.8283f, 0.9622f));
    AddControlPoint(0.4039f, Color(0.7777f, 0.8307f, 0.9594f));
    AddControlPoint(0.4078f, Color(0.7817f, 0.8331f, 0.9566f));
    AddControlPoint(0.4117f, Color(0.7856f, 0.8353f, 0.9536f));
    AddControlPoint(0.4156f, Color(0.7895f, 0.8375f, 0.9506f));
    AddControlPoint(0.4196f, Color(0.7933f, 0.8397f, 0.9475f));
    AddControlPoint(0.4235f, Color(0.7971f, 0.8417f, 0.9443f));
    AddControlPoint(0.4274f, Color(0.8009f, 0.8437f, 0.9410f));
    AddControlPoint(0.4313f, Color(0.8047f, 0.8456f, 0.9376f));
    AddControlPoint(0.4352f, Color(0.8085f, 0.8474f, 0.9342f));
    AddControlPoint(0.4392f, Color(0.8122f, 0.8491f, 0.9306f));
    AddControlPoint(0.4431f, Color(0.8159f, 0.8507f, 0.9270f));
    AddControlPoint(0.4470f, Color(0.8195f, 0.8523f, 0.9233f));
    AddControlPoint(0.4509f, Color(0.8231f, 0.8538f, 0.9195f));
    AddControlPoint(0.4549f, Color(0.8267f, 0.8552f, 0.9156f));
    AddControlPoint(0.4588f, Color(0.8303f, 0.8565f, 0.9117f));
    AddControlPoint(0.4627f, Color(0.8338f, 0.8577f, 0.9076f));
    AddControlPoint(0.4666f, Color(0.8373f, 0.8589f, 0.9035f));
    AddControlPoint(0.4705f, Color(0.8407f, 0.8600f, 0.8993f));
    AddControlPoint(0.4745f, Color(0.8441f, 0.8610f, 0.8950f));
    AddControlPoint(0.4784f, Color(0.8475f, 0.8619f, 0.8906f));
    AddControlPoint(0.4823f, Color(0.8508f, 0.8627f, 0.8862f));
    AddControlPoint(0.4862f, Color(0.8541f, 0.8634f, 0.8817f));
    AddControlPoint(0.4901f, Color(0.8574f, 0.8641f, 0.8771f));
    AddControlPoint(0.4941f, Color(0.8606f, 0.8647f, 0.8724f));
    AddControlPoint(0.4980f, Color(0.8638f, 0.8651f, 0.8677f));
    AddControlPoint(0.5019f, Color(0.8673f, 0.8645f, 0.8626f));
    AddControlPoint(0.5058f, Color(0.8710f, 0.8627f, 0.8571f));
    AddControlPoint(0.5098f, Color(0.8747f, 0.8609f, 0.8515f));
    AddControlPoint(0.5137f, Color(0.8783f, 0.8589f, 0.8459f));
    AddControlPoint(0.5176f, Color(0.8818f, 0.8569f, 0.8403f));
    AddControlPoint(0.5215f, Color(0.8852f, 0.8548f, 0.8347f));
    AddControlPoint(0.5254f, Color(0.8885f, 0.8526f, 0.8290f));
    AddControlPoint(0.5294f, Color(0.8918f, 0.8504f, 0.8233f));
    AddControlPoint(0.5333f, Color(0.8949f, 0.8480f, 0.8176f));
    AddControlPoint(0.5372f, Color(0.8980f, 0.8456f, 0.8119f));
    AddControlPoint(0.5411f, Color(0.9010f, 0.8431f, 0.8061f));
    AddControlPoint(0.5450f, Color(0.9040f, 0.8405f, 0.8003f));
    AddControlPoint(0.5490f, Color(0.9068f, 0.8378f, 0.7944f));
    AddControlPoint(0.5529f, Color(0.9096f, 0.8351f, 0.7886f));
    AddControlPoint(0.5568f, Color(0.9123f, 0.8322f, 0.7827f));
    AddControlPoint(0.5607f, Color(0.9149f, 0.8293f, 0.7768f));
    AddControlPoint(0.5647f, Color(0.9174f, 0.8263f, 0.7709f));
    AddControlPoint(0.5686f, Color(0.9198f, 0.8233f, 0.7649f));
    AddControlPoint(0.5725f, Color(0.9222f, 0.8201f, 0.7590f));
    AddControlPoint(0.5764f, Color(0.9245f, 0.8169f, 0.7530f));
    AddControlPoint(0.5803f, Color(0.9266f, 0.8136f, 0.7470f));
    AddControlPoint(0.5843f, Color(0.9288f, 0.8103f, 0.7410f));
    AddControlPoint(0.5882f, Color(0.9308f, 0.8068f, 0.7349f));
    AddControlPoint(0.5921f, Color(0.9327f, 0.8033f, 0.7289f));
    AddControlPoint(0.5960f, Color(0.9346f, 0.7997f, 0.7228f));
    AddControlPoint(0.6f, Color(0.9363f, 0.7960f, 0.7167f));
    AddControlPoint(0.6039f, Color(0.9380f, 0.7923f, 0.7106f));
    AddControlPoint(0.6078f, Color(0.9396f, 0.7884f, 0.7045f));
    AddControlPoint(0.6117f, Color(0.9412f, 0.7845f, 0.6984f));
    AddControlPoint(0.6156f, Color(0.9426f, 0.7806f, 0.6923f));
    AddControlPoint(0.6196f, Color(0.9439f, 0.7765f, 0.6861f));
    AddControlPoint(0.6235f, Color(0.9452f, 0.7724f, 0.6800f));
    AddControlPoint(0.6274f, Color(0.9464f, 0.7682f, 0.6738f));
    AddControlPoint(0.6313f, Color(0.9475f, 0.7640f, 0.6677f));
    AddControlPoint(0.6352f, Color(0.9485f, 0.7596f, 0.6615f));
    AddControlPoint(0.6392f, Color(0.9495f, 0.7552f, 0.6553f));
    AddControlPoint(0.6431f, Color(0.9503f, 0.7508f, 0.6491f));
    AddControlPoint(0.6470f, Color(0.9511f, 0.7462f, 0.6429f));
    AddControlPoint(0.6509f, Color(0.9517f, 0.7416f, 0.6368f));
    AddControlPoint(0.6549f, Color(0.9523f, 0.7369f, 0.6306f));
    AddControlPoint(0.6588f, Color(0.9529f, 0.7322f, 0.6244f));
    AddControlPoint(0.6627f, Color(0.9533f, 0.7274f, 0.6182f));
    AddControlPoint(0.6666f, Color(0.9536f, 0.7225f, 0.6120f));
    AddControlPoint(0.6705f, Color(0.9539f, 0.7176f, 0.6058f));
    AddControlPoint(0.6745f, Color(0.9541f, 0.7126f, 0.5996f));
    AddControlPoint(0.6784f, Color(0.9542f, 0.7075f, 0.5934f));
    AddControlPoint(0.6823f, Color(0.9542f, 0.7023f, 0.5873f));
    AddControlPoint(0.6862f, Color(0.9541f, 0.6971f, 0.5811f));
    AddControlPoint(0.6901f, Color(0.9539f, 0.6919f, 0.5749f));
    AddControlPoint(0.6941f, Color(0.9537f, 0.6865f, 0.5687f));
    AddControlPoint(0.6980f, Color(0.9534f, 0.6811f, 0.5626f));
    AddControlPoint(0.7019f, Color(0.9529f, 0.6757f, 0.5564f));
    AddControlPoint(0.7058f, Color(0.9524f, 0.6702f, 0.5503f));
    AddControlPoint(0.7098f, Color(0.9519f, 0.6646f, 0.5441f));
    AddControlPoint(0.7137f, Color(0.9512f, 0.6589f, 0.5380f));
    AddControlPoint(0.7176f, Color(0.9505f, 0.6532f, 0.5319f));
    AddControlPoint(0.7215f, Color(0.9496f, 0.6475f, 0.5258f));
    AddControlPoint(0.7254f, Color(0.9487f, 0.6416f, 0.5197f));
    AddControlPoint(0.7294f, Color(0.9477f, 0.6358f, 0.5136f));
    AddControlPoint(0.7333f, Color(0.9466f, 0.6298f, 0.5075f));
    AddControlPoint(0.7372f, Color(0.9455f, 0.6238f, 0.5015f));
    AddControlPoint(0.7411f, Color(0.9442f, 0.6178f, 0.4954f));
    AddControlPoint(0.7450f, Color(0.9429f, 0.6117f, 0.4894f));
    AddControlPoint(0.7490f, Color(0.9415f, 0.6055f, 0.4834f));
    AddControlPoint(0.7529f, Color(0.9400f, 0.5993f, 0.4774f));
    AddControlPoint(0.7568f, Color(0.9384f, 0.5930f, 0.4714f));
    AddControlPoint(0.7607f, Color(0.9368f, 0.5866f, 0.4654f));
    AddControlPoint(0.7647f, Color(0.9350f, 0.5802f, 0.4595f));
    AddControlPoint(0.7686f, Color(0.9332f, 0.5738f, 0.4536f));
    AddControlPoint(0.7725f, Color(0.9313f, 0.5673f, 0.4477f));
    AddControlPoint(0.7764f, Color(0.9293f, 0.5607f, 0.4418f));
    AddControlPoint(0.7803f, Color(0.9273f, 0.5541f, 0.4359f));
    AddControlPoint(0.7843f, Color(0.9251f, 0.5475f, 0.4300f));
    AddControlPoint(0.7882f, Color(0.9229f, 0.5407f, 0.4242f));
    AddControlPoint(0.7921f, Color(0.9206f, 0.5340f, 0.4184f));
    AddControlPoint(0.7960f, Color(0.9182f, 0.5271f, 0.4126f));
    AddControlPoint(0.8f, Color(0.9158f, 0.5203f, 0.4069f));
    AddControlPoint(0.8039f, Color(0.9132f, 0.5133f, 0.4011f));
    AddControlPoint(0.8078f, Color(0.9106f, 0.5063f, 0.3954f));
    AddControlPoint(0.8117f, Color(0.9079f, 0.4993f, 0.3897f));
    AddControlPoint(0.8156f, Color(0.9052f, 0.4922f, 0.3841f));
    AddControlPoint(0.8196f, Color(0.9023f, 0.4851f, 0.3784f));
    AddControlPoint(0.8235f, Color(0.8994f, 0.4779f, 0.3728f));
    AddControlPoint(0.8274f, Color(0.8964f, 0.4706f, 0.3672f));
    AddControlPoint(0.8313f, Color(0.8933f, 0.4633f, 0.3617f));
    AddControlPoint(0.8352f, Color(0.8901f, 0.4559f, 0.3561f));
    AddControlPoint(0.8392f, Color(0.8869f, 0.4485f, 0.3506f));
    AddControlPoint(0.8431f, Color(0.8836f, 0.4410f, 0.3452f));
    AddControlPoint(0.8470f, Color(0.8802f, 0.4335f, 0.3397f));
    AddControlPoint(0.8509f, Color(0.8767f, 0.4259f, 0.3343f));
    AddControlPoint(0.8549f, Color(0.8732f, 0.4183f, 0.3289f));
    AddControlPoint(0.8588f, Color(0.8696f, 0.4106f, 0.3236f));
    AddControlPoint(0.8627f, Color(0.8659f, 0.4028f, 0.3183f));
    AddControlPoint(0.8666f, Color(0.8622f, 0.3950f, 0.3130f));
    AddControlPoint(0.8705f, Color(0.8583f, 0.3871f, 0.3077f));
    AddControlPoint(0.8745f, Color(0.8544f, 0.3792f, 0.3025f));
    AddControlPoint(0.8784f, Color(0.8505f, 0.3712f, 0.2973f));
    AddControlPoint(0.8823f, Color(0.8464f, 0.3631f, 0.2921f));
    AddControlPoint(0.8862f, Color(0.8423f, 0.3549f, 0.2870f));
    AddControlPoint(0.8901f, Color(0.8381f, 0.3467f, 0.2819f));
    AddControlPoint(0.8941f, Color(0.8339f, 0.3384f, 0.2768f));
    AddControlPoint(0.8980f, Color(0.8295f, 0.3300f, 0.2718f));
    AddControlPoint(0.9019f, Color(0.8251f, 0.3215f, 0.2668f));
    AddControlPoint(0.9058f, Color(0.8207f, 0.3129f, 0.2619f));
    AddControlPoint(0.9098f, Color(0.8162f, 0.3043f, 0.2570f));
    AddControlPoint(0.9137f, Color(0.8116f, 0.2955f, 0.2521f));
    AddControlPoint(0.9176f, Color(0.8069f, 0.2866f, 0.2472f));
    AddControlPoint(0.9215f, Color(0.8022f, 0.2776f, 0.2424f));
    AddControlPoint(0.9254f, Color(0.7974f, 0.2685f, 0.2377f));
    AddControlPoint(0.9294f, Color(0.7925f, 0.2592f, 0.2329f));
    AddControlPoint(0.9333f, Color(0.7876f, 0.2498f, 0.2282f));
    AddControlPoint(0.9372f, Color(0.7826f, 0.2402f, 0.2236f));
    AddControlPoint(0.9411f, Color(0.7775f, 0.2304f, 0.2190f));
    AddControlPoint(0.9450f, Color(0.7724f, 0.2204f, 0.2144f));
    AddControlPoint(0.9490f, Color(0.7672f, 0.2102f, 0.2098f));
    AddControlPoint(0.9529f, Color(0.7620f, 0.1997f, 0.2053f));
    AddControlPoint(0.9568f, Color(0.7567f, 0.1889f, 0.2009f));
    AddControlPoint(0.9607f, Color(0.7514f, 0.1777f, 0.1965f));
    AddControlPoint(0.9647f, Color(0.7459f, 0.1662f, 0.1921f));
    AddControlPoint(0.9686f, Color(0.7405f, 0.1541f, 0.1877f));
    AddControlPoint(0.9725f, Color(0.7349f, 0.1414f, 0.1834f));
    AddControlPoint(0.9764f, Color(0.7293f, 0.1279f, 0.1792f));
    AddControlPoint(0.9803f, Color(0.7237f, 0.1134f, 0.1750f));
    AddControlPoint(0.9843f, Color(0.7180f, 0.0975f, 0.1708f));
    AddControlPoint(0.9882f, Color(0.7122f, 0.0796f, 0.1667f));
    AddControlPoint(0.9921f, Color(0.7064f, 0.0585f, 0.1626f));
    AddControlPoint(0.9960f, Color(0.7005f, 0.0315f, 0.1585f));
    AddControlPoint(1.0f, Color(0.6946f, 0.0029f, 0.1545f));
  }
  else if (name == "temperature")
  {
    AddControlPoint(0.05f, Color(0.f, 0.f, 1.f));
    AddControlPoint(0.35f, Color(0.f, 1.f, 1.f));
    AddControlPoint(0.50f, Color(1.f, 1.f, 1.f));
    AddControlPoint(0.65f, Color(1.f, 1.f, 0.f));
    AddControlPoint(0.95f, Color(1.f, 0.f, 0.f));
  }
  else if (name == "rainbow")
  {
    // I really want to delete this. If users want to make a crap
    // color map, let them build it themselves.
    AddControlPoint(0.00f, Color(0.f, 0.f, 1.f));
    AddControlPoint(0.20f, Color(0.f, 1.f, 1.f));
    AddControlPoint(0.45f, Color(0.f, 1.f, 0.f));
    AddControlPoint(0.55f, Color(.7f, 1.f, 0.f));
    AddControlPoint(0.6f, Color(1.f, 1.f, 0.f));
    AddControlPoint(0.75f, Color(1.f, .5f, 0.f));
    AddControlPoint(0.9f, Color(1.f, 0.f, 0.f));
    AddControlPoint(0.98f, Color(1.f, 0.f, .5F));
    AddControlPoint(1.0f, Color(1.f, 0.f, 1.f));
  }
  else if (name == "levels")
  {
    AddControlPoint(0.0f, Color(0.f, 0.f, 1.f));
    AddControlPoint(0.2f, Color(0.f, 0.f, 1.f));
    AddControlPoint(0.2f, Color(0.f, 1.f, 1.f));
    AddControlPoint(0.4f, Color(0.f, 1.f, 1.f));
    AddControlPoint(0.4f, Color(0.f, 1.f, 0.f));
    AddControlPoint(0.6f, Color(0.f, 1.f, 0.f));
    AddControlPoint(0.6f, Color(1.f, 1.f, 0.f));
    AddControlPoint(0.8f, Color(1.f, 1.f, 0.f));
    AddControlPoint(0.8f, Color(1.f, 0.f, 0.f));
    AddControlPoint(1.0f, Color(1.f, 0.f, 0.f));
  }
  else if (name == "dense" || name == "sharp")
  {
    // I'm not fond of this color map either.
    this->Internals->Smooth = (name == "dense") ? true : false;
    AddControlPoint(0.0f, Color(0.26f, 0.22f, 0.92f));
    AddControlPoint(0.1f, Color(0.00f, 0.00f, 0.52f));
    AddControlPoint(0.2f, Color(0.00f, 1.00f, 1.00f));
    AddControlPoint(0.3f, Color(0.00f, 0.50f, 0.00f));
    AddControlPoint(0.4f, Color(1.00f, 1.00f, 0.00f));
    AddControlPoint(0.5f, Color(0.60f, 0.47f, 0.00f));
    AddControlPoint(0.6f, Color(1.00f, 0.47f, 0.00f));
    AddControlPoint(0.7f, Color(0.61f, 0.18f, 0.00f));
    AddControlPoint(0.8f, Color(1.00f, 0.03f, 0.17f));
    AddControlPoint(0.9f, Color(0.63f, 0.12f, 0.34f));
    AddControlPoint(1.0f, Color(1.00f, 0.40f, 1.00f));
  }
  else if (name == "thermal")
  {
    AddControlPoint(0.0f, Color(0.30f, 0.00f, 0.00f));
    AddControlPoint(0.25f, Color(1.00f, 0.00f, 0.00f));
    AddControlPoint(0.50f, Color(1.00f, 1.00f, 0.00f));
    AddControlPoint(0.55f, Color(0.80f, 0.55f, 0.20f));
    AddControlPoint(0.60f, Color(0.60f, 0.37f, 0.40f));
    AddControlPoint(0.65f, Color(0.40f, 0.22f, 0.60f));
    AddControlPoint(0.75f, Color(0.00f, 0.00f, 1.00f));
    AddControlPoint(1.00f, Color(1.00f, 1.00f, 1.00f));
  }
  // The following five tables are perceeptually linearized colortables
  // (4 rainbow, one heatmap) from BSD-licensed code by Matteo Niccoli.
  // See: http://mycarta.wordpress.com/2012/05/29/the-rainbow-is-dead-long-live-the-rainbow-series-outline/
  else if (name == "IsoL")
  {
    vtkm::Float32 n = 5;
    AddControlPoint(0.f / n, Color(0.9102f, 0.2236f, 0.8997f));
    AddControlPoint(1.f / n, Color(0.4027f, 0.3711f, 1.0000f));
    AddControlPoint(2.f / n, Color(0.0422f, 0.5904f, 0.5899f));
    AddControlPoint(3.f / n, Color(0.0386f, 0.6206f, 0.0201f));
    AddControlPoint(4.f / n, Color(0.5441f, 0.5428f, 0.0110f));
    AddControlPoint(5.f / n, Color(1.0000f, 0.2288f, 0.1631f));
  }
  else if (name == "CubicL")
  {
    vtkm::Float32 n = 15;
    AddControlPoint(0.f / n, Color(0.4706f, 0.0000f, 0.5216f));
    AddControlPoint(1.f / n, Color(0.5137f, 0.0527f, 0.7096f));
    AddControlPoint(2.f / n, Color(0.4942f, 0.2507f, 0.8781f));
    AddControlPoint(3.f / n, Color(0.4296f, 0.3858f, 0.9922f));
    AddControlPoint(4.f / n, Color(0.3691f, 0.5172f, 0.9495f));
    AddControlPoint(5.f / n, Color(0.2963f, 0.6191f, 0.8515f));
    AddControlPoint(6.f / n, Color(0.2199f, 0.7134f, 0.7225f));
    AddControlPoint(7.f / n, Color(0.2643f, 0.7836f, 0.5756f));
    AddControlPoint(8.f / n, Color(0.3094f, 0.8388f, 0.4248f));
    AddControlPoint(9.f / n, Color(0.3623f, 0.8917f, 0.2858f));
    AddControlPoint(10.f / n, Color(0.5200f, 0.9210f, 0.3137f));
    AddControlPoint(11.f / n, Color(0.6800f, 0.9255f, 0.3386f));
    AddControlPoint(12.f / n, Color(0.8000f, 0.9255f, 0.3529f));
    AddControlPoint(13.f / n, Color(0.8706f, 0.8549f, 0.3608f));
    AddControlPoint(14.f / n, Color(0.9514f, 0.7466f, 0.3686f));
    AddControlPoint(15.f / n, Color(0.9765f, 0.5887f, 0.3569f));
  }
  else if (name == "CubicYF")
  {
    vtkm::Float32 n = 15;
    AddControlPoint(0.f / n, Color(0.5151f, 0.0482f, 0.6697f));
    AddControlPoint(1.f / n, Color(0.5199f, 0.1762f, 0.8083f));
    AddControlPoint(2.f / n, Color(0.4884f, 0.2912f, 0.9234f));
    AddControlPoint(3.f / n, Color(0.4297f, 0.3855f, 0.9921f));
    AddControlPoint(4.f / n, Color(0.3893f, 0.4792f, 0.9775f));
    AddControlPoint(5.f / n, Color(0.3337f, 0.5650f, 0.9056f));
    AddControlPoint(6.f / n, Color(0.2795f, 0.6419f, 0.8287f));
    AddControlPoint(7.f / n, Color(0.2210f, 0.7123f, 0.7258f));
    AddControlPoint(8.f / n, Color(0.2468f, 0.7612f, 0.6248f));
    AddControlPoint(9.f / n, Color(0.2833f, 0.8125f, 0.5069f));
    AddControlPoint(10.f / n, Color(0.3198f, 0.8492f, 0.3956f));
    AddControlPoint(11.f / n, Color(0.3602f, 0.8896f, 0.2919f));
    AddControlPoint(12.f / n, Color(0.4568f, 0.9136f, 0.3018f));
    AddControlPoint(13.f / n, Color(0.6033f, 0.9255f, 0.3295f));
    AddControlPoint(14.f / n, Color(0.7066f, 0.9255f, 0.3414f));
    AddControlPoint(15.f / n, Color(0.8000f, 0.9255f, 0.3529f));
  }
  else if (name == "LinearL")
  {
    vtkm::Float32 n = 15;
    AddControlPoint(0.f / n, Color(0.0143f, 0.0143f, 0.0143f));
    AddControlPoint(1.f / n, Color(0.1413f, 0.0555f, 0.1256f));
    AddControlPoint(2.f / n, Color(0.1761f, 0.0911f, 0.2782f));
    AddControlPoint(3.f / n, Color(0.1710f, 0.1314f, 0.4540f));
    AddControlPoint(4.f / n, Color(0.1074f, 0.2234f, 0.4984f));
    AddControlPoint(5.f / n, Color(0.0686f, 0.3044f, 0.5068f));
    AddControlPoint(6.f / n, Color(0.0008f, 0.3927f, 0.4267f));
    AddControlPoint(7.f / n, Color(0.0000f, 0.4763f, 0.3464f));
    AddControlPoint(8.f / n, Color(0.0000f, 0.5565f, 0.2469f));
    AddControlPoint(9.f / n, Color(0.0000f, 0.6381f, 0.1638f));
    AddControlPoint(10.f / n, Color(0.2167f, 0.6966f, 0.0000f));
    AddControlPoint(11.f / n, Color(0.3898f, 0.7563f, 0.0000f));
    AddControlPoint(12.f / n, Color(0.6912f, 0.7795f, 0.0000f));
    AddControlPoint(13.f / n, Color(0.8548f, 0.8041f, 0.4555f));
    AddControlPoint(14.f / n, Color(0.9712f, 0.8429f, 0.7287f));
    AddControlPoint(15.f / n, Color(0.9692f, 0.9273f, 0.8961f));
  }
  else if (name == "LinLhot")
  {
    vtkm::Float32 n = 15;
    AddControlPoint(0.f / n, Color(0.0225f, 0.0121f, 0.0121f));
    AddControlPoint(1.f / n, Color(0.1927f, 0.0225f, 0.0311f));
    AddControlPoint(2.f / n, Color(0.3243f, 0.0106f, 0.0000f));
    AddControlPoint(3.f / n, Color(0.4463f, 0.0000f, 0.0091f));
    AddControlPoint(4.f / n, Color(0.5706f, 0.0000f, 0.0737f));
    AddControlPoint(5.f / n, Color(0.6969f, 0.0000f, 0.1337f));
    AddControlPoint(6.f / n, Color(0.8213f, 0.0000f, 0.1792f));
    AddControlPoint(7.f / n, Color(0.8636f, 0.0000f, 0.0565f));
    AddControlPoint(8.f / n, Color(0.8821f, 0.2555f, 0.0000f));
    AddControlPoint(9.f / n, Color(0.8720f, 0.4182f, 0.0000f));
    AddControlPoint(10.f / n, Color(0.8424f, 0.5552f, 0.0000f));
    AddControlPoint(11.f / n, Color(0.8031f, 0.6776f, 0.0000f));
    AddControlPoint(12.f / n, Color(0.7659f, 0.7870f, 0.0000f));
    AddControlPoint(13.f / n, Color(0.8170f, 0.8296f, 0.0000f));
    AddControlPoint(14.f / n, Color(0.8853f, 0.8896f, 0.4113f));
    AddControlPoint(15.f / n, Color(0.9481f, 0.9486f, 0.7165f));
  }
  // ColorBrewer tables here.  (See LICENSE.txt)
  else if (name == "PuRd")
  {
    AddControlPoint(0.0000f, Color(0.9686f, 0.9569f, 0.9765f));
    AddControlPoint(0.1250f, Color(0.9059f, 0.8824f, 0.9373f));
    AddControlPoint(0.2500f, Color(0.8314f, 0.7255f, 0.8549f));
    AddControlPoint(0.3750f, Color(0.7882f, 0.5804f, 0.7804f));
    AddControlPoint(0.5000f, Color(0.8745f, 0.3961f, 0.6902f));
    AddControlPoint(0.6250f, Color(0.9059f, 0.1608f, 0.5412f));
    AddControlPoint(0.7500f, Color(0.8078f, 0.0706f, 0.3373f));
    AddControlPoint(0.8750f, Color(0.5961f, 0.0000f, 0.2627f));
    AddControlPoint(1.0000f, Color(0.4039f, 0.0000f, 0.1216f));
  }
  else if (name == "Accent")
  {
    AddControlPoint(0.0000f, Color(0.4980f, 0.7882f, 0.4980f));
    AddControlPoint(0.1429f, Color(0.7451f, 0.6824f, 0.8314f));
    AddControlPoint(0.2857f, Color(0.9922f, 0.7529f, 0.5255f));
    AddControlPoint(0.4286f, Color(1.0000f, 1.0000f, 0.6000f));
    AddControlPoint(0.5714f, Color(0.2196f, 0.4235f, 0.6902f));
    AddControlPoint(0.7143f, Color(0.9412f, 0.0078f, 0.4980f));
    AddControlPoint(0.8571f, Color(0.7490f, 0.3569f, 0.0902f));
    AddControlPoint(1.0000f, Color(0.4000f, 0.4000f, 0.4000f));
  }
  else if (name == "Blues")
  {
    AddControlPoint(0.0000f, Color(0.9686f, 0.9843f, 1.0000f));
    AddControlPoint(0.1250f, Color(0.8706f, 0.9216f, 0.9686f));
    AddControlPoint(0.2500f, Color(0.7765f, 0.8588f, 0.9373f));
    AddControlPoint(0.3750f, Color(0.6196f, 0.7922f, 0.8824f));
    AddControlPoint(0.5000f, Color(0.4196f, 0.6824f, 0.8392f));
    AddControlPoint(0.6250f, Color(0.2588f, 0.5725f, 0.7765f));
    AddControlPoint(0.7500f, Color(0.1294f, 0.4431f, 0.7098f));
    AddControlPoint(0.8750f, Color(0.0314f, 0.3176f, 0.6118f));
    AddControlPoint(1.0000f, Color(0.0314f, 0.1882f, 0.4196f));
  }
  else if (name == "BrBG")
  {
    AddControlPoint(0.0000f, Color(0.3294f, 0.1882f, 0.0196f));
    AddControlPoint(0.1000f, Color(0.5490f, 0.3176f, 0.0392f));
    AddControlPoint(0.2000f, Color(0.7490f, 0.5059f, 0.1765f));
    AddControlPoint(0.3000f, Color(0.8745f, 0.7608f, 0.4902f));
    AddControlPoint(0.4000f, Color(0.9647f, 0.9098f, 0.7647f));
    AddControlPoint(0.5000f, Color(0.9608f, 0.9608f, 0.9608f));
    AddControlPoint(0.6000f, Color(0.7804f, 0.9176f, 0.8980f));
    AddControlPoint(0.7000f, Color(0.5020f, 0.8039f, 0.7569f));
    AddControlPoint(0.8000f, Color(0.2078f, 0.5922f, 0.5608f));
    AddControlPoint(0.9000f, Color(0.0039f, 0.4000f, 0.3686f));
    AddControlPoint(1.0000f, Color(0.0000f, 0.2353f, 0.1882f));
  }
  else if (name == "BuGn")
  {
    AddControlPoint(0.0000f, Color(0.9686f, 0.9882f, 0.9922f));
    AddControlPoint(0.1250f, Color(0.8980f, 0.9608f, 0.9765f));
    AddControlPoint(0.2500f, Color(0.8000f, 0.9255f, 0.9020f));
    AddControlPoint(0.3750f, Color(0.6000f, 0.8471f, 0.7882f));
    AddControlPoint(0.5000f, Color(0.4000f, 0.7608f, 0.6431f));
    AddControlPoint(0.6250f, Color(0.2549f, 0.6824f, 0.4627f));
    AddControlPoint(0.7500f, Color(0.1373f, 0.5451f, 0.2706f));
    AddControlPoint(0.8750f, Color(0.0000f, 0.4275f, 0.1725f));
    AddControlPoint(1.0000f, Color(0.0000f, 0.2667f, 0.1059f));
  }
  else if (name == "BuPu")
  {
    AddControlPoint(0.0000f, Color(0.9686f, 0.9882f, 0.9922f));
    AddControlPoint(0.1250f, Color(0.8784f, 0.9255f, 0.9569f));
    AddControlPoint(0.2500f, Color(0.7490f, 0.8275f, 0.9020f));
    AddControlPoint(0.3750f, Color(0.6196f, 0.7373f, 0.8549f));
    AddControlPoint(0.5000f, Color(0.5490f, 0.5882f, 0.7765f));
    AddControlPoint(0.6250f, Color(0.5490f, 0.4196f, 0.6941f));
    AddControlPoint(0.7500f, Color(0.5333f, 0.2549f, 0.6157f));
    AddControlPoint(0.8750f, Color(0.5059f, 0.0588f, 0.4863f));
    AddControlPoint(1.0000f, Color(0.3020f, 0.0000f, 0.2941f));
  }
  else if (name == "Dark2")
  {
    AddControlPoint(0.0000f, Color(0.1059f, 0.6196f, 0.4667f));
    AddControlPoint(0.1429f, Color(0.8510f, 0.3725f, 0.0078f));
    AddControlPoint(0.2857f, Color(0.4588f, 0.4392f, 0.7020f));
    AddControlPoint(0.4286f, Color(0.9059f, 0.1608f, 0.5412f));
    AddControlPoint(0.5714f, Color(0.4000f, 0.6510f, 0.1176f));
    AddControlPoint(0.7143f, Color(0.9020f, 0.6706f, 0.0078f));
    AddControlPoint(0.8571f, Color(0.6510f, 0.4627f, 0.1137f));
    AddControlPoint(1.0000f, Color(0.4000f, 0.4000f, 0.4000f));
  }
  else if (name == "GnBu")
  {
    AddControlPoint(0.0000f, Color(0.9686f, 0.9882f, 0.9412f));
    AddControlPoint(0.1250f, Color(0.8784f, 0.9529f, 0.8588f));
    AddControlPoint(0.2500f, Color(0.8000f, 0.9216f, 0.7725f));
    AddControlPoint(0.3750f, Color(0.6588f, 0.8667f, 0.7098f));
    AddControlPoint(0.5000f, Color(0.4824f, 0.8000f, 0.7686f));
    AddControlPoint(0.6250f, Color(0.3059f, 0.7020f, 0.8275f));
    AddControlPoint(0.7500f, Color(0.1686f, 0.5490f, 0.7451f));
    AddControlPoint(0.8750f, Color(0.0314f, 0.4078f, 0.6745f));
    AddControlPoint(1.0000f, Color(0.0314f, 0.2510f, 0.5059f));
  }
  else if (name == "Greens")
  {
    AddControlPoint(0.0000f, Color(0.9686f, 0.9882f, 0.9608f));
    AddControlPoint(0.1250f, Color(0.8980f, 0.9608f, 0.8784f));
    AddControlPoint(0.2500f, Color(0.7804f, 0.9137f, 0.7529f));
    AddControlPoint(0.3750f, Color(0.6314f, 0.8510f, 0.6078f));
    AddControlPoint(0.5000f, Color(0.4549f, 0.7686f, 0.4627f));
    AddControlPoint(0.6250f, Color(0.2549f, 0.6706f, 0.3647f));
    AddControlPoint(0.7500f, Color(0.1373f, 0.5451f, 0.2706f));
    AddControlPoint(0.8750f, Color(0.0000f, 0.4275f, 0.1725f));
    AddControlPoint(1.0000f, Color(0.0000f, 0.2667f, 0.1059f));
  }
  else if (name == "Greys")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 1.0000f, 1.0000f));
    AddControlPoint(0.1250f, Color(0.9412f, 0.9412f, 0.9412f));
    AddControlPoint(0.2500f, Color(0.8510f, 0.8510f, 0.8510f));
    AddControlPoint(0.3750f, Color(0.7412f, 0.7412f, 0.7412f));
    AddControlPoint(0.5000f, Color(0.5882f, 0.5882f, 0.5882f));
    AddControlPoint(0.6250f, Color(0.4510f, 0.4510f, 0.4510f));
    AddControlPoint(0.7500f, Color(0.3216f, 0.3216f, 0.3216f));
    AddControlPoint(0.8750f, Color(0.1451f, 0.1451f, 0.1451f));
    AddControlPoint(1.0000f, Color(0.0000f, 0.0000f, 0.0000f));
  }
  else if (name == "Oranges")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 0.9608f, 0.9216f));
    AddControlPoint(0.1250f, Color(0.9961f, 0.9020f, 0.8078f));
    AddControlPoint(0.2500f, Color(0.9922f, 0.8157f, 0.6353f));
    AddControlPoint(0.3750f, Color(0.9922f, 0.6824f, 0.4196f));
    AddControlPoint(0.5000f, Color(0.9922f, 0.5529f, 0.2353f));
    AddControlPoint(0.6250f, Color(0.9451f, 0.4118f, 0.0745f));
    AddControlPoint(0.7500f, Color(0.8510f, 0.2824f, 0.0039f));
    AddControlPoint(0.8750f, Color(0.6510f, 0.2118f, 0.0118f));
    AddControlPoint(1.0000f, Color(0.4980f, 0.1529f, 0.0157f));
  }
  else if (name == "OrRd")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 0.9686f, 0.9255f));
    AddControlPoint(0.1250f, Color(0.9961f, 0.9098f, 0.7843f));
    AddControlPoint(0.2500f, Color(0.9922f, 0.8314f, 0.6196f));
    AddControlPoint(0.3750f, Color(0.9922f, 0.7333f, 0.5176f));
    AddControlPoint(0.5000f, Color(0.9882f, 0.5529f, 0.3490f));
    AddControlPoint(0.6250f, Color(0.9373f, 0.3961f, 0.2824f));
    AddControlPoint(0.7500f, Color(0.8431f, 0.1882f, 0.1216f));
    AddControlPoint(0.8750f, Color(0.7020f, 0.0000f, 0.0000f));
    AddControlPoint(1.0000f, Color(0.4980f, 0.0000f, 0.0000f));
  }
  else if (name == "Paired")
  {
    AddControlPoint(0.0000f, Color(0.6510f, 0.8078f, 0.8902f));
    AddControlPoint(0.0909f, Color(0.1216f, 0.4706f, 0.7059f));
    AddControlPoint(0.1818f, Color(0.6980f, 0.8745f, 0.5412f));
    AddControlPoint(0.2727f, Color(0.2000f, 0.6275f, 0.1725f));
    AddControlPoint(0.3636f, Color(0.9843f, 0.6039f, 0.6000f));
    AddControlPoint(0.4545f, Color(0.8902f, 0.1020f, 0.1098f));
    AddControlPoint(0.5455f, Color(0.9922f, 0.7490f, 0.4353f));
    AddControlPoint(0.6364f, Color(1.0000f, 0.4980f, 0.0000f));
    AddControlPoint(0.7273f, Color(0.7922f, 0.6980f, 0.8392f));
    AddControlPoint(0.8182f, Color(0.4157f, 0.2392f, 0.6039f));
    AddControlPoint(0.9091f, Color(1.0000f, 1.0000f, 0.6000f));
    AddControlPoint(1.0000f, Color(0.6941f, 0.3490f, 0.1569f));
  }
  else if (name == "Pastel1")
  {
    AddControlPoint(0.0000f, Color(0.9843f, 0.7059f, 0.6824f));
    AddControlPoint(0.1250f, Color(0.7020f, 0.8039f, 0.8902f));
    AddControlPoint(0.2500f, Color(0.8000f, 0.9216f, 0.7725f));
    AddControlPoint(0.3750f, Color(0.8706f, 0.7961f, 0.8941f));
    AddControlPoint(0.5000f, Color(0.9961f, 0.8510f, 0.6510f));
    AddControlPoint(0.6250f, Color(1.0000f, 1.0000f, 0.8000f));
    AddControlPoint(0.7500f, Color(0.8980f, 0.8471f, 0.7412f));
    AddControlPoint(0.8750f, Color(0.9922f, 0.8549f, 0.9255f));
    AddControlPoint(1.0000f, Color(0.9490f, 0.9490f, 0.9490f));
  }
  else if (name == "Pastel2")
  {
    AddControlPoint(0.0000f, Color(0.7020f, 0.8863f, 0.8039f));
    AddControlPoint(0.1429f, Color(0.9922f, 0.8039f, 0.6745f));
    AddControlPoint(0.2857f, Color(0.7961f, 0.8353f, 0.9098f));
    AddControlPoint(0.4286f, Color(0.9569f, 0.7922f, 0.8941f));
    AddControlPoint(0.5714f, Color(0.9020f, 0.9608f, 0.7882f));
    AddControlPoint(0.7143f, Color(1.0000f, 0.9490f, 0.6824f));
    AddControlPoint(0.8571f, Color(0.9451f, 0.8863f, 0.8000f));
    AddControlPoint(1.0000f, Color(0.8000f, 0.8000f, 0.8000f));
  }
  else if (name == "PiYG")
  {
    AddControlPoint(0.0000f, Color(0.5569f, 0.0039f, 0.3216f));
    AddControlPoint(0.1000f, Color(0.7725f, 0.1059f, 0.4902f));
    AddControlPoint(0.2000f, Color(0.8706f, 0.4667f, 0.6824f));
    AddControlPoint(0.3000f, Color(0.9451f, 0.7137f, 0.8549f));
    AddControlPoint(0.4000f, Color(0.9922f, 0.8784f, 0.9373f));
    AddControlPoint(0.5000f, Color(0.9686f, 0.9686f, 0.9686f));
    AddControlPoint(0.6000f, Color(0.9020f, 0.9608f, 0.8157f));
    AddControlPoint(0.7000f, Color(0.7216f, 0.8824f, 0.5255f));
    AddControlPoint(0.8000f, Color(0.4980f, 0.7373f, 0.2549f));
    AddControlPoint(0.9000f, Color(0.3020f, 0.5725f, 0.1294f));
    AddControlPoint(1.0000f, Color(0.1529f, 0.3922f, 0.0980f));
  }
  else if (name == "PRGn")
  {
    AddControlPoint(0.0000f, Color(0.2510f, 0.0000f, 0.2941f));
    AddControlPoint(0.1000f, Color(0.4627f, 0.1647f, 0.5137f));
    AddControlPoint(0.2000f, Color(0.6000f, 0.4392f, 0.6706f));
    AddControlPoint(0.3000f, Color(0.7608f, 0.6471f, 0.8118f));
    AddControlPoint(0.4000f, Color(0.9059f, 0.8314f, 0.9098f));
    AddControlPoint(0.5000f, Color(0.9686f, 0.9686f, 0.9686f));
    AddControlPoint(0.6000f, Color(0.8510f, 0.9412f, 0.8275f));
    AddControlPoint(0.7000f, Color(0.6510f, 0.8588f, 0.6275f));
    AddControlPoint(0.8000f, Color(0.3529f, 0.6824f, 0.3804f));
    AddControlPoint(0.9000f, Color(0.1059f, 0.4706f, 0.2157f));
    AddControlPoint(1.0000f, Color(0.0000f, 0.2667f, 0.1059f));
  }
  else if (name == "PuBu")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 0.9686f, 0.9843f));
    AddControlPoint(0.1250f, Color(0.9255f, 0.9059f, 0.9490f));
    AddControlPoint(0.2500f, Color(0.8157f, 0.8196f, 0.9020f));
    AddControlPoint(0.3750f, Color(0.6510f, 0.7412f, 0.8588f));
    AddControlPoint(0.5000f, Color(0.4549f, 0.6627f, 0.8118f));
    AddControlPoint(0.6250f, Color(0.2118f, 0.5647f, 0.7529f));
    AddControlPoint(0.7500f, Color(0.0196f, 0.4392f, 0.6902f));
    AddControlPoint(0.8750f, Color(0.0157f, 0.3529f, 0.5529f));
    AddControlPoint(1.0000f, Color(0.0078f, 0.2196f, 0.3451f));
  }
  else if (name == "PuBuGn")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 0.9686f, 0.9843f));
    AddControlPoint(0.1250f, Color(0.9255f, 0.8863f, 0.9412f));
    AddControlPoint(0.2500f, Color(0.8157f, 0.8196f, 0.9020f));
    AddControlPoint(0.3750f, Color(0.6510f, 0.7412f, 0.8588f));
    AddControlPoint(0.5000f, Color(0.4039f, 0.6627f, 0.8118f));
    AddControlPoint(0.6250f, Color(0.2118f, 0.5647f, 0.7529f));
    AddControlPoint(0.7500f, Color(0.0078f, 0.5059f, 0.5412f));
    AddControlPoint(0.8750f, Color(0.0039f, 0.4235f, 0.3490f));
    AddControlPoint(1.0000f, Color(0.0039f, 0.2745f, 0.2118f));
  }
  else if (name == "PuOr")
  {
    AddControlPoint(0.0000f, Color(0.4980f, 0.2314f, 0.0314f));
    AddControlPoint(0.1000f, Color(0.7020f, 0.3451f, 0.0235f));
    AddControlPoint(0.2000f, Color(0.8784f, 0.5098f, 0.0784f));
    AddControlPoint(0.3000f, Color(0.9922f, 0.7216f, 0.3882f));
    AddControlPoint(0.4000f, Color(0.9961f, 0.8784f, 0.7137f));
    AddControlPoint(0.5000f, Color(0.9686f, 0.9686f, 0.9686f));
    AddControlPoint(0.6000f, Color(0.8471f, 0.8549f, 0.9216f));
    AddControlPoint(0.7000f, Color(0.6980f, 0.6706f, 0.8235f));
    AddControlPoint(0.8000f, Color(0.5020f, 0.4510f, 0.6745f));
    AddControlPoint(0.9000f, Color(0.3294f, 0.1529f, 0.5333f));
    AddControlPoint(1.0000f, Color(0.1765f, 0.0000f, 0.2941f));
  }
  else if (name == "PuRd")
  {
    AddControlPoint(0.0000f, Color(0.9686f, 0.9569f, 0.9765f));
    AddControlPoint(0.1250f, Color(0.9059f, 0.8824f, 0.9373f));
    AddControlPoint(0.2500f, Color(0.8314f, 0.7255f, 0.8549f));
    AddControlPoint(0.3750f, Color(0.7882f, 0.5804f, 0.7804f));
    AddControlPoint(0.5000f, Color(0.8745f, 0.3961f, 0.6902f));
    AddControlPoint(0.6250f, Color(0.9059f, 0.1608f, 0.5412f));
    AddControlPoint(0.7500f, Color(0.8078f, 0.0706f, 0.3373f));
    AddControlPoint(0.8750f, Color(0.5961f, 0.0000f, 0.2627f));
    AddControlPoint(1.0000f, Color(0.4039f, 0.0000f, 0.1216f));
  }
  else if (name == "Purples")
  {
    AddControlPoint(0.0000f, Color(0.9882f, 0.9843f, 0.9922f));
    AddControlPoint(0.1250f, Color(0.9373f, 0.9294f, 0.9608f));
    AddControlPoint(0.2500f, Color(0.8549f, 0.8549f, 0.9216f));
    AddControlPoint(0.3750f, Color(0.7373f, 0.7412f, 0.8627f));
    AddControlPoint(0.5000f, Color(0.6196f, 0.6039f, 0.7843f));
    AddControlPoint(0.6250f, Color(0.5020f, 0.4902f, 0.7294f));
    AddControlPoint(0.7500f, Color(0.4157f, 0.3176f, 0.6392f));
    AddControlPoint(0.8750f, Color(0.3294f, 0.1529f, 0.5608f));
    AddControlPoint(1.0000f, Color(0.2471f, 0.0000f, 0.4902f));
  }
  else if (name == "RdBu")
  {
    AddControlPoint(0.0000f, Color(0.4039f, 0.0000f, 0.1216f));
    AddControlPoint(0.1000f, Color(0.6980f, 0.0941f, 0.1686f));
    AddControlPoint(0.2000f, Color(0.8392f, 0.3765f, 0.3020f));
    AddControlPoint(0.3000f, Color(0.9569f, 0.6471f, 0.5098f));
    AddControlPoint(0.4000f, Color(0.9922f, 0.8588f, 0.7804f));
    AddControlPoint(0.5000f, Color(0.9686f, 0.9686f, 0.9686f));
    AddControlPoint(0.6000f, Color(0.8196f, 0.8980f, 0.9412f));
    AddControlPoint(0.7000f, Color(0.5725f, 0.7725f, 0.8706f));
    AddControlPoint(0.8000f, Color(0.2627f, 0.5765f, 0.7647f));
    AddControlPoint(0.9000f, Color(0.1294f, 0.4000f, 0.6745f));
    AddControlPoint(1.0000f, Color(0.0196f, 0.1882f, 0.3804f));
  }
  else if (name == "RdGy")
  {
    AddControlPoint(0.0000f, Color(0.4039f, 0.0000f, 0.1216f));
    AddControlPoint(0.1000f, Color(0.6980f, 0.0941f, 0.1686f));
    AddControlPoint(0.2000f, Color(0.8392f, 0.3765f, 0.3020f));
    AddControlPoint(0.3000f, Color(0.9569f, 0.6471f, 0.5098f));
    AddControlPoint(0.4000f, Color(0.9922f, 0.8588f, 0.7804f));
    AddControlPoint(0.5000f, Color(1.0000f, 1.0000f, 1.0000f));
    AddControlPoint(0.6000f, Color(0.8784f, 0.8784f, 0.8784f));
    AddControlPoint(0.7000f, Color(0.7294f, 0.7294f, 0.7294f));
    AddControlPoint(0.8000f, Color(0.5294f, 0.5294f, 0.5294f));
    AddControlPoint(0.9000f, Color(0.3020f, 0.3020f, 0.3020f));
    AddControlPoint(1.0000f, Color(0.1020f, 0.1020f, 0.1020f));
  }
  else if (name == "RdPu")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 0.9686f, 0.9529f));
    AddControlPoint(0.1250f, Color(0.9922f, 0.8784f, 0.8667f));
    AddControlPoint(0.2500f, Color(0.9882f, 0.7725f, 0.7529f));
    AddControlPoint(0.3750f, Color(0.9804f, 0.6235f, 0.7098f));
    AddControlPoint(0.5000f, Color(0.9686f, 0.4078f, 0.6314f));
    AddControlPoint(0.6250f, Color(0.8667f, 0.2039f, 0.5922f));
    AddControlPoint(0.7500f, Color(0.6824f, 0.0039f, 0.4941f));
    AddControlPoint(0.8750f, Color(0.4784f, 0.0039f, 0.4667f));
    AddControlPoint(1.0000f, Color(0.2863f, 0.0000f, 0.4157f));
  }
  else if (name == "RdYlBu")
  {
    AddControlPoint(0.0000f, Color(0.6471f, 0.0000f, 0.1490f));
    AddControlPoint(0.1000f, Color(0.8431f, 0.1882f, 0.1529f));
    AddControlPoint(0.2000f, Color(0.9569f, 0.4275f, 0.2627f));
    AddControlPoint(0.3000f, Color(0.9922f, 0.6824f, 0.3804f));
    AddControlPoint(0.4000f, Color(0.9961f, 0.8784f, 0.5647f));
    AddControlPoint(0.5000f, Color(1.0000f, 1.0000f, 0.7490f));
    AddControlPoint(0.6000f, Color(0.8784f, 0.9529f, 0.9725f));
    AddControlPoint(0.7000f, Color(0.6706f, 0.8510f, 0.9137f));
    AddControlPoint(0.8000f, Color(0.4549f, 0.6784f, 0.8196f));
    AddControlPoint(0.9000f, Color(0.2706f, 0.4588f, 0.7059f));
    AddControlPoint(1.0000f, Color(0.1922f, 0.2118f, 0.5843f));
  }
  else if (name == "RdYlGn")
  {
    AddControlPoint(0.0000f, Color(0.6471f, 0.0000f, 0.1490f));
    AddControlPoint(0.1000f, Color(0.8431f, 0.1882f, 0.1529f));
    AddControlPoint(0.2000f, Color(0.9569f, 0.4275f, 0.2627f));
    AddControlPoint(0.3000f, Color(0.9922f, 0.6824f, 0.3804f));
    AddControlPoint(0.4000f, Color(0.9961f, 0.8784f, 0.5451f));
    AddControlPoint(0.5000f, Color(1.0000f, 1.0000f, 0.7490f));
    AddControlPoint(0.6000f, Color(0.8510f, 0.9373f, 0.5451f));
    AddControlPoint(0.7000f, Color(0.6510f, 0.8510f, 0.4157f));
    AddControlPoint(0.8000f, Color(0.4000f, 0.7412f, 0.3882f));
    AddControlPoint(0.9000f, Color(0.1020f, 0.5961f, 0.3137f));
    AddControlPoint(1.0000f, Color(0.0000f, 0.4078f, 0.2157f));
  }
  else if (name == "Reds")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 0.9608f, 0.9412f));
    AddControlPoint(0.1250f, Color(0.9961f, 0.8784f, 0.8235f));
    AddControlPoint(0.2500f, Color(0.9882f, 0.7333f, 0.6314f));
    AddControlPoint(0.3750f, Color(0.9882f, 0.5725f, 0.4471f));
    AddControlPoint(0.5000f, Color(0.9843f, 0.4157f, 0.2902f));
    AddControlPoint(0.6250f, Color(0.9373f, 0.2314f, 0.1725f));
    AddControlPoint(0.7500f, Color(0.7961f, 0.0941f, 0.1137f));
    AddControlPoint(0.8750f, Color(0.6471f, 0.0588f, 0.0824f));
    AddControlPoint(1.0000f, Color(0.4039f, 0.0000f, 0.0510f));
  }
  else if (name == "Set1")
  {
    AddControlPoint(0.0000f, Color(0.8941f, 0.1020f, 0.1098f));
    AddControlPoint(0.1250f, Color(0.2157f, 0.4941f, 0.7216f));
    AddControlPoint(0.2500f, Color(0.3020f, 0.6863f, 0.2902f));
    AddControlPoint(0.3750f, Color(0.5961f, 0.3059f, 0.6392f));
    AddControlPoint(0.5000f, Color(1.0000f, 0.4980f, 0.0000f));
    AddControlPoint(0.6250f, Color(1.0000f, 1.0000f, 0.2000f));
    AddControlPoint(0.7500f, Color(0.6510f, 0.3373f, 0.1569f));
    AddControlPoint(0.8750f, Color(0.9686f, 0.5059f, 0.7490f));
    AddControlPoint(1.0000f, Color(0.6000f, 0.6000f, 0.6000f));
  }
  else if (name == "Set2")
  {
    AddControlPoint(0.0000f, Color(0.4000f, 0.7608f, 0.6471f));
    AddControlPoint(0.1429f, Color(0.9882f, 0.5529f, 0.3843f));
    AddControlPoint(0.2857f, Color(0.5529f, 0.6275f, 0.7961f));
    AddControlPoint(0.4286f, Color(0.9059f, 0.5412f, 0.7647f));
    AddControlPoint(0.5714f, Color(0.6510f, 0.8471f, 0.3294f));
    AddControlPoint(0.7143f, Color(1.0000f, 0.8510f, 0.1843f));
    AddControlPoint(0.8571f, Color(0.8980f, 0.7686f, 0.5804f));
    AddControlPoint(1.0000f, Color(0.7020f, 0.7020f, 0.7020f));
  }
  else if (name == "Set3")
  {
    AddControlPoint(0.0000f, Color(0.5529f, 0.8275f, 0.7804f));
    AddControlPoint(0.0909f, Color(1.0000f, 1.0000f, 0.7020f));
    AddControlPoint(0.1818f, Color(0.7451f, 0.7294f, 0.8549f));
    AddControlPoint(0.2727f, Color(0.9843f, 0.5020f, 0.4471f));
    AddControlPoint(0.3636f, Color(0.5020f, 0.6941f, 0.8275f));
    AddControlPoint(0.4545f, Color(0.9922f, 0.7059f, 0.3843f));
    AddControlPoint(0.5455f, Color(0.7020f, 0.8706f, 0.4118f));
    AddControlPoint(0.6364f, Color(0.9882f, 0.8039f, 0.8980f));
    AddControlPoint(0.7273f, Color(0.8510f, 0.8510f, 0.8510f));
    AddControlPoint(0.8182f, Color(0.7373f, 0.5020f, 0.7412f));
    AddControlPoint(0.9091f, Color(0.8000f, 0.9216f, 0.7725f));
    AddControlPoint(1.0000f, Color(1.0000f, 0.9294f, 0.4353f));
  }
  else if (name == "Spectral")
  {
    AddControlPoint(0.0000f, Color(0.6196f, 0.0039f, 0.2588f));
    AddControlPoint(0.1000f, Color(0.8353f, 0.2431f, 0.3098f));
    AddControlPoint(0.2000f, Color(0.9569f, 0.4275f, 0.2627f));
    AddControlPoint(0.3000f, Color(0.9922f, 0.6824f, 0.3804f));
    AddControlPoint(0.4000f, Color(0.9961f, 0.8784f, 0.5451f));
    AddControlPoint(0.5000f, Color(1.0000f, 1.0000f, 0.7490f));
    AddControlPoint(0.6000f, Color(0.9020f, 0.9608f, 0.5961f));
    AddControlPoint(0.7000f, Color(0.6706f, 0.8667f, 0.6431f));
    AddControlPoint(0.8000f, Color(0.4000f, 0.7608f, 0.6471f));
    AddControlPoint(0.9000f, Color(0.1961f, 0.5333f, 0.7412f));
    AddControlPoint(1.0000f, Color(0.3686f, 0.3098f, 0.6353f));
  }
  else if (name == "YlGnBu")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 1.0000f, 0.8510f));
    AddControlPoint(0.1250f, Color(0.9294f, 0.9725f, 0.6941f));
    AddControlPoint(0.2500f, Color(0.7804f, 0.9137f, 0.7059f));
    AddControlPoint(0.3750f, Color(0.4980f, 0.8039f, 0.7333f));
    AddControlPoint(0.5000f, Color(0.2549f, 0.7137f, 0.7686f));
    AddControlPoint(0.6250f, Color(0.1137f, 0.5686f, 0.7529f));
    AddControlPoint(0.7500f, Color(0.1333f, 0.3686f, 0.6588f));
    AddControlPoint(0.8750f, Color(0.1451f, 0.2039f, 0.5804f));
    AddControlPoint(1.0000f, Color(0.0314f, 0.1137f, 0.3451f));
  }
  else if (name == "YlGn")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 1.0000f, 0.8980f));
    AddControlPoint(0.1250f, Color(0.9686f, 0.9882f, 0.7255f));
    AddControlPoint(0.2500f, Color(0.8510f, 0.9412f, 0.6392f));
    AddControlPoint(0.3750f, Color(0.6784f, 0.8667f, 0.5569f));
    AddControlPoint(0.5000f, Color(0.4706f, 0.7765f, 0.4745f));
    AddControlPoint(0.6250f, Color(0.2549f, 0.6706f, 0.3647f));
    AddControlPoint(0.7500f, Color(0.1373f, 0.5176f, 0.2627f));
    AddControlPoint(0.8750f, Color(0.0000f, 0.4078f, 0.2157f));
    AddControlPoint(1.0000f, Color(0.0000f, 0.2706f, 0.1608f));
  }
  else if (name == "YlOrBr")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 1.0000f, 0.8980f));
    AddControlPoint(0.1250f, Color(1.0000f, 0.9686f, 0.7373f));
    AddControlPoint(0.2500f, Color(0.9961f, 0.8902f, 0.5686f));
    AddControlPoint(0.3750f, Color(0.9961f, 0.7686f, 0.3098f));
    AddControlPoint(0.5000f, Color(0.9961f, 0.6000f, 0.1608f));
    AddControlPoint(0.6250f, Color(0.9255f, 0.4392f, 0.0784f));
    AddControlPoint(0.7500f, Color(0.8000f, 0.2980f, 0.0078f));
    AddControlPoint(0.8750f, Color(0.6000f, 0.2039f, 0.0157f));
    AddControlPoint(1.0000f, Color(0.4000f, 0.1451f, 0.0235f));
  }
  else if (name == "YlOrRd")
  {
    AddControlPoint(0.0000f, Color(1.0000f, 1.0000f, 0.8000f));
    AddControlPoint(0.1250f, Color(1.0000f, 0.9294f, 0.6275f));
    AddControlPoint(0.2500f, Color(0.9961f, 0.8510f, 0.4627f));
    AddControlPoint(0.3750f, Color(0.9961f, 0.6980f, 0.2980f));
    AddControlPoint(0.5000f, Color(0.9922f, 0.5529f, 0.2353f));
    AddControlPoint(0.6250f, Color(0.9882f, 0.3059f, 0.1647f));
    AddControlPoint(0.7500f, Color(0.8902f, 0.1020f, 0.1098f));
    AddControlPoint(0.8750f, Color(0.7412f, 0.0000f, 0.1490f));
    AddControlPoint(1.0000f, Color(0.5020f, 0.0000f, 0.1490f));
  }
  else
  {
    std::cout << "Unknown Color Table" << std::endl;
    AddControlPoint(0.0000f, Color(1.0000f, 1.0000f, 0.8000f));
    AddControlPoint(0.1250f, Color(1.0000f, 0.9294f, 0.6275f));
    AddControlPoint(0.2500f, Color(0.9961f, 0.8510f, 0.4627f));
    AddControlPoint(0.3750f, Color(0.9961f, 0.6980f, 0.2980f));
    AddControlPoint(0.5000f, Color(0.9922f, 0.5529f, 0.2353f));
    AddControlPoint(0.6250f, Color(0.9882f, 0.3059f, 0.1647f));
    AddControlPoint(0.7500f, Color(0.8902f, 0.1020f, 0.1098f));
    AddControlPoint(0.8750f, Color(0.7412f, 0.0000f, 0.1490f));
    AddControlPoint(1.0000f, Color(0.5020f, 0.0000f, 0.1490f));
  }
  this->Internals->UniqueName = std::string("00") + name;
  if (this->Internals->Smooth)
  {
    this->Internals->UniqueName[0] = '1';
  }
}

ColorTable::ColorTable(const vtkm::rendering::Color& color)
  : Internals(new detail::ColorTableInternals)
{
  this->Internals->UniqueName = "";
  this->Internals->Smooth = false;

  AddControlPoint(0, color);
  AddControlPoint(1, color);
}
}
} // namespace vtkm::rendering
