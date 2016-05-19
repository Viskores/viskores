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

#include <string>
#include <vector>
#include <vtkm/rendering/Color.h>
namespace vtkm {
namespace rendering {
/// \brief It's a color table!
///
/// This class provides the basic representation of a color table. This class was 
/// Ported from EAVL. Originally created by Jeremy Meredith, Dave Pugmire, 
/// and Sean Ahern. This class uses seperate RGB and alpha control points and can
/// be used as a transfer function.
///
class ColorTable
{
  public:
    class ColorControlPoint
    {
      public:
        vtkm::Float32 Position;
        Color RGBA;
        ColorControlPoint(vtkm::Float32 position, const Color &rgba) 
          : Position(position), RGBA(rgba) 
        { }
    };

    class AlphaControlPoint
    {
      public:
        vtkm::Float32 Position;
        vtkm::Float32 AlphaValue;
        AlphaControlPoint(vtkm::Float32 position, const vtkm::Float32 &alphaValue) 
          : Position(position), AlphaValue(alphaValue) 
        { }
    };

  protected:
    std::string uniquename;
    bool smooth;
    std::vector<ColorControlPoint> RBGPoints;
    std::vector<AlphaControlPoint> AlphaPoints;
    typedef std::vector<AlphaControlPoint>::size_type AlphaType;
    typedef std::vector<ColorControlPoint>::size_type ColorType;
  public:
    const std::string &GetName() const
    {
      return uniquename;
    }
    bool GetSmooth() const
    {
      return smooth;
    }
    void Sample(int n, vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > &colors) const
    {
        colors.Allocate(n);
        
        for (int i=0; i<n; i++)
        {
          vtkm::Vec<vtkm::Float32,4> color;
          Color c = MapRGB(static_cast<vtkm::Float32>(i)/static_cast<vtkm::Float32>(n-1));
          color[0] = c.Components[0];
          color[1] = c.Components[1];
          color[2] = c.Components[2];
          color[3] = MapAlpha(static_cast<vtkm::Float32>(i)/static_cast<vtkm::Float32>(n-1));
          colors.GetPortalControl().Set(i,color);
        }
    }
    Color MapRGB(vtkm::Float32 c) const
    {
      ColorType n = RBGPoints.size(); 
      if (n == 0)
          return Color(0.5f, 0.5f, 0.5f);
      if (n == 1 || c <= RBGPoints[0].Position)
          return RBGPoints[0].RGBA;
      if (c >= RBGPoints[n - 1].Position)
          return RBGPoints[n - 1].RGBA;
      ColorType second;
      for (second = 1; second < n - 1; second++)
      {
          if (c < RBGPoints[second].Position)
              break;
      }
      ColorType first = second - 1;
      vtkm::Float32 seg = RBGPoints[second].Position-RBGPoints[first].Position;
      vtkm::Float32 alpha;
      if (seg == 0.f)
          alpha = .5f;
      else
          alpha = (c - RBGPoints[first].Position)/seg;
      if (smooth)
      {
          return Color(RBGPoints[first].RGBA.Components[0] * (1.f-alpha) + RBGPoints[second].RGBA.Components[0] * alpha,
                           RBGPoints[first].RGBA.Components[1] * (1.f-alpha) + RBGPoints[second].RGBA.Components[1] * alpha,
                           RBGPoints[first].RGBA.Components[2] * (1.f-alpha) + RBGPoints[second].RGBA.Components[2] * alpha);
      }
      else
      {
        if (alpha < .5)
          return RBGPoints[first].RGBA;
        else
          return RBGPoints[second].RGBA;
      }
            
    }

