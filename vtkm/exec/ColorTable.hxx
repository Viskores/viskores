//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_ColorTable_hxx
#define vtk_m_exec_ColorTable_hxx

#include <vtkm/Math.h>
#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace exec
{
namespace detail
{

VTKM_EXEC
inline void RGBToHSV(const vtkm::Vec<float, 3>& rgb, vtkm::Vec<float, 3>& hsv)
{
  constexpr float onethird = 1.0f / 3.0f;
  constexpr float onesixth = 1.0f / 6.0f;
  constexpr float twothird = 2.0f / 3.0f;

  const float cmax = vtkm::Max(rgb[0], vtkm::Max(rgb[1], rgb[2]));
  const float cmin = vtkm::Min(rgb[0], vtkm::Min(rgb[1], rgb[2]));

  hsv[0] = 0.0f;
  hsv[1] = 0.0f;
  hsv[2] = cmax;

  if (cmax > 0.0f && cmax != cmin)
  {
    hsv[1] = (cmax - cmin) / cmax;
    if (rgb[0] == cmax)
    {
      hsv[0] = onesixth * (rgb[1] - rgb[2]) / (cmax - cmin);
    }
    else if (rgb[1] == cmax)
    {
      hsv[0] = onethird + onesixth * (rgb[2] - rgb[0]) / (cmax - cmin);
    }
    else
    {
      hsv[0] = twothird + onesixth * (rgb[0] - rgb[1]) / (cmax - cmin);
    }
    if (hsv[0] < 0.0f)
    {
      hsv[0] += 1.0f;
    }
  }
}

VTKM_EXEC
inline vtkm::Vec<float, 3> HSVToRGB(const vtkm::Vec<float, 3>& hsv)
{
  vtkm::Vec<float, 3> rgb;
  constexpr float onethird = 1.0f / 3.0f;
  constexpr float onesixth = 1.0f / 6.0f;
  constexpr float twothird = 2.0f / 3.0f;
  constexpr float fivesixth = 5.0f / 6.0f;

  // compute RGB from HSV
  if (hsv[0] > onesixth && hsv[0] <= onethird) // green/red
  {
    rgb[1] = 1.0f;
    rgb[0] = (onethird - hsv[0]) * 6.0f;
    rgb[2] = 0.0f;
  }
  else if (hsv[0] > onethird && hsv[0] <= 0.5f) // green/blue
  {
    rgb[1] = 1.0f;
    rgb[2] = (hsv[0] - onethird) * 6.0f;
    rgb[0] = 0.0f;
  }
  else if (hsv[0] > 0.5 && hsv[0] <= twothird) // blue/green
  {
    rgb[2] = 1.0f;
    rgb[1] = (twothird - hsv[0]) * 6.0f;
    rgb[0] = 0.0f;
  }
  else if (hsv[0] > twothird && hsv[0] <= fivesixth) // blue/red
  {
    rgb[2] = 1.0f;
    rgb[0] = (hsv[0] - twothird) * 6.0f;
    rgb[1] = 0.0f;
  }
  else if (hsv[0] > fivesixth && hsv[0] <= 1.0f) // red/blue
  {
    rgb[0] = 1.0f;
    rgb[2] = (1.0f - hsv[0]) * 6.0f;
    rgb[1] = 0.0f;
  }
  else // red/green
  {
    rgb[0] = 1.0f;
    rgb[1] = hsv[0] * 6.0f;
    rgb[2] = 0.0f;
  }

  // add Saturation to the equation.
  rgb[0] = (hsv[1] * rgb[0] + (1.0f - hsv[1]));
  rgb[1] = (hsv[1] * rgb[1] + (1.0f - hsv[1]));
  rgb[2] = (hsv[1] * rgb[2] + (1.0f - hsv[1]));

  rgb[0] *= hsv[2];
  rgb[1] *= hsv[2];
  rgb[2] *= hsv[2];
  return rgb;
}

VTKM_EXEC
inline void RGBToLab(const vtkm::Vec<float, 3>& rgb, vtkm::Vec<float, 3>& lab)
{
  // clang-format off

  // The following performs a "gamma correction" specified by the sRGB color
  // space.  sRGB is defined by a canonical definition of a display monitor and
  // has been standardized by the International Electrotechnical Commission (IEC
  // 61966-2-1).  The nonlinearity of the correction is designed to make the
  // colors more perceptually uniform.  This color space has been adopted by
  // several applications including Adobe Photoshop and Microsoft Windows color
  // management.  OpenGL is agnostic on its RGB color space, but it is reasonable
  // to assume it is close to this one.
  { //rgb to xyz start ( lab == xyz )
  float r = rgb[0];
  float g = rgb[1];
  float b = rgb[2];
  if ( r > 0.04045f ) r = vtkm::Pow(( r + 0.055f ) / 1.055f, 2.4f);
  else                r = r / 12.92f;
  if ( g > 0.04045f ) g = vtkm::Pow(( g + 0.055f ) / 1.055f, 2.4f);
  else                g = g / 12.92f;
  if ( b > 0.04045f ) b = vtkm::Pow(( b + 0.055f ) / 1.055f, 2.4f);
  else                b = b / 12.92f;

  //Observer. = 2 deg, Illuminant = D65
  lab[0] = r * 0.4124f + g * 0.3576f + b * 0.1805f;
  lab[1] = r * 0.2126f + g * 0.7152f + b * 0.0722f;
  lab[2] = r * 0.0193f + g * 0.1192f + b * 0.9505f;
  } //rgb to xyz end ( lab == xyz )

  //xyz to lab start
  constexpr float onethird = 1.0f / 3.0f;
  constexpr float sixteen_onesixteen = 16.0f / 116.0f;

  constexpr float ref_X = 0.9505f;
  constexpr float ref_Y = 1.000f;
  constexpr float ref_Z = 1.089f;
  float var_X = lab[0] / ref_X;
  float var_Y = lab[1] / ref_Y;
  float var_Z = lab[2] / ref_Z;

  if ( var_X > 0.008856f ) var_X = vtkm::Pow(var_X, onethird);
  else                     var_X = ( 7.787f * var_X ) + sixteen_onesixteen;
  if ( var_Y > 0.008856f ) var_Y = vtkm::Pow(var_Y, onethird);
  else                     var_Y = ( 7.787f * var_Y ) + sixteen_onesixteen;
  if ( var_Z > 0.008856f ) var_Z = vtkm::Pow(var_Z, onethird);
  else                     var_Z = ( 7.787f * var_Z ) + sixteen_onesixteen;

  //notice that we are mapping g => L, r => a, and b => b
  lab[0] = ( 116.0f * var_Y ) - 16.0f;
  lab[1] = 500.0f * ( var_X - var_Y );
  lab[2] = 200.0f * ( var_Y - var_Z );
  // clang-format on
}

VTKM_EXEC
inline vtkm::Vec<float, 3> LabToRGB(const vtkm::Vec<float, 3>& lab)
{
  // clang-format off
  vtkm::Vec<float, 3> rgb;
  { //lab to xyz start ( rgb == xyz )
  constexpr float sixteen_onesixteen = 16.0f / 116.0f;

  //notice that we are mapping L => g, a => r, and b => b
  rgb[1] = ( lab[0] + 16.0f ) / 116.0f;
  rgb[0] = lab[1] / 500.0f + rgb[1];
  rgb[2] = rgb[1] - lab[2] / 200.0f;

  if ( vtkm::Pow(rgb[0],3) > 0.008856f ) rgb[0] = vtkm::Pow(rgb[0],3);
  else rgb[0] = ( rgb[0] - sixteen_onesixteen ) / 7.787f;

  if ( vtkm::Pow(rgb[1],3) > 0.008856f ) rgb[1] = vtkm::Pow(rgb[1],3);
  else rgb[1] = ( rgb[1] - sixteen_onesixteen ) / 7.787f;

  if ( vtkm::Pow(rgb[2],3) > 0.008856f ) rgb[2] = vtkm::Pow(rgb[2],3);
  else rgb[2] = ( rgb[2] - sixteen_onesixteen ) / 7.787f;
  constexpr float ref_X = 0.9505f;
  constexpr float ref_Y = 1.000f;
  constexpr float ref_Z = 1.089f;
  rgb[0] *= ref_X; //Observer= 2 deg Illuminant= D65
  rgb[1] *= ref_Y;
  rgb[2] *= ref_Z;
  } // lab to xyz end

  //xyz to rgb start
  rgb = vtkm::Vec<float,3>(
    rgb[0] *  3.2406f + rgb[1] * -1.5372f + rgb[2] * -0.4986f,
    rgb[0] * -0.9689f + rgb[1] *  1.8758f + rgb[2] *  0.0415f,
    rgb[0] *  0.0557f + rgb[1] * -0.2040f + rgb[2] *  1.0570f);
  float& r = rgb[0];
  float& g = rgb[1];
  float& b = rgb[2];

  // The following performs a "gamma correction" specified by the sRGB color
  // space.  sRGB is defined by a canonical definition of a display monitor and
  // has been standardized by the International Electrotechnical Commission (IEC
  // 61966-2-1).  The nonlinearity of the correction is designed to make the
  // colors more perceptually uniform.  This color space has been adopted by
  // several applications including Adobe Photoshop and Microsoft Windows color
  // management.  OpenGL is agnostic on its RGB color space, but it is reasonable
  // to assume it is close to this one.
  constexpr float one_twopointfour = ( 1.0f / 2.4f );
  if (r > 0.0031308f) r = 1.055f * (vtkm::Pow(r, one_twopointfour)) - 0.055f;
  else r = 12.92f * r;
  if (g > 0.0031308f) g = 1.055f * (vtkm::Pow(g ,one_twopointfour)) - 0.055f;
  else  g = 12.92f * (g);
  if (b > 0.0031308f) b = 1.055f * (vtkm::Pow(b, one_twopointfour)) - 0.055f;
  else b = 12.92f * (b);

  // Clip colors. ideally we would do something that is perceptually closest
  // (since we can see colors outside of the display gamut), but this seems to
  // work well enough.
  const float maxVal = vtkm::Max(r, vtkm::Max(g,b));
  if (maxVal > 1.0f)
  {
    r /= maxVal;
    g /= maxVal;
    b /= maxVal;
  }
  r = vtkm::Max(r,0.0f);
  g = vtkm::Max(g,0.0f);
  b = vtkm::Max(b,0.0f);
  // clang-format on
  return rgb;
}

// Convert to a special polar version of CIELAB (useful for creating
// continuous diverging color maps).
VTKM_EXEC
inline void LabToMsh(const vtkm::Vec<float, 3>& lab, vtkm::Vec<float, 3>& msh)
{
  const float& L = lab[0];
  const float& a = lab[1];
  const float& b = lab[2];
  float& M = msh[0];
  float& s = msh[1];
  float& h = msh[2];

  M = vtkm::Sqrt(L * L + a * a + b * b);
  s = (M > 0.001f) ? vtkm::ACos(L / M) : 0.0f;
  h = (s > 0.001f) ? vtkm::ATan2(b, a) : 0.0f;
}

// Convert from a special polar version of CIELAB (useful for creating
// continuous diverging color maps).
VTKM_EXEC
inline vtkm::Vec<float, 3> MshToLab(const vtkm::Vec<float, 3>& msh)
{
  const float& M = msh[0];
  const float& s = msh[1];
  const float& h = msh[2];
  vtkm::Vec<float, 3> r(
    M * vtkm::Cos(s), M * vtkm::Sin(s) * vtkm::Cos(h), M * vtkm::Sin(s) * vtkm::Sin(h));
  return r;
}

// Given two angular orientations, returns the smallest angle between the two.
VTKM_EXEC
inline float DivergingAngleDiff(float a1, float a2)
{
  constexpr float f_pi = vtkm::Pif();
  constexpr float f_two_pi = vtkm::TwoPif();
  float adiff = a1 - a2;
  if (adiff < 0.0f)
    adiff = -adiff;
  while (adiff >= f_two_pi)
    adiff -= (f_two_pi);
  if (adiff > f_pi)
    adiff = (f_two_pi)-adiff;
  return adiff;
}

// For the case when interpolating from a saturated color to an unsaturated
// color, find a hue for the unsaturated color that makes sense.
VTKM_EXEC
inline float DivergingAdjustHue(const vtkm::Vec<float, 3>& msh, float unsatM)
{
  const float sinS = vtkm::Sin(msh[1]);

  if (msh[0] >= unsatM - 0.1f)
  {
    // The best we can do is hold hue constant.
    return msh[2];
  }
  else
  {
    // This equation is designed to make the perceptual change of the
    // interpolation to be close to constant.
    float hueSpin = msh[1] * vtkm::Sqrt(unsatM * unsatM - msh[0] * msh[0]) / (msh[0] * sinS);

    constexpr float one_third_pi = vtkm::Pi_3f();
    // Spin hue away from 0 except in purple hues.
    if (msh[2] > -one_third_pi)
    {
      return msh[2] + hueSpin;
    }
    else
    {
      return msh[2] - hueSpin;
    }
  }
}


} //namespace detail

//---------------------------------------------------------------------------
VTKM_EXEC
vtkm::Vec<float, 3> ColorTableBase::MapThroughColorSpace(double value) const
{
  vtkm::Vec<float, 3> rgb1, rgb2;
  float weight = 0;
  this->FindColors(value, rgb1, rgb2, weight);
  if (weight == 0)
  {
    return rgb1;
  }
  else if (weight == 1)
  {
    return rgb2;
  }
  else
  {
    return this->MapThroughColorSpace(rgb1, rgb2, weight);
  }
}

//---------------------------------------------------------------------------
VTKM_EXEC
void ColorTableBase::FindColors(double value,
                                vtkm::Vec<float, 3>& rgb1,
                                vtkm::Vec<float, 3>& rgb2,
                                float& weight) const
{
  // All the special cases have equivalent rgb1 and rgb2 values so we
  // set the weight to 0.0f as a default
  weight = 0.0f;

  if (vtkm::IsNan(value))
  { //If we are trying to color NaN use the special NaN color value
    rgb1 = this->NaNColor;
    rgb2 = this->NaNColor;
  }
  else if (this->ColorSize == 0)
  { //If we have no entries use the below range value
    rgb1 = this->BelowRangeColor;
    rgb2 = this->BelowRangeColor;
  }
  else if (value < this->ColorNodes[0])
  { //If we are below the color range
    rgb1 = (this->UseClamping) ? this->RGB[0] : this->BelowRangeColor;
    rgb2 = rgb1;
  }
  else if (value == this->ColorNodes[0])
  { //If we are exactly on the first color value
    rgb1 = this->RGB[0];
    rgb2 = rgb1;
  }
  else if (value > this->ColorNodes[this->ColorSize - 1])
  { //If we are above the color range
    rgb1 = (this->UseClamping) ? this->RGB[this->ColorSize - 1] : this->AboveRangeColor;
    rgb2 = rgb1;
  }
  else if (value == this->ColorNodes[this->ColorSize - 1])
  { //If we are exactly at the last color value
    rgb1 = this->RGB[this->ColorSize - 1];
    rgb2 = rgb1;
  }
  else
  {
    //The value is inside the range.
    //Note: this for loop can be optimized to a lower_bound call as it
    //accessing a sorted array
    vtkm::Int32 first = 0;
    vtkm::Int32 second = 1;
    for (; second < this->ColorSize; ++second, ++first)
    {
      if (this->ColorNodes[second] >= value)
      {
        //this is our spot
        break;
      }
    }
    rgb1 = this->RGB[first];
    rgb2 = this->RGB[second];

    const auto w =
      (value - this->ColorNodes[first]) / (this->ColorNodes[second] - this->ColorNodes[first]);
    weight = static_cast<float>(w);
  }
}

//---------------------------------------------------------------------------
VTKM_EXEC
float ColorTableBase::MapThroughOpacitySpace(double value) const
{
  if (vtkm::IsNan(value))
  { //If we are trying to find the opacity of NaN use a constant of 1.0
    return 1.0f;
  }
  else if (this->OpacitySize == 0)
  { //no opacity control functions so use a constant of 1.0
    return 1.0f;
  }
  else if (value <= this->ONodes[0])
  { //If we are below the color range
    return this->Alpha[0];
  }
  else if (value >= this->ONodes[this->OpacitySize - 1])
  { //If we are above the color range
    return this->Alpha[this->OpacitySize - 1];
  }
  else
  {
    //The value is inside the range.
    //Note: this for loop can be optimized to a lower_bound call as it
    //accessing a sorted array
    vtkm::Int32 first = 0;
    vtkm::Int32 second = 1;
    for (; second < this->OpacitySize; ++second, ++first)
    {
      if (this->ONodes[second] >= value)
      {
        //this is our spot
        break;
      }
    }
    const auto w = (value - this->ONodes[first]) / (this->ONodes[second] - this->ONodes[first]);
    float weight = static_cast<float>(w);

    //we only need the previous midpoint and sharpness as they control this region
    const auto& alpha1 = this->Alpha[first];
    const auto& alpha2 = this->Alpha[second];
    const auto& midsharp = this->MidSharp[first];
    if (weight < midsharp[0])
    {
      weight = 0.5f * weight / midsharp[0];
    }
    else
    {
      weight = 0.5f + 0.5f * (weight - midsharp[0]) / (1.0f - midsharp[0]);
    }

    if (midsharp[1] == 1.0f)
    {
      // override for sharpness == 1.0f
      // In this case we just want piecewise constant
      // Use the first value since when we are below the midpoint
      // otherwise use the second value
      return (weight < 0.5f) ? alpha1 : alpha2;
    }
    else if (midsharp[1] == 0.0f)
    {
      // Override for sharpness == 0.0f
      // In this case we simple linear interpolation
      return vtkm::Lerp(alpha1, alpha2, weight);
    }
    else
    {
      // We have a sharpness between [0.0, 1.0] (exclusive) - we will
      // used a modified hermite curve interpolation where we
      // derive the slope based on the sharpness, and we compress
      // the curve non-linearly based on the sharpness

      // First, we will adjust our position based on sharpness in
      // order to make the curve sharper (closer to piecewise constant)
      if (weight < 0.5f)
      {
        weight = 0.5f * vtkm::Pow(weight * 2.0f, 1.0f + 10.0f * midsharp[1]);
      }
      else if (weight > .5)
      {
        weight = 1.0f - 0.5f * vtkm::Pow((1.0f - weight) * 2.0f, 1.0f + 10.0f * midsharp[1]);
      }

      // Compute some coefficients we will need for the hermite curve
      const float ww = weight * weight;
      const float www = weight * weight * weight;

      const float h1 = 2.0f * www - 3.0f * ww + 1.0f;
      const float h2 = -2.0f * www + 3.0f * ww;
      const float h3 = www - 2.0f * ww + weight;
      const float h4 = www - ww;

      // Use one slope for both end points
      const float slope = alpha2 - alpha1;
      const float t = (1.0f - midsharp[1]) * slope;

      // Compute the value
      float result = h1 * alpha1 + h2 * alpha2 + h3 * t + h4 * t;

      // Final error check to make sure we don't go outside
      // the Y range
      result = vtkm::Max(result, vtkm::Min(alpha1, alpha2));
      result = vtkm::Min(result, vtkm::Max(alpha1, alpha2));
      return result;
    }
  }
}
//---------------------------------------------------------------------------
VTKM_EXEC
vtkm::Vec<float, 3> ColorTableRGB::MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                        const vtkm::Vec<float, 3>& rgb2,
                                                        float weight) const
{
  return vtkm::Lerp(rgb1, rgb2, weight);
}

