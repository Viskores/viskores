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
#ifndef vtk_m_rendering_Color_h
#define vtk_m_rendering_Color_h

#include <iostream>
#include <vtkm/cont/DataSet.h>
namespace vtkm {
namespace rendering {
/// \brief It's a color!
///
/// This class provides the basic representation of a color. This class was 
/// Ported from EAVL. Originally created by Jeremy Meredith, Dave Pugmire, 
/// and Sean Ahern.
///
class Color
{
  public:
    vtkm::Float32 Components[4];
    Color()
    {
      Components[0] = 0;
      Components[1] = 0;
      Components[2] = 0;
      Components[3] = 1;
    }
    Color(vtkm::Float32 r_, 
          vtkm::Float32 g_, 
          vtkm::Float32 b_,
          vtkm::Float32 a_ = 1.f)
    {
      Components[0] = r_;
      Components[1] = g_;
      Components[2] = b_;
      Components[3] = a_;
    }
    inline void SetComponentFromByte(vtkm::Int32 i, vtkm::UInt8 v)
    {
      // Note that though GetComponentAsByte below
      // multiplies by 256, we're dividing by 255. here.
      // This is, believe it or not, still correct.
      // That's partly because we always round down in
      // that method.  For example, if we set the float
      // here using byte(1), /255 gives us .00392, which
      // *256 gives us 1.0035, which is then rounded back
      // down to byte(1) below.  Or, if we set the float
      // here using byte(254), /255 gives us .99608, which
      // *256 gives us 254.996, which is then rounded
      // back down to 254 below.  So it actually reverses
      // correctly, even though the mutliplier and
      // divider don't match between these two methods.
      //
      // Of course, converting in GetComponentAsByte from
      // 1.0 gives 256, so we need to still clamp to 255
      // anyway.  Again, this is not a problem, because it 
      // doesn't really extend the range of floating point 
      // values which map to 255.
      Components[i] = float(v) / 255.f;
      // clamp?
      if (Components[i]<0) Components[i] = 0;
      if (Components[i]>1) Components[i] = 1;
    }
    inline vtkm::UInt8 GetComponentAsByte(int i)
    {
      // We need this to match what OpenGL/Mesa do.
      // Why?  Well, we need to set glClearColor
      // using floats, but the frame buffer comes
      // back as bytes (and is internally such) in
      // most cases.  In one example -- parallel 
      // compositing -- we need the byte values
      // returned from here to match the byte values
      // returned in the frame buffer.  Though
      // a quick source code inspection of Mesa
      // led me to believe I should do *255., in 
      // fact this led to a mismatch.  *256. was
      // actually closer.  (And arguably more correct
      // if you think the byte value 255 should share
      // approximately the same range in the float [0,1]
      // space as the other byte values.)  Note in the
      // inverse method above, though, we still use 255;
      // see SetComponentFromByte for an explanation of
      // why that is correct, if non-obvious.

      int tv = vtkm::Int32(Components[i] * 256.f);
      // Converting even from valid values (i.e 1.0)
      // can give a result outside the range (i.e. 256),
      // but we have to clamp anyway.
      return vtkm::UInt8((tv < 0) ? 0 : (tv > 255) ? 255 : tv);
    }
    void GetRGBA(vtkm::UInt8 &r, vtkm::UInt8 &g,
                 vtkm::UInt8 &b, vtkm::UInt8 &a)
    {
      r = GetComponentAsByte(0);
      g = GetComponentAsByte(1);
      b = GetComponentAsByte(2);
      a = GetComponentAsByte(3);
    }
    vtkm::Float64 RawBrightness()
    {
      return (Components[0]+Components[1]+Components[2])/3.;
    }
    friend std::ostream &operator<<(std::ostream &out, const Color &c)
    {
      out << "["<<c.Components[0]<<","<<c.Components[1]<<","<<c.Components[2]<<","<<c.Components[3]<<"]";
      return out;
    }
};
}}//namespace vtkm::rendering
#endif //vtk_m_rendering_Color_h