    vtkm::Float32 MapAlpha(vtkm::Float32 c) const
    {
      AlphaType n = AlphaPoints.size(); 
      // If no alpha control points were set, just return full opacity
      if (n == 0)
          return 1.f;
      if (n == 1 || c <= AlphaPoints[0].Position)
          return AlphaPoints[0].AlphaValue;
      if (c >= AlphaPoints[n-1].Position)
          return AlphaPoints[n-1].AlphaValue;
      AlphaType second;
      for (second=1; second<n-1; second++)
      {
          if (c < AlphaPoints[second].Position)
              break;
      }
      AlphaType first = second - 1;
      vtkm::Float32 seg = AlphaPoints[second].Position-AlphaPoints[first].Position;
      vtkm::Float32 alpha;
      if (seg == 0.f)
          alpha = .5;
      else
          alpha = (c - AlphaPoints[first].Position)/seg;
      if (smooth)
      {
          return (AlphaPoints[first].AlphaValue * (1.f-alpha) + AlphaPoints[second].AlphaValue * alpha);
      }
      else
      {
        if (alpha < .5)
          return AlphaPoints[first].AlphaValue;
        else
          return AlphaPoints[second].AlphaValue;
      }
            
    }
    ColorTable() : 
        uniquename(""), smooth(false)
    {
    }
    ColorTable(const ColorTable &ct) : 
        uniquename(ct.uniquename), smooth(ct.smooth), RBGPoints(ct.RBGPoints.begin(), ct.RBGPoints.end())
    {
    }
    void operator=(const ColorTable &ct)
    {
      uniquename = ct.uniquename;
      smooth = ct.smooth;
      RBGPoints.clear();
      RBGPoints.insert(RBGPoints.end(), ct.RBGPoints.begin(), ct.RBGPoints.end());
      AlphaPoints.insert(AlphaPoints.end(), ct.AlphaPoints.begin(), ct.AlphaPoints.end());
    }
    void Clear()
    {
        RBGPoints.clear();
        AlphaPoints.clear();
    }
    void AddControlPoint(vtkm::Float32 position, Color color)
    {
        RBGPoints.push_back(ColorControlPoint(position, color));
    }
    void AddControlPoint(vtkm::Float32 position, Color color, vtkm::Float32 alpha)
    {
        RBGPoints.push_back(ColorControlPoint(position, color));
        AlphaPoints.push_back(AlphaControlPoint(position, alpha));
    }
    void AddAlphaControlPoint(vtkm::Float32 position, vtkm::Float32 alpha)
    {
      AlphaPoints.push_back(AlphaControlPoint(position, alpha));
    }
    void Reverse()
    {   
        // copy old control points
        std::vector<ColorControlPoint> tmp(RBGPoints.begin(), RBGPoints.end());
        std::vector<AlphaControlPoint> tmpAlpha(AlphaPoints.begin(), AlphaPoints.end());
        Clear();
        vtkm::Int32 vectorSize = vtkm::Int32(tmp.size());
        for (vtkm::Int32 i = vectorSize - 1; i >= 0; --i)
            AddControlPoint(1.0f - tmp[ColorType(i)].Position, tmp[ColorType(i)].RGBA);
        vectorSize = vtkm::Int32(tmpAlpha.size());
        for (vtkm::Int32 i = vectorSize - 1; i >= 0; --i)
            AddAlphaControlPoint(1.0f - tmpAlpha[AlphaType(i)].Position, tmpAlpha[AlphaType(i)].AlphaValue);
        if (uniquename[1] == '0')
            uniquename[1] = '1';
        else
            uniquename[1] = '0';
    }
    ColorTable(std::string name)
    {
        if (name == "" || name == "default")
            name = "dense";

        smooth = true;
        if (name == "grey" || name == "gray")
        {
            AddControlPoint(0.0f, Color( 0.f, 0.f, 0.f));
            AddControlPoint(1.0f, Color( 1.f, 1.f, 1.f));
        }
        else if (name == "blue")
        {
            AddControlPoint(0.00f, Color( 0.f, 0.f, 0.f));
            AddControlPoint(0.33f, Color( 0.f, 0.f, .5f));
            AddControlPoint(0.66f, Color( 0.f, .5f, 1.f));
            AddControlPoint(1.00f, Color( 1.f, 1.f, 1.f));
        }
        else if (name == "orange")
        {
            AddControlPoint(0.00f, Color( 0.f, 0.f, 0.f));
            AddControlPoint(0.33f, Color( .5f, 0.f, 0.f));
            AddControlPoint(0.66f, Color( 1.f, .5f, 0.f));
            AddControlPoint(1.00f, Color( 1.f, 1.f, 1.f));
        }
        else if (name == "temperature")
        {
            AddControlPoint(0.05f, Color( 0.f, 0.f, 1.f));
            AddControlPoint(0.35f, Color( 0.f, 1.f, 1.f));
            AddControlPoint(0.50f, Color( 1.f, 1.f, 1.f));
            AddControlPoint(0.65f, Color( 1.f, 1.f, 0.f));
            AddControlPoint(0.95f, Color( 1.f, 0.f, 0.f));
        }
        else if (name == "rainbow")
        {
            AddControlPoint(0.00f, Color( 0.f, 0.f, 1.f));
            AddControlPoint(0.20f, Color( 0.f, 1.f, 1.f));
            AddControlPoint(0.45f, Color( 0.f, 1.f, 0.f));
            AddControlPoint(0.55f, Color( .7f, 1.f, 0.f));
            AddControlPoint(0.6f,  Color( 1.f, 1.f, 0.f));
            AddControlPoint(0.75f, Color( 1.f, .5f, 0.f));
            AddControlPoint(0.9f,  Color( 1.f, 0.f, 0.f));
            AddControlPoint(0.98f, Color( 1.f, 0.f, .5F));
            AddControlPoint(1.0f,  Color( 1.f, 0.f, 1.f));
        }
        else if (name == "levels")
        {
            AddControlPoint(0.0f, Color( 0.f, 0.f, 1.f));
            AddControlPoint(0.2f, Color( 0.f, 0.f, 1.f));
            AddControlPoint(0.2f, Color( 0.f, 1.f, 1.f));
            AddControlPoint(0.4f, Color( 0.f, 1.f, 1.f));
            AddControlPoint(0.4f, Color( 0.f, 1.f, 0.f));
            AddControlPoint(0.6f, Color( 0.f, 1.f, 0.f));
            AddControlPoint(0.6f, Color( 1.f, 1.f, 0.f));
            AddControlPoint(0.8f, Color( 1.f, 1.f, 0.f));
            AddControlPoint(0.8f, Color( 1.f, 0.f, 0.f));
            AddControlPoint(1.0f, Color( 1.f, 0.f, 0.f));
        }
        else if (name == "dense" || name == "sharp")
        {
            smooth = (name == "dense") ? true : false;
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
            AddControlPoint(0.25f,Color(1.00f, 0.00f, 0.00f));
            AddControlPoint(0.50f,Color(1.00f, 1.00f, 0.00f));
            AddControlPoint(0.55f,Color(0.80f, 0.55f, 0.20f));
            AddControlPoint(0.60f,Color(0.60f, 0.37f, 0.40f));
            AddControlPoint(0.65f,Color(0.40f, 0.22f, 0.60f));
            AddControlPoint(0.75f,Color(0.00f, 0.00f, 1.00f));
            AddControlPoint(1.00f,Color(1.00f, 1.00f, 1.00f));
        }
        // The following five tables are perceeptually linearized colortables
        // (4 rainbow, one heatmap) from BSD-licensed code by Matteo Niccoli.
        // See: http://mycarta.wordpress.com/2012/05/29/the-rainbow-is-dead-long-live-the-rainbow-series-outline/
        else if (name == "IsoL")
        {
            vtkm::Float32 n = 5;
            AddControlPoint(0.f/n,  Color(0.9102f, 0.2236f, 0.8997f));
            AddControlPoint(1.f/n,  Color(0.4027f, 0.3711f, 1.0000f));
            AddControlPoint(2.f/n,  Color(0.0422f, 0.5904f, 0.5899f));
            AddControlPoint(3.f/n,  Color(0.0386f, 0.6206f, 0.0201f));
            AddControlPoint(4.f/n,  Color(0.5441f, 0.5428f, 0.0110f));
            AddControlPoint(5.f/n,  Color(1.0000f, 0.2288f, 0.1631f));
        }
        else if (name == "CubicL")
        {
            vtkm::Float32 n = 15;
            AddControlPoint(0.f/n,  Color(0.4706f, 0.0000f, 0.5216f));
            AddControlPoint(1.f/n,  Color(0.5137f, 0.0527f, 0.7096f));
            AddControlPoint(2.f/n,  Color(0.4942f, 0.2507f, 0.8781f));
            AddControlPoint(3.f/n,  Color(0.4296f, 0.3858f, 0.9922f));
            AddControlPoint(4.f/n,  Color(0.3691f, 0.5172f, 0.9495f));
            AddControlPoint(5.f/n,  Color(0.2963f, 0.6191f, 0.8515f));
            AddControlPoint(6.f/n,  Color(0.2199f, 0.7134f, 0.7225f));
            AddControlPoint(7.f/n,  Color(0.2643f, 0.7836f, 0.5756f));
            AddControlPoint(8.f/n,  Color(0.3094f, 0.8388f, 0.4248f));
            AddControlPoint(9.f/n,  Color(0.3623f, 0.8917f, 0.2858f));
            AddControlPoint(10.f/n, Color(0.5200f, 0.9210f, 0.3137f));
            AddControlPoint(11.f/n, Color(0.6800f, 0.9255f, 0.3386f));
            AddControlPoint(12.f/n, Color(0.8000f, 0.9255f, 0.3529f));
            AddControlPoint(13.f/n, Color(0.8706f, 0.8549f, 0.3608f));
            AddControlPoint(14.f/n, Color(0.9514f, 0.7466f, 0.3686f));
            AddControlPoint(15.f/n, Color(0.9765f, 0.5887f, 0.3569f));
        }
        else if (name == "CubicYF")
        {
            vtkm::Float32 n = 15;
            AddControlPoint(0.f/n,  Color(0.5151f, 0.0482f, 0.6697f));
            AddControlPoint(1.f/n,  Color(0.5199f, 0.1762f, 0.8083f));
            AddControlPoint(2.f/n,  Color(0.4884f, 0.2912f, 0.9234f));
            AddControlPoint(3.f/n,  Color(0.4297f, 0.3855f, 0.9921f));
            AddControlPoint(4.f/n,  Color(0.3893f, 0.4792f, 0.9775f));
            AddControlPoint(5.f/n,  Color(0.3337f, 0.5650f, 0.9056f));
            AddControlPoint(6.f/n,  Color(0.2795f, 0.6419f, 0.8287f));
            AddControlPoint(7.f/n,  Color(0.2210f, 0.7123f, 0.7258f));
            AddControlPoint(8.f/n,  Color(0.2468f, 0.7612f, 0.6248f));
            AddControlPoint(9.f/n,  Color(0.2833f, 0.8125f, 0.5069f));
            AddControlPoint(10.f/n, Color(0.3198f, 0.8492f, 0.3956f));
            AddControlPoint(11.f/n, Color(0.3602f, 0.8896f, 0.2919f));
            AddControlPoint(12.f/n, Color(0.4568f, 0.9136f, 0.3018f));
            AddControlPoint(13.f/n, Color(0.6033f, 0.9255f, 0.3295f));
            AddControlPoint(14.f/n, Color(0.7066f, 0.9255f, 0.3414f));
            AddControlPoint(15.f/n, Color(0.8000f, 0.9255f, 0.3529f));
        }
        else if (name == "LinearL")
        {
            vtkm::Float32 n = 15;
            AddControlPoint(0.f/n,  Color(0.0143f, 0.0143f, 0.0143f));
            AddControlPoint(1.f/n,  Color(0.1413f, 0.0555f, 0.1256f));
            AddControlPoint(2.f/n,  Color(0.1761f, 0.0911f, 0.2782f));
            AddControlPoint(3.f/n,  Color(0.1710f, 0.1314f, 0.4540f));
            AddControlPoint(4.f/n,  Color(0.1074f, 0.2234f, 0.4984f));
            AddControlPoint(5.f/n,  Color(0.0686f, 0.3044f, 0.5068f));
            AddControlPoint(6.f/n,  Color(0.0008f, 0.3927f, 0.4267f));
            AddControlPoint(7.f/n,  Color(0.0000f, 0.4763f, 0.3464f));
            AddControlPoint(8.f/n,  Color(0.0000f, 0.5565f, 0.2469f));
            AddControlPoint(9.f/n,  Color(0.0000f, 0.6381f, 0.1638f));
            AddControlPoint(10.f/n, Color(0.2167f, 0.6966f, 0.0000f));
            AddControlPoint(11.f/n, Color(0.3898f, 0.7563f, 0.0000f));
            AddControlPoint(12.f/n, Color(0.6912f, 0.7795f, 0.0000f));
            AddControlPoint(13.f/n, Color(0.8548f, 0.8041f, 0.4555f));
            AddControlPoint(14.f/n, Color(0.9712f, 0.8429f, 0.7287f));
            AddControlPoint(15.f/n, Color(0.9692f, 0.9273f, 0.8961f));
        }
        else if (name == "LinLhot")
        {
            vtkm::Float32 n = 15;
            AddControlPoint(0.f/n,  Color(0.0225f, 0.0121f, 0.0121f));
            AddControlPoint(1.f/n,  Color(0.1927f, 0.0225f, 0.0311f));
            AddControlPoint(2.f/n,  Color(0.3243f, 0.0106f, 0.0000f));
            AddControlPoint(3.f/n,  Color(0.4463f, 0.0000f, 0.0091f));
            AddControlPoint(4.f/n,  Color(0.5706f, 0.0000f, 0.0737f));
            AddControlPoint(5.f/n,  Color(0.6969f, 0.0000f, 0.1337f));
            AddControlPoint(6.f/n,  Color(0.8213f, 0.0000f, 0.1792f));
            AddControlPoint(7.f/n,  Color(0.8636f, 0.0000f, 0.0565f));
            AddControlPoint(8.f/n,  Color(0.8821f, 0.2555f, 0.0000f));
            AddControlPoint(9.f/n,  Color(0.8720f, 0.4182f, 0.0000f));
            AddControlPoint(10.f/n, Color(0.8424f, 0.5552f, 0.0000f));
            AddControlPoint(11.f/n, Color(0.8031f, 0.6776f, 0.0000f));
            AddControlPoint(12.f/n, Color(0.7659f, 0.7870f, 0.0000f));
            AddControlPoint(13.f/n, Color(0.8170f, 0.8296f, 0.0000f));
            AddControlPoint(14.f/n, Color(0.8853f, 0.8896f, 0.4113f));
            AddControlPoint(15.f/n, Color(0.9481f, 0.9486f, 0.7165f));
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
           std::cout<<"Unknown Color Table"<<std::endl;
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
        uniquename = std::string("00") + name;
        if (smooth)
            uniquename[0] = '1';
    }
};
}}//namespace vtkm::rendering
#endif //vtk_m_rendering_ColorTable_h