//---------------------------------------------------------------------------
VTKM_EXEC
vtkm::Vec<float, 3> ColorTableHSV::MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                        const vtkm::Vec<float, 3>& rgb2,
                                                        float weight) const
{
  vtkm::Vec<float, 3> hsv1, hsv2;
  detail::RGBToHSV(rgb1, hsv1);
  detail::RGBToHSV(rgb2, hsv2);

  auto tmp = vtkm::Lerp(hsv1, hsv2, weight);
  if (tmp[0] < 0.0f)
  {
    tmp[0] += 1.0f;
  }
  return detail::HSVToRGB(tmp);
}

//---------------------------------------------------------------------------
VTKM_EXEC
vtkm::Vec<float, 3> ColorTableHSVWrap::MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                            const vtkm::Vec<float, 3>& rgb2,
                                                            float weight) const
{
  vtkm::Vec<float, 3> hsv1, hsv2;
  detail::RGBToHSV(rgb1, hsv1);
  detail::RGBToHSV(rgb2, hsv2);

  const float diff = hsv1[0] - hsv2[0];
  if (diff > 0.5f)
  {
    hsv1[0] -= 1.0f;
  }
  else if (diff < 0.5f)
  {
    hsv2[0] -= 1.0f;
  }

  auto tmp = vtkm::Lerp(hsv1, hsv2, weight);
  if (tmp[0] < 0.0f)
  {
    tmp[0] += 1.0f;
  }
  return detail::HSVToRGB(tmp);
}

//---------------------------------------------------------------------------
VTKM_EXEC
vtkm::Vec<float, 3> ColorTableLab::MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                        const vtkm::Vec<float, 3>& rgb2,
                                                        float weight) const
{
  vtkm::Vec<float, 3> lab1, lab2;
  detail::RGBToLab(rgb1, lab1);
  detail::RGBToLab(rgb2, lab2);

  auto tmp = vtkm::Lerp(lab1, lab2, weight);

  return detail::LabToRGB(tmp);
}

//---------------------------------------------------------------------------
VTKM_EXEC
vtkm::Vec<float, 3> ColorTableDiverging::MapThroughColorSpace(const vtkm::Vec<float, 3>& rgb1,
                                                              const vtkm::Vec<float, 3>& rgb2,
                                                              float weight) const
{
  vtkm::Vec<float, 3> lab1, lab2;
  detail::RGBToLab(rgb1, lab1);
  detail::RGBToLab(rgb2, lab2);

  vtkm::Vec<float, 3> msh1, msh2;
  detail::LabToMsh(lab1, msh1);
  detail::LabToMsh(lab2, msh2);
  // If the endpoints are distinct saturated colors, then place white in between
  // them.

  constexpr float one_third_pi = vtkm::Pi_3f();
  if ((msh1[1] > 0.05f) && (msh2[1] > 0.05f) &&
      (detail::DivergingAngleDiff(msh1[2], msh2[2]) > one_third_pi))
  {
    // Insert the white midpoint by setting one end to white and adjusting the
    // scalar value.
    float Mmid = vtkm::Max(msh1[0], msh2[0]);
    Mmid = vtkm::Max(88.0f, Mmid);
    if (weight < 0.5f)
    {
      msh2[0] = Mmid;
      msh2[1] = 0.0f;
      msh2[2] = 0.0f;
      weight = 2.0f * weight;
    }
    else
    {
      msh1[0] = Mmid;
      msh1[1] = 0.0f;
      msh1[2] = 0.0f;
      weight = 2.0f * weight - 1.0f;
    }
  }

  // If one color has no saturation, then its hue value is invalid.  In this
  // case, we want to set it to something logical so that the interpolation of
  // hue makes sense.
  if ((msh1[1] < 0.05f) && (msh2[1] > 0.05f))
  {
    msh1[2] = detail::DivergingAdjustHue(msh2, msh1[0]);
  }
  else if ((msh2[1] < 0.05f) && (msh1[1] > 0.05f))
  {
    msh2[2] = detail::DivergingAdjustHue(msh1, msh2[0]);
  }

  auto tmp = vtkm::Lerp(msh1, msh2, weight);
  tmp = detail::MshToLab(tmp);
  return detail::LabToRGB(tmp);
}
}
}

// Cuda seems to have a bug where it expects the template class VirtualObjectTransfer
// to be instantiated in a consistent order among all the translation units of an
// executable. Failing to do so results in random crashes and incorrect results.
// We workaroud this issue by explicitly instantiating VirtualObjectTransfer for
// all the portal types here.
#ifdef VTKM_CUDA
#include <vtkm/cont/cuda/internal/VirtualObjectTransferCuda.h>
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(vtkm::exec::ColorTableRGB);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(vtkm::exec::ColorTableHSV);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(vtkm::exec::ColorTableHSVWrap);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(vtkm::exec::ColorTableLab);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(vtkm::exec::ColorTableDiverging);
#endif

#endif
