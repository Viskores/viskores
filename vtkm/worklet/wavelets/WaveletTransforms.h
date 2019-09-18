//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_Wavelets_h
#define vtk_m_worklet_Wavelets_h

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/Math.h>

namespace vtkm
{
namespace worklet
{
namespace wavelets
{

enum DWTMode
{ // boundary extension modes
  SYMH,
  SYMW,
  ASYMH,
  ASYMW
};

enum ExtensionDirection
{         // which side of a cube to extend
  LEFT,   // X direction
  RIGHT,  // X direction     Y
  TOP,    // Y direction     |   Z
  BOTTOM, // Y direction     |  /
  FRONT,  // Z direction     | /
  BACK    // Z direction     |/________ X
};

// Worklet for 3D signal extension
// It operates on a specified part of a big cube
class ExtensionWorklet3D : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension
                                WholeArrayIn); // signal
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  ExtensionWorklet3D(vtkm::Id extdimX,
                     vtkm::Id extdimY,
                     vtkm::Id extdimZ,
                     vtkm::Id sigdimX,
                     vtkm::Id sigdimY,
                     vtkm::Id sigdimZ,
                     vtkm::Id sigstartX,
                     vtkm::Id sigstartY,
                     vtkm::Id sigstartZ,
                     vtkm::Id sigpretendX,
                     vtkm::Id sigpretendY,
                     vtkm::Id sigpretendZ,
                     DWTMode m, // SYMH, SYMW, etc.
                     ExtensionDirection dir,
                     bool pad_zero)
    : extDimX(extdimX)
    , extDimY(extdimY)
    , extDimZ(extdimZ)
    , sigDimX(sigdimX)
    , sigDimY(sigdimY)
    , sigDimZ(sigdimZ)
    , sigStartX(sigstartX)
    , sigStartY(sigstartY)
    , sigStartZ(sigstartZ)
    , sigPretendDimX(sigpretendX)
    , sigPretendDimY(sigpretendY)
    , sigPretendDimZ(sigpretendZ)
    , mode(m)
    , direction(dir)
    , padZero(pad_zero)
  {
    (void)sigDimZ;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  void Ext1Dto3D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y, vtkm::Id& z) const
  {
    z = idx / (extDimX * extDimY);
    y = (idx - z * extDimX * extDimY) / extDimX;
    x = idx % extDimX;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  vtkm::Id Sig3Dto1D(vtkm::Id x, vtkm::Id y, vtkm::Id z) const
  {
    return z * sigDimX * sigDimY + y * sigDimX + x;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  vtkm::Id SigPretend3Dto1D(vtkm::Id x, vtkm::Id y, vtkm::Id z) const
  {
    return (z + sigStartZ) * sigDimX * sigDimY + (y + sigStartY) * sigDimX + x + sigStartX;
  }

  template <typename PortalOutType, typename PortalInType>
  VTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const vtkm::Id& workIndex) const
  {
    vtkm::Id extX, extY, extZ;
    vtkm::Id sigPretendX, sigPretendY, sigPretendZ;
    Ext1Dto3D(workIndex, extX, extY, extZ);
    typename PortalOutType::ValueType sym = 1.0;
    if (mode == ASYMH || mode == ASYMW)
    {
      sym = -1.0;
    }
    if (direction == LEFT)
    {
      sigPretendY = extY;
      sigPretendZ = extZ;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendX = extDimX - extX - 1;
      }
      else // mode == SYMW || mode == ASYMW
      {
        sigPretendX = extDimX - extX;
      }
    }
    else if (direction == RIGHT)
    {
      sigPretendY = extY;
      sigPretendZ = extZ;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendX = sigPretendDimX - extX - 1;
      }
      else
      {
        sigPretendX = sigPretendDimX - extX - 2;
      }
      if (padZero)
      {
        sigPretendX++;
      }
    }
    else if (direction == TOP)
    {
      sigPretendX = extX;
      sigPretendZ = extZ;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendY = extDimY - extY - 1;
      }
      else // mode == SYMW || mode == ASYMW
      {
        sigPretendY = extDimY - extY;
      }
    }
    else if (direction == BOTTOM)
    {
      sigPretendX = extX;
      sigPretendZ = extZ;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendY = sigPretendDimY - extY - 1;
      }
      else
      {
        sigPretendY = sigPretendDimY - extY - 2;
      }
      if (padZero)
      {
        sigPretendY++;
      }
    }
    else if (direction == FRONT)
    {
      sigPretendX = extX;
      sigPretendY = extY;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendZ = extDimZ - extZ - 1;
      }
      else // mode == SYMW || mode == ASYMW
      {
        sigPretendZ = extDimZ - extZ;
      }
    }
    else if (direction == BACK)
    {
      sigPretendX = extX;
      sigPretendY = extY;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendZ = sigPretendDimZ - extZ - 1;
      }
      else
      {
        sigPretendZ = sigPretendDimZ - extZ - 2;
      }
      if (padZero)
      {
        sigPretendZ++;
      }
    }
    else
    {
      sigPretendX = sigPretendY = sigPretendZ = 0; // so the compiler doesn't complain
    }

    if (sigPretendX == sigPretendDimX || // decides to pad a zero
        sigPretendY == sigPretendDimY ||
        sigPretendZ == sigPretendDimZ)
    {
      portalOut.Set(workIndex, 0.0);
    }
    else
    {
      portalOut.Set(workIndex,
                    sym * portalIn.Get(SigPretend3Dto1D(sigPretendX, sigPretendY, sigPretendZ)));
    }
  }

private:
  const vtkm::Id extDimX, extDimY, extDimZ, sigDimX, sigDimY, sigDimZ;
  const vtkm::Id sigStartX, sigStartY, sigStartZ;                // defines a small cube to work on
  const vtkm::Id sigPretendDimX, sigPretendDimY, sigPretendDimZ; // small cube dims
  const DWTMode mode;
  const ExtensionDirection direction;
  const bool padZero; // treat sigIn as having a zero at the end
};

//=============================================================================

//  Y
//
//  |      Z
//  |     /
//  |    /
//  |   /
//  |  /
//  | /
//  |/------------- X
//
// The following 3 classes perform the same functionaliry in 3 directions
class IndexTranslator3CubesLeftRight
{
public:
  IndexTranslator3CubesLeftRight(vtkm::Id x_1,
                                 vtkm::Id y_1,
                                 vtkm::Id z_1,
                                 vtkm::Id x_2,
                                 vtkm::Id y_2,
                                 vtkm::Id z_2,
                                 vtkm::Id startx_2,
                                 vtkm::Id starty_2,
                                 vtkm::Id startz_2,
                                 vtkm::Id pretendx_2,
                                 vtkm::Id pretendy_2,
                                 vtkm::Id pretendz_2,
                                 vtkm::Id x_3,
                                 vtkm::Id y_3,
                                 vtkm::Id z_3)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , startX2(startx_2)
    , startY2(starty_2)
    , startZ2(startz_2)
    , pretendDimX2(pretendx_2)
    , pretendDimY2(pretendy_2)
    , pretendDimZ2(pretendz_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
  {
    (void)dimZ1;
    (void)dimZ2;
    (void)dimZ3;
    (void)pretendDimY2;
    (void)pretendDimZ2;
  }

  VTKM_EXEC_CONT
  void Translate3Dto1D(vtkm::Id inX,
                       vtkm::Id inY,
                       vtkm::Id inZ, // 3D indices as input
                       vtkm::Id& cube,
                       vtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimX1 <= inX && inX < (dimX1 + pretendDimX2))
    {
      vtkm::Id inX_local = inX - dimX1;
      cube = 2;
      idx = (inZ + startZ2) * dimX2 * dimY2 + (inY + startY2) * dimX2 + (inX_local + startX2);
    }
    else if (0 <= inX && inX < dimX1)
    {
      cube = 1;
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimX1 + pretendDimX2) <= inX && inX < (dimX1 + pretendDimX2 + dimX3))
    {
      vtkm::Id inX_local = inX - dimX1 - pretendDimX2;
      cube = 3;
      idx = inZ * dimX3 * dimY3 + inY * dimX3 + inX_local;
    }
  }

private:
  const vtkm::Id dimX1, dimY1, dimZ1; // left extension
  const vtkm::Id dimX2, dimY2, dimZ2; // actual signal dims
  const vtkm::Id startX2, startY2, startZ2, pretendDimX2, pretendDimY2, pretendDimZ2;
  const vtkm::Id dimX3, dimY3, dimZ3; // right extension
};

class IndexTranslator3CubesTopDown
{
public:
  IndexTranslator3CubesTopDown(vtkm::Id x_1,
                               vtkm::Id y_1,
                               vtkm::Id z_1,
                               vtkm::Id x_2,
                               vtkm::Id y_2,
                               vtkm::Id z_2,
                               vtkm::Id startx_2,
                               vtkm::Id starty_2,
                               vtkm::Id startz_2,
                               vtkm::Id pretendx_2,
                               vtkm::Id pretendy_2,
                               vtkm::Id pretendz_2,
                               vtkm::Id x_3,
                               vtkm::Id y_3,
                               vtkm::Id z_3)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , startX2(startx_2)
    , startY2(starty_2)
    , startZ2(startz_2)
    , pretendDimX2(pretendx_2)
    , pretendDimY2(pretendy_2)
    , pretendDimZ2(pretendz_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
  {
    (void)dimZ1;
    (void)dimZ2;
    (void)dimZ3;
    (void)pretendDimX2;
    (void)pretendDimZ2;
  }

  VTKM_EXEC_CONT
  void Translate3Dto1D(vtkm::Id inX,
                       vtkm::Id inY,
                       vtkm::Id inZ, // 3D indices as input
                       vtkm::Id& cube,
                       vtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimY1 <= inY && inY < (dimY1 + pretendDimY2))
    {
      vtkm::Id inY_local = inY - dimY1;
      cube = 2;
      idx = (inZ + startZ2) * dimX2 * dimY2 + (inY_local + startY2) * dimX2 + inX + startX2;
    }
    else if (0 <= inY && inY < dimY1)
    {
      cube = 1;
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimY1 + pretendDimY2) <= inY && inY < (dimY1 + pretendDimY2 + dimY3))
    {
      vtkm::Id inY_local = inY - dimY1 - pretendDimY2;
      cube = 3;
      idx = inZ * dimX3 * dimY3 + inY_local * dimX3 + inX;
    }
  }

private:
  const vtkm::Id dimX1, dimY1, dimZ1; // left extension
  const vtkm::Id dimX2, dimY2, dimZ2; // actual signal dims
  const vtkm::Id startX2, startY2, startZ2, pretendDimX2, pretendDimY2, pretendDimZ2;
  const vtkm::Id dimX3, dimY3, dimZ3; // right extension
};

class IndexTranslator3CubesFrontBack
{
public:
  IndexTranslator3CubesFrontBack(vtkm::Id x_1,
                                 vtkm::Id y_1,
                                 vtkm::Id z_1,
                                 vtkm::Id x_2,
                                 vtkm::Id y_2,
                                 vtkm::Id z_2,
                                 vtkm::Id startx_2,
                                 vtkm::Id starty_2,
                                 vtkm::Id startz_2,
                                 vtkm::Id pretendx_2,
                                 vtkm::Id pretendy_2,
                                 vtkm::Id pretendz_2,
                                 vtkm::Id x_3,
                                 vtkm::Id y_3,
                                 vtkm::Id z_3)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , startX2(startx_2)
    , startY2(starty_2)
    , startZ2(startz_2)
    , pretendDimX2(pretendx_2)
    , pretendDimY2(pretendy_2)
    , pretendDimZ2(pretendz_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
  {
    (void)dimZ2;
    (void)pretendDimX2;
    (void)pretendDimY2;
  }

  VTKM_EXEC_CONT
  void Translate3Dto1D(vtkm::Id inX,
                       vtkm::Id inY,
                       vtkm::Id inZ, // 3D indices as input
                       vtkm::Id& cube,
                       vtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimZ1 <= inZ && inZ < (dimZ1 + pretendDimZ2))
    {
      vtkm::Id inZ_local = inZ - dimZ1;
      cube = 2;
      idx = (inZ_local + startZ2) * dimX2 * dimY2 + (inY + startY2) * dimX2 + inX + startX2;
    }
    else if (0 <= inZ && inZ < dimZ1)
    {
      cube = 1;
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimZ1 + pretendDimZ2) <= inZ && inZ < (dimZ1 + pretendDimZ2 + dimZ3))
    {
      vtkm::Id inZ_local = inZ - dimZ1 - pretendDimZ2;
      cube = 3;
      idx = inZ_local * dimX3 * dimY3 + inY * dimX3 + inX;
    }
  }

private:
  const vtkm::Id dimX1, dimY1, dimZ1; // left extension
  const vtkm::Id dimX2, dimY2, dimZ2; // actual signal dims
  const vtkm::Id startX2, startY2, startZ2, pretendDimX2, pretendDimY2, pretendDimZ2;
  const vtkm::Id dimX3, dimY3, dimZ3; // right extension
};

//=============================================================================

//
//  ---------------------------------------------------
//  |      |          |      |      |          |      |
//  |cube1 |  cube5   |cube2 |cube3 |  cube5   |cube4 |
//  | ext1 |    cA    | ext2 | ext3 |    cD    | ext4 |
//  | (x1) |   (xa)   | (x2) | (x3) |   (xd)   | (x4) |
//  |      |          |      |      |          |      |
//  ----------------------------------------------------
// The following 3 classes perform the same functionaliry in 3 directions
class IndexTranslator6CubesLeftRight
{
public:
  IndexTranslator6CubesLeftRight(vtkm::Id x_1,
                                 vtkm::Id y_1,
                                 vtkm::Id z_1,
                                 vtkm::Id x_2,
                                 vtkm::Id y_2,
                                 vtkm::Id z_2,
                                 vtkm::Id x_3,
                                 vtkm::Id y_3,
                                 vtkm::Id z_3,
                                 vtkm::Id x_4,
                                 vtkm::Id y_4,
                                 vtkm::Id z_4,
                                 vtkm::Id x_a,
                                 vtkm::Id y_a,
                                 vtkm::Id z_a,
                                 vtkm::Id x_d,
                                 vtkm::Id y_d,
                                 vtkm::Id z_d,
                                 vtkm::Id x_5,
                                 vtkm::Id y_5,
                                 vtkm::Id z_5,
                                 vtkm::Id start_x5,
                                 vtkm::Id start_y5,
                                 vtkm::Id start_z5)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
    , dimX4(x_4)
    , dimY4(y_4)
    , dimZ4(z_4)
    , dimXa(x_a)
    , dimYa(y_a)
    , dimZa(z_a)
    , dimXd(x_d)
    , dimYd(y_d)
    , dimZd(z_d)
    , dimX5(x_5)
    , dimY5(y_5)
    , dimZ5(z_5)
    , startX5(start_x5)
    , startY5(start_y5)
    , startZ5(start_z5)
  {
    (void)dimYa;
    (void)dimYd;
    (void)dimZa;
    (void)dimZd;
    (void)dimZ5;
    (void)dimZ4;
    (void)dimZ3;
    (void)dimZ2;
    (void)dimZ1;
  }

  VTKM_EXEC_CONT
  void Translate3Dto1D(vtkm::Id inX,
                       vtkm::Id inY,
                       vtkm::Id inZ, // 3D indices as input
                       vtkm::Id& cube,
                       vtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimX1 <= inX && inX < (dimX1 + dimXa))
    {
      vtkm::Id inX_local = inX - dimX1;
      cube = 5; // cAcD
      idx = (inZ + startZ5) * dimX5 * dimY5 + (inY + startY5) * dimX5 + (inX_local + startX5);
    }
    else if ((dimX1 + dimXa + dimX2 + dimX3) <= inX &&
             inX < (dimX1 + dimXa + dimX2 + dimX3 + dimXd))
    {
      vtkm::Id inX_local = inX - dimX1 - dimX2 - dimX3; // no -dimXa since this is on the same cube
      cube = 5;                                         // cAcD
      idx = (inZ + startZ5) * dimX5 * dimY5 + (inY + startY5) * dimX5 + (inX_local + startX5);
    }
    else if (0 <= inX && inX < dimX1)
    {
      cube = 1; // ext1
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimX1 + dimXa) <= inX && inX < (dimX1 + dimXa + dimX2))
    {
      vtkm::Id inX_local = inX - dimX1 - dimXa;
      cube = 2; // ext2
      idx = inZ * dimX2 * dimY2 + inY * dimX2 + inX_local;
    }
    else if ((dimX1 + dimXa + dimX2) <= inX && inX < (dimX1 + dimXa + dimX2 + dimX3))
    {
      vtkm::Id inX_local = inX - dimX1 - dimXa - dimX2;
      cube = 3; // ext3
      idx = inZ * dimX3 * dimY3 + inY * dimX3 + inX_local;
    }
    else if ((dimX1 + dimXa + dimX2 + dimX3 + dimXd) <= inX &&
             inX < (dimX1 + dimXa + dimX2 + dimX3 + dimXd + dimX4))
    {
      vtkm::Id inX_local = inX - dimX1 - dimXa - dimX2 - dimX3 - dimXd;
      cube = 4; // ext4
      idx = inZ * dimX4 * dimY4 + inY * dimX4 + inX_local;
    }
  }

private:
  const vtkm::Id dimX1, dimY1, dimZ1, dimX2, dimY2, dimZ2;       // extension cube sizes
  const vtkm::Id dimX3, dimY3, dimZ3, dimX4, dimY4, dimZ4;       // extension cube sizes
  const vtkm::Id dimXa, dimYa, dimZa, dimXd, dimYd, dimZd;       // signal cube sizes
  const vtkm::Id dimX5, dimY5, dimZ5, startX5, startY5, startZ5; // entire cube size
};

class IndexTranslator6CubesTopDown
{
public:
  IndexTranslator6CubesTopDown(vtkm::Id x_1,
                               vtkm::Id y_1,
                               vtkm::Id z_1,
                               vtkm::Id x_2,
                               vtkm::Id y_2,
                               vtkm::Id z_2,
                               vtkm::Id x_3,
                               vtkm::Id y_3,
                               vtkm::Id z_3,
                               vtkm::Id x_4,
                               vtkm::Id y_4,
                               vtkm::Id z_4,
                               vtkm::Id x_a,
                               vtkm::Id y_a,
                               vtkm::Id z_a,
                               vtkm::Id x_d,
                               vtkm::Id y_d,
                               vtkm::Id z_d,
                               vtkm::Id x_5,
                               vtkm::Id y_5,
                               vtkm::Id z_5,
                               vtkm::Id start_x5,
                               vtkm::Id start_y5,
                               vtkm::Id start_z5)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
    , dimX4(x_4)
    , dimY4(y_4)
    , dimZ4(z_4)
    , dimXa(x_a)
    , dimYa(y_a)
    , dimZa(z_a)
    , dimXd(x_d)
    , dimYd(y_d)
    , dimZd(z_d)
    , dimX5(x_5)
    , dimY5(y_5)
    , dimZ5(z_5)
    , startX5(start_x5)
    , startY5(start_y5)
    , startZ5(start_z5)
  {
    (void)dimXa;
    (void)dimXd;
    (void)dimZd;
    (void)dimZa;
    (void)dimZ5;
    (void)dimZ4;
    (void)dimZ3;
    (void)dimZ2;
    (void)dimZ1;
  }

  VTKM_EXEC_CONT
  void Translate3Dto1D(vtkm::Id inX,
                       vtkm::Id inY,
                       vtkm::Id inZ, // 3D indices as input
                       vtkm::Id& cube,
                       vtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimY1 <= inY && inY < (dimY1 + dimYa))
    {
      vtkm::Id inY_local = inY - dimY1;
      cube = 5; // cAcD
      idx = (inZ + startZ5) * dimX5 * dimY5 + (inY_local + startY5) * dimX5 + (inX + startX5);
    }
    else if ((dimY1 + dimYa + dimY2 + dimY3) <= inY &&
             inY < (dimY1 + dimYa + dimY2 + dimY3 + dimYd))
    {
      vtkm::Id inY_local = inY - dimY1 - dimY2 - dimY3;
      cube = 5; // cAcD
      idx = (inZ + startZ5) * dimX5 * dimY5 + (inY_local + startY5) * dimX5 + (inX + startX5);
    }
    else if (0 <= inY && inY < dimY1)
    {
      cube = 1; // ext1
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimY1 + dimYa) <= inY && inY < (dimY1 + dimYa + dimY2))
    {
      vtkm::Id inY_local = inY - dimY1 - dimYa;
      cube = 2; // ext2
      idx = inZ * dimX2 * dimY2 + inY_local * dimX2 + inX;
    }
    else if ((dimY1 + dimYa + dimY2) <= inY && inY < (dimY1 + dimYa + dimY2 + dimY3))
    {
      vtkm::Id inY_local = inY - dimY1 - dimYa - dimY2;
      cube = 3; // ext3
      idx = inZ * dimX3 * dimY3 + inY_local * dimX3 + inX;
    }
    else if ((dimY1 + dimYa + dimY2 + dimY3 + dimYd) <= inY &&
             inY < (dimY1 + dimYa + dimY2 + dimY3 + dimYd + dimY4))
    {
      vtkm::Id inY_local = inY - dimY1 - dimYa - dimY2 - dimY3 - dimYd;
      cube = 4; // ext4
      idx = inZ * dimX4 * dimY4 + inY_local * dimX4 + inX;
    }
  }

private:
  const vtkm::Id dimX1, dimY1, dimZ1, dimX2, dimY2, dimZ2;       // extension cube sizes
  const vtkm::Id dimX3, dimY3, dimZ3, dimX4, dimY4, dimZ4;       // extension cube sizes
  const vtkm::Id dimXa, dimYa, dimZa, dimXd, dimYd, dimZd;       // signal cube sizes
  const vtkm::Id dimX5, dimY5, dimZ5, startX5, startY5, startZ5; // entire cube size
};

class IndexTranslator6CubesFrontBack
{
public:
  VTKM_EXEC_CONT
  IndexTranslator6CubesFrontBack(vtkm::Id x_1,
                                 vtkm::Id y_1,
                                 vtkm::Id z_1,
                                 vtkm::Id x_2,
                                 vtkm::Id y_2,
                                 vtkm::Id z_2,
                                 vtkm::Id x_3,
                                 vtkm::Id y_3,
                                 vtkm::Id z_3,
                                 vtkm::Id x_4,
                                 vtkm::Id y_4,
                                 vtkm::Id z_4,
                                 vtkm::Id x_a,
                                 vtkm::Id y_a,
                                 vtkm::Id z_a,
                                 vtkm::Id x_d,
                                 vtkm::Id y_d,
                                 vtkm::Id z_d,
                                 vtkm::Id x_5,
                                 vtkm::Id y_5,
                                 vtkm::Id z_5,
                                 vtkm::Id start_x5,
                                 vtkm::Id start_y5,
                                 vtkm::Id start_z5)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
    , dimX4(x_4)
    , dimY4(y_4)
    , dimZ4(z_4)
    , dimXa(x_a)
    , dimYa(y_a)
    , dimZa(z_a)
    , dimXd(x_d)
    , dimYd(y_d)
    , dimZd(z_d)
    , dimX5(x_5)
    , dimY5(y_5)
    , dimZ5(z_5)
    , startX5(start_x5)
    , startY5(start_y5)
    , startZ5(start_z5)
  {
    (void)dimXd;
    (void)dimXa;
    (void)dimYd;
    (void)dimYa;
    (void)dimZ5;
  }

  VTKM_EXEC_CONT
  void Translate3Dto1D(vtkm::Id inX,
                       vtkm::Id inY,
                       vtkm::Id inZ, // 3D indices as input
                       vtkm::Id& cube,
                       vtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimZ1 <= inZ && inZ < (dimZ1 + dimZa))
    {
      vtkm::Id inZ_local = inZ - dimZ1;
      cube = 5; // cAcD
      idx = (inZ_local + startZ5) * dimX5 * dimY5 + (inY + startY5) * dimX5 + (inX + startX5);
    }
    else if ((dimZ1 + dimZa + dimZ2 + dimZ3) <= inZ &&
             inZ < (dimZ1 + dimZa + dimZ2 + dimZ3 + dimZd))
    {
      vtkm::Id inZ_local = inZ - dimZ1 - dimZ2 - dimZ3;
      cube = 5; // cAcD
      idx = (inZ_local + startZ5) * dimX5 * dimY5 + (inY + startY5) * dimX5 + (inX + startX5);
    }
    else if (0 <= inZ && inZ < dimZ1)
    {
      cube = 1; // ext1
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimZ1 + dimZa) <= inZ && inZ < (dimZ1 + dimZa + dimZ2))
    {
      vtkm::Id inZ_local = inZ - dimZ1 - dimZa;
      cube = 2; // ext2
      idx = inZ_local * dimX2 * dimY2 + inY * dimX2 + inX;
    }
    else if ((dimZ1 + dimZa + dimZ2) <= inZ && inZ < (dimZ1 + dimZa + dimZ2 + dimZ3))
    {
      vtkm::Id inZ_local = inZ - dimZ1 - dimZa - dimZ2;
      cube = 3; // ext3
      idx = inZ_local * dimX3 * dimY3 + inY * dimX3 + inX;
    }
    else if ((dimZ1 + dimZa + dimZ2 + dimZ3 + dimZd) <= inZ &&
             inZ < (dimZ1 + dimZa + dimZ2 + dimZ3 + dimZd + dimZ4))
    {
      vtkm::Id inZ_local = inZ - dimZ1 - dimZa - dimZ2 - dimZ3 - dimZd;
      cube = 4; // ext4
      idx = inZ_local * dimX4 * dimY4 + inY * dimX4 + inX;
    }
  }

private:
  const vtkm::Id dimX1, dimY1, dimZ1, dimX2, dimY2, dimZ2;       // extension cube sizes
  const vtkm::Id dimX3, dimY3, dimZ3, dimX4, dimY4, dimZ4;       // extension cube sizes
  const vtkm::Id dimXa, dimYa, dimZa, dimXd, dimYd, dimZd;       // signal cube sizes
  const vtkm::Id dimX5, dimY5, dimZ5, startX5, startY5, startZ5; // entire cube size
};

//=============================================================================

// Worklet: 3D forward transform along X (left-right)
class ForwardTransform3DLeftRight : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // left extension
                                WholeArrayIn,   // signal
                                WholeArrayIn,   // right extension
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // cA followed by cD
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);
  using InputDomain = _6;

  ForwardTransform3DLeftRight(vtkm::Id filter_len,
                              vtkm::Id approx_len,
                              bool odd_low,
                              vtkm::Id dimX1,
                              vtkm::Id dimY1,
                              vtkm::Id dimZ1,
                              vtkm::Id dimX2,
                              vtkm::Id dimY2,
                              vtkm::Id dimZ2,
                              vtkm::Id startX2,
                              vtkm::Id startY2,
                              vtkm::Id startZ2,
                              vtkm::Id pretendX2,
                              vtkm::Id pretendY2,
                              vtkm::Id pretendZ2,
                              vtkm::Id dimX3,
                              vtkm::Id dimY3,
                              vtkm::Id dimZ3)
    : filterLen(filter_len)
    , approxLen(approx_len)
    , outDimX(pretendX2)
    , outDimY(pretendY2)
    //, outDimZ(pretendZ2)
    , translator(dimX1,
                 dimY1,
                 dimZ1,
                 dimX2,
                 dimY2,
                 dimZ2,
                 startX2,
                 startY2,
                 startZ2,
                 pretendX2,
                 pretendY2,
                 pretendZ2,
                 dimX3,
                 dimY3,
                 dimZ3)
  {
    this->lstart = odd_low ? 1 : 0;
    this->hstart = 1;
  }

  VTKM_EXEC_CONT
  void Output1Dto3D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y, vtkm::Id& z) const
  {
    z = idx / (this->outDimX * this->outDimY);
    y = (idx - z * this->outDimX * this->outDimY) / this->outDimX;
    x = idx % this->outDimX;
  }
  VTKM_EXEC_CONT
  vtkm::Id Output3Dto1D(vtkm::Id x, vtkm::Id y, vtkm::Id z) const
  {
    return z * outDimX * outDimY + y * outDimX + x;
  }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1, typename InPortalType2, typename InPortalType3>
  VTKM_EXEC_CONT VAL GetVal(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            vtkm::Id inCube,
                            vtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(portal2.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(portal1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(portal3.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename FilterPortalType,
            typename OutputPortalType>
  VTKM_EXEC_CONT void operator()(const InPortalType1& inPortal1,     // left extension
                                 const InPortalType2& inPortal2,     // signal
                                 const InPortalType3& inPortal3,     // right extension
                                 const FilterPortalType& lowFilter,  // lowFilter
                                 const FilterPortalType& highFilter, // highFilter
                                 OutputPortalType& coeffOut,
                                 const vtkm::Id& workIndex) const
  {
    vtkm::Id workX, workY, workZ, output1D;
    this->Output1Dto3D(workIndex, workX, workY, workZ);
    vtkm::Id inputCube = 0, inputIdx = 0;
    using OutputValueType = typename OutputPortalType::ValueType;

    if (workX % 2 == 0) // calculate cA
    {
      vtkm::Id xl = lstart + workX;
      VAL sum = MAKEVAL(0.0);
      for (vtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(xl, workY, workZ, inputCube, inputIdx);
        sum += lowFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        xl++;
      }
      output1D = Output3Dto1D(workX / 2, workY, workZ);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
    else // calculate cD
    {
      vtkm::Id xh = hstart + workX - 1;
      VAL sum = MAKEVAL(0.0);
      for (vtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(xh, workY, workZ, inputCube, inputIdx);
        sum += highFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        xh++;
      }
      output1D = Output3Dto1D((workX - 1) / 2 + approxLen, workY, workZ);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen, approxLen;
  const vtkm::Id outDimX;
  const vtkm::Id outDimY;
  //const vtkm::Id outDimZ; Not used
  const IndexTranslator3CubesLeftRight translator;
  vtkm::Id lstart, hstart;
};

class ForwardTransform3DTopDown : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // left extension
                                WholeArrayIn,   // signal
                                WholeArrayIn,   // right extension
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // cA followed by cD
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);
  using InputDomain = _6;

  ForwardTransform3DTopDown(vtkm::Id filter_len,
                            vtkm::Id approx_len,
                            bool odd_low,
                            vtkm::Id dimX1,
                            vtkm::Id dimY1,
                            vtkm::Id dimZ1,
                            vtkm::Id dimX2,
                            vtkm::Id dimY2,
                            vtkm::Id dimZ2,
                            vtkm::Id startX2,
                            vtkm::Id startY2,
                            vtkm::Id startZ2,
                            vtkm::Id pretendX2,
                            vtkm::Id pretendY2,
                            vtkm::Id pretendZ2,
                            vtkm::Id dimX3,
                            vtkm::Id dimY3,
                            vtkm::Id dimZ3)
    : filterLen(filter_len)
    , approxLen(approx_len)
    , outDimX(pretendX2)
    , outDimY(pretendY2)
    //, outDimZ(pretendZ2)
    , translator(dimX1,
                 dimY1,
                 dimZ1,
                 dimX2,
                 dimY2,
                 dimZ2,
                 startX2,
                 startY2,
                 startZ2,
                 pretendX2,
                 pretendY2,
                 pretendZ2,
                 dimX3,
                 dimY3,
                 dimZ3)
  {
    this->lstart = odd_low ? 1 : 0;
    this->hstart = 1;
  }

  VTKM_EXEC_CONT
  void Output1Dto3D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y, vtkm::Id& z) const
  {
    z = idx / (outDimX * outDimY);
    y = (idx - z * outDimX * outDimY) / outDimX;
    x = idx % outDimX;
  }
  VTKM_EXEC_CONT
  vtkm::Id Output3Dto1D(vtkm::Id x, vtkm::Id y, vtkm::Id z) const
  {
    return z * outDimX * outDimY + y * outDimX + x;
  }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1, typename InPortalType2, typename InPortalType3>
  VTKM_EXEC_CONT VAL GetVal(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            vtkm::Id inCube,
                            vtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(portal2.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(portal1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(portal3.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename FilterPortalType,
            typename OutputPortalType>
  VTKM_EXEC_CONT void operator()(const InPortalType1& inPortal1,     // top extension
                                 const InPortalType2& inPortal2,     // signal
                                 const InPortalType3& inPortal3,     // down extension
                                 const FilterPortalType& lowFilter,  // lowFilter
                                 const FilterPortalType& highFilter, // highFilter
                                 OutputPortalType& coeffOut,
                                 const vtkm::Id& workIndex) const
  {
    vtkm::Id workX, workY, workZ, output1D;
    Output1Dto3D(workIndex, workX, workY, workZ);
    vtkm::Id inputCube = 0, inputIdx = 0;
    using OutputValueType = typename OutputPortalType::ValueType;

    if (workY % 2 == 0) // calculate cA
    {
      vtkm::Id yl = lstart + workY;
      VAL sum = MAKEVAL(0.0);
      for (vtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(workX, yl, workZ, inputCube, inputIdx);
        sum += lowFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        yl++;
      }
      output1D = Output3Dto1D(workX, workY / 2, workZ);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
    else // calculate cD
    {
      vtkm::Id yh = hstart + workY - 1;
      VAL sum = MAKEVAL(0.0);
      for (vtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(workX, yh, workZ, inputCube, inputIdx);
        sum += highFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        yh++;
      }
      output1D = Output3Dto1D(workX, (workY - 1) / 2 + approxLen, workZ);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen, approxLen;
  const vtkm::Id outDimX;
  const vtkm::Id outDimY;
  //const vtkm::Id outDimZ; Not used
  const IndexTranslator3CubesTopDown translator;
  vtkm::Id lstart, hstart;
};

class ForwardTransform3DFrontBack : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // left extension
                                WholeArrayIn,   // signal
                                WholeArrayIn,   // right extension
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // cA followed by cD
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);
  using InputDomain = _6;

  ForwardTransform3DFrontBack(vtkm::Id filter_len,
                              vtkm::Id approx_len,
                              bool odd_low,
                              vtkm::Id dimX1,
                              vtkm::Id dimY1,
                              vtkm::Id dimZ1,
                              vtkm::Id dimX2,
                              vtkm::Id dimY2,
                              vtkm::Id dimZ2,
                              vtkm::Id startX2,
                              vtkm::Id startY2,
                              vtkm::Id startZ2,
                              vtkm::Id pretendX2,
                              vtkm::Id pretendY2,
                              vtkm::Id pretendZ2,
                              vtkm::Id dimX3,
                              vtkm::Id dimY3,
                              vtkm::Id dimZ3)
    : filterLen(filter_len)
    , approxLen(approx_len)
    , outDimX(pretendX2)
    , outDimY(pretendY2)
    //, outDimZ(pretendZ2)
    , translator(dimX1,
                 dimY1,
                 dimZ1,
                 dimX2,
                 dimY2,
                 dimZ2,
                 startX2,
                 startY2,
                 startZ2,
                 pretendX2,
                 pretendY2,
                 pretendZ2,
                 dimX3,
                 dimY3,
                 dimZ3)
  {
    this->lstart = odd_low ? 1 : 0;
    this->hstart = 1;
  }

  VTKM_EXEC_CONT
  void Output1Dto3D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y, vtkm::Id& z) const
  {
    z = idx / (outDimX * outDimY);
    y = (idx - z * outDimX * outDimY) / outDimX;
    x = idx % outDimX;
  }
  VTKM_EXEC_CONT
  vtkm::Id Output3Dto1D(vtkm::Id x, vtkm::Id y, vtkm::Id z) const
  {
    return z * outDimX * outDimY + y * outDimX + x;
  }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1, typename InPortalType2, typename InPortalType3>
  VTKM_EXEC_CONT VAL GetVal(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            vtkm::Id inCube,
                            vtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(portal2.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(portal1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(portal3.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename FilterPortalType,
            typename OutputPortalType>
  VTKM_EXEC_CONT void operator()(const InPortalType1& inPortal1, // top extension
                                 const InPortalType2& inPortal2, // signal
                                 const InPortalType3& inPortal3, // down extension
                                 const FilterPortalType& lowFilter,
                                 const FilterPortalType& highFilter,
                                 OutputPortalType& coeffOut,
                                 const vtkm::Id& workIndex) const
  {
    vtkm::Id workX, workY, workZ, output1D;
    Output1Dto3D(workIndex, workX, workY, workZ);
    vtkm::Id inputCube = 0, inputIdx = 0;
    using OutputValueType = typename OutputPortalType::ValueType;

    if (workZ % 2 == 0) // calculate cA
    {
      vtkm::Id zl = lstart + workZ;
      VAL sum = MAKEVAL(0.0);
      for (vtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(workX, workY, zl, inputCube, inputIdx);
        sum += lowFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        zl++;
      }
      output1D = Output3Dto1D(workX, workY, workZ / 2);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
    else // calculate cD
    {
      vtkm::Id zh = hstart + workZ - 1;
      VAL sum = MAKEVAL(0.0);
      for (vtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(workX, workY, zh, inputCube, inputIdx);
        sum += highFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        zh++;
      }
      output1D = Output3Dto1D(workX, workY, (workZ - 1) / 2 + approxLen);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen, approxLen;
  const vtkm::Id outDimX;
  const vtkm::Id outDimY;
  //const vtkm::Id outDimZ; Not used
  const IndexTranslator3CubesFrontBack translator;
  vtkm::Id lstart, hstart;
};

//=============================================================================

//
//  ---------------------------------------------------
//  |      |          |      |      |          |      |
//  |cube1 |  cube5   |cube2 |cube3 |  cube5   |cube4 |
//  | ext1 |    cA    | ext2 | ext3 |    cD    | ext4 |
//  | (x1) |   (xa)   | (x2) | (x3) |   (xd)   | (x4) |
//  |      |          |      |      |          |      |
//  ----------------------------------------------------
// The following 3 classes perform the same functionaliry in 3 directions
//
// Worklet: perform a simple 3D inverse transform along X direction (Left-right)
class InverseTransform3DLeftRight : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, // ext1
                                WholeArrayIn, // ext2
                                WholeArrayIn, // ext3
                                WholeArrayIn, // ext4
                                WholeArrayIn, // cA+cD (signal)
                                WholeArrayIn, // lowFilter
                                WholeArrayIn, // highFilter
                                FieldOut);    // outptu coefficients
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex);
  using InputDomain = _8;

  // Constructor
  InverseTransform3DLeftRight(vtkm::Id fil_len,
                              vtkm::Id x_1,
                              vtkm::Id y_1,
                              vtkm::Id z_1, // ext1
                              vtkm::Id x_2,
                              vtkm::Id y_2,
                              vtkm::Id z_2, // ext2
                              vtkm::Id x_3,
                              vtkm::Id y_3,
                              vtkm::Id z_3, // ext3
                              vtkm::Id x_4,
                              vtkm::Id y_4,
                              vtkm::Id z_4, // ext4
                              vtkm::Id x_a,
                              vtkm::Id y_a,
                              vtkm::Id z_a, // cA
                              vtkm::Id x_d,
                              vtkm::Id y_d,
                              vtkm::Id z_d, // cD
                              vtkm::Id x_5,
                              vtkm::Id y_5,
                              vtkm::Id z_5, // signal, actual dims
                              vtkm::Id startX5,
                              vtkm::Id startY5,
                              vtkm::Id startZ5)
    : filterLen(fil_len)
    , outDimX(x_a + x_d)
    , outDimY(y_a)
    //, outDimZ(z_a)
    , cALenExtended(x_1 + x_a + x_2)
    , translator(x_1,
                 y_1,
                 z_1,
                 x_2,
                 y_2,
                 z_2,
                 x_3,
                 y_3,
                 z_3,
                 x_4,
                 y_4,
                 z_4,
                 x_a,
                 y_a,
                 z_a,
                 x_d,
                 y_d,
                 z_d,
                 x_5,
                 y_5,
                 z_5,
                 startX5,
                 startY5,
                 startZ5)
  {
  }

  VTKM_EXEC_CONT
  void Output1Dto3D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y, vtkm::Id& z) const
  {
    z = idx / (outDimX * outDimY);
    y = (idx - z * outDimX * outDimY) / outDimX;
    x = idx % outDimX;
  }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5>
  VTKM_EXEC_CONT VAL GetVal(const InPortalType1& ext1,
                            const InPortalType2& ext2,
                            const InPortalType3& ext3,
                            const InPortalType4& ext4,
                            const InPortalType5& sig5,
                            vtkm::Id inCube,
                            vtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(ext2.Get(inIdx));
    }
    else if (inCube == 4)
    {
      return MAKEVAL(ext4.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(ext1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(ext3.Get(inIdx));
    }
    else if (inCube == 5)
    {
      return MAKEVAL(sig5.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5,
            typename FilterPortalType,
            typename OutputValueType>
  VTKM_EXEC void operator()(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            const InPortalType4& portal4,
                            const InPortalType5& portal5,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputValueType& coeffOut,
                            const vtkm::Id& workIdx) const
  {
    vtkm::Id workX, workY, workZ;
    vtkm::Id k1, k2, xi;
    vtkm::Id inputCube = 0, inputIdx = 0;
    Output1Dto3D(workIdx, workX, workY, workZ);

    if (filterLen % 2 != 0) // odd filter
    {
      if (workX % 2 != 0)
      {
        k1 = filterLen - 2;
        k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;
        k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      xi = (workX + 1) / 2;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(xi, workY, workZ, inputCube, inputIdx);
        sum += lowFilter.Get(k1) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        xi++;
        k1 -= 2;
      }
      xi = workX / 2;
      while (k2 > -1)
      {
        translator.Translate3Dto1D(xi + cALenExtended, workY, workZ, inputCube, inputIdx);
        sum += highFilter.Get(k2) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        xi++;
        k2 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
    else // even filter
    {
      if ((filterLen / 2) % 2 != 0) // odd length half filter
      {
        xi = workX / 2;
        if (workX % 2 != 0)
        {
          k1 = filterLen - 1;
        }
        else
        {
          k1 = filterLen - 2;
        }
      }
      else // even length half filter
      {
        xi = (workX + 1) / 2;
        if (workX % 2 != 0)
        {
          k1 = filterLen - 2;
        }
        else
        {
          k1 = filterLen - 1;
        }
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(xi, workY, workZ, inputCube, inputIdx);
        cA = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        translator.Translate3Dto1D(xi + cALenExtended, workY, workZ, inputCube, inputIdx);
        cD = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        xi++;
        k1 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen;
  vtkm::Id outDimX;
  vtkm::Id outDimY;
  // vtkm::Id outDimZ; Not used
  vtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
  const IndexTranslator6CubesLeftRight translator;
};

class InverseTransform3DTopDown : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, // ext1
                                WholeArrayIn, // ext2
                                WholeArrayIn, // ext3
                                WholeArrayIn, // ext4
                                WholeArrayIn, // cA+cD (signal)
                                WholeArrayIn, // lowFilter
                                WholeArrayIn, // highFilter
                                FieldOut);    // outptu coefficients
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex);
  using InputDomain = _8;

  // Constructor
  InverseTransform3DTopDown(vtkm::Id fil_len,
                            vtkm::Id x_1,
                            vtkm::Id y_1,
                            vtkm::Id z_1, // ext1
                            vtkm::Id x_2,
                            vtkm::Id y_2,
                            vtkm::Id z_2, // ext2
                            vtkm::Id x_3,
                            vtkm::Id y_3,
                            vtkm::Id z_3, // ext3
                            vtkm::Id x_4,
                            vtkm::Id y_4,
                            vtkm::Id z_4, // ext4
                            vtkm::Id x_a,
                            vtkm::Id y_a,
                            vtkm::Id z_a, // cA
                            vtkm::Id x_d,
                            vtkm::Id y_d,
                            vtkm::Id z_d, // cD
                            vtkm::Id x_5,
                            vtkm::Id y_5,
                            vtkm::Id z_5, // signal, actual dims
                            vtkm::Id startX5,
                            vtkm::Id startY5,
                            vtkm::Id startZ5)
    : filterLen(fil_len)
    , outDimX(x_a)
    , outDimY(y_a + y_d)
    //, outDimZ(z_a)
    , cALenExtended(y_1 + y_a + y_2)
    , translator(x_1,
                 y_1,
                 z_1,
                 x_2,
                 y_2,
                 z_2,
                 x_3,
                 y_3,
                 z_3,
                 x_4,
                 y_4,
                 z_4,
                 x_a,
                 y_a,
                 z_a,
                 x_d,
                 y_d,
                 z_d,
                 x_5,
                 y_5,
                 z_5,
                 startX5,
                 startY5,
                 startZ5)
  {
  }

  VTKM_EXEC_CONT
  void Output1Dto3D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y, vtkm::Id& z) const
  {
    z = idx / (outDimX * outDimY);
    y = (idx - z * outDimX * outDimY) / outDimX;
    x = idx % outDimX;
  }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5>
  VTKM_EXEC_CONT VAL GetVal(const InPortalType1& ext1,
                            const InPortalType2& ext2,
                            const InPortalType3& ext3,
                            const InPortalType4& ext4,
                            const InPortalType5& sig5,
                            vtkm::Id inCube,
                            vtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(ext2.Get(inIdx));
    }
    else if (inCube == 4)
    {
      return MAKEVAL(ext4.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(ext1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(ext3.Get(inIdx));
    }
    else if (inCube == 5)
    {
      return MAKEVAL(sig5.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5,
            typename FilterPortalType,
            typename OutputValueType>
  VTKM_EXEC void operator()(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            const InPortalType4& portal4,
                            const InPortalType5& portal5,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputValueType& coeffOut,
                            const vtkm::Id& workIdx) const
  {
    vtkm::Id workX, workY, workZ;
    vtkm::Id k1, k2, yi;
    vtkm::Id inputCube = 0, inputIdx = 0;
    Output1Dto3D(workIdx, workX, workY, workZ);

    if (filterLen % 2 != 0) // odd filter
    {
      if (workY % 2 != 0)
      {
        k1 = filterLen - 2;
        k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;
        k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      yi = (workY + 1) / 2;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(workX, yi, workZ, inputCube, inputIdx);
        sum += lowFilter.Get(k1) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        yi++;
        k1 -= 2;
      }
      yi = workY / 2;
      while (k2 > -1)
      {
        translator.Translate3Dto1D(workX, yi + cALenExtended, workZ, inputCube, inputIdx);
        sum += highFilter.Get(k2) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        yi++;
        k2 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
    else // even filter
    {
      if ((filterLen / 2) % 2 != 0)
      {
        yi = workY / 2;
        if (workY % 2 != 0)
        {
          k1 = filterLen - 1;
        }
        else
        {
          k1 = filterLen - 2;
        }
      }
      else
      {
        yi = (workY + 1) / 2;
        if (workY % 2 != 0)
        {
          k1 = filterLen - 2;
        }
        else
        {
          k1 = filterLen - 1;
        }
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(workX, yi, workZ, inputCube, inputIdx);
        cA = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        translator.Translate3Dto1D(workX, yi + cALenExtended, workZ, inputCube, inputIdx);
        cD = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        yi++;
        k1 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen;
  vtkm::Id outDimX;
  vtkm::Id outDimY;
  //vtkm::Id outDimZ; Not used
  vtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
  const IndexTranslator6CubesTopDown translator;
};

class InverseTransform3DFrontBack : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, // ext1
                                WholeArrayIn, // ext2
                                WholeArrayIn, // ext3
                                WholeArrayIn, // ext4
                                WholeArrayIn, // cA+cD (signal)
                                WholeArrayIn, // lowFilter
                                WholeArrayIn, // highFilter
                                FieldOut);    // outptu coefficients
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex);
  using InputDomain = _8;

  // Constructor
  InverseTransform3DFrontBack(vtkm::Id fil_len,
                              vtkm::Id x_1,
                              vtkm::Id y_1,
                              vtkm::Id z_1, // ext1
                              vtkm::Id x_2,
                              vtkm::Id y_2,
                              vtkm::Id z_2, // ext2
                              vtkm::Id x_3,
                              vtkm::Id y_3,
                              vtkm::Id z_3, // ext3
                              vtkm::Id x_4,
                              vtkm::Id y_4,
                              vtkm::Id z_4, // ext4
                              vtkm::Id x_a,
                              vtkm::Id y_a,
                              vtkm::Id z_a, // cA
                              vtkm::Id x_d,
                              vtkm::Id y_d,
                              vtkm::Id z_d, // cD
                              vtkm::Id x_5,
                              vtkm::Id y_5,
                              vtkm::Id z_5, // signal, actual dims
                              vtkm::Id startX5,
                              vtkm::Id startY5,
                              vtkm::Id startZ5)
    : filterLen(fil_len)
    , outDimX(x_a)
    , outDimY(y_a)
    //, outDimZ(z_a + z_d)
    , cALenExtended(z_1 + z_a + z_2)
    , translator(x_1,
                 y_1,
                 z_1,
                 x_2,
                 y_2,
                 z_2,
                 x_3,
                 y_3,
                 z_3,
                 x_4,
                 y_4,
                 z_4,
                 x_a,
                 y_a,
                 z_a,
                 x_d,
                 y_d,
                 z_d,
                 x_5,
                 y_5,
                 z_5,
                 startX5,
                 startY5,
                 startZ5)
  {
    /* printf("InverseTransform3DFrontBack: \n");
    printf("  output dims: (%lld, %lld, %lld)\n", outDimX, outDimY, outDimZ ); */
  }

  VTKM_EXEC_CONT
  void Output1Dto3D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y, vtkm::Id& z) const
  {
    z = idx / (outDimX * outDimY);
    y = (idx - z * outDimX * outDimY) / outDimX;
    x = idx % outDimX;
  }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5>
  VTKM_EXEC_CONT VAL GetVal(const InPortalType1& ext1,
                            const InPortalType2& ext2,
                            const InPortalType3& ext3,
                            const InPortalType4& ext4,
                            const InPortalType5& sig5,
                            vtkm::Id inCube,
                            vtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(ext2.Get(inIdx));
    }
    else if (inCube == 4)
    {
      return MAKEVAL(ext4.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(ext1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(ext3.Get(inIdx));
    }
    else if (inCube == 5)
    {
      return MAKEVAL(sig5.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5,
            typename FilterPortalType,
            typename OutputValueType>
  VTKM_EXEC void operator()(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            const InPortalType4& portal4,
                            const InPortalType5& portal5,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputValueType& coeffOut,
                            const vtkm::Id& workIdx) const
  {
    vtkm::Id workX, workY, workZ;
    vtkm::Id k1, k2, zi;
    vtkm::Id inputCube = 0, inputIdx = 0;
    Output1Dto3D(workIdx, workX, workY, workZ);

    if (filterLen % 2 != 0) // odd filter
    {
      if (workZ % 2 != 0)
      {
        k1 = filterLen - 2;
        k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;
        k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      zi = (workZ + 1) / 2;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(workX, workY, zi, inputCube, inputIdx);
        sum += lowFilter.Get(k1) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        zi++;
        k1 -= 2;
      }
      zi = workZ / 2;
      while (k2 > -1)
      {
        translator.Translate3Dto1D(workX, workY, zi + cALenExtended, inputCube, inputIdx);
        sum += highFilter.Get(k2) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        zi++;
        k2 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
    else // even filter
    {
      if ((filterLen / 2) % 2 != 0)
      {
        zi = workZ / 2;
        if (workZ % 2 != 0)
        {
          k1 = filterLen - 1;
        }
        else
        {
          k1 = filterLen - 2;
        }
      }
      else
      {
        zi = (workZ + 1) / 2;
        if (workZ % 2 != 0)
        {
          k1 = filterLen - 2;
        }
        else
        {
          k1 = filterLen - 1;
        }
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(workX, workY, zi, inputCube, inputIdx);
        cA = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        translator.Translate3Dto1D(workX, workY, zi + cALenExtended, inputCube, inputIdx);
        cD = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        zi++;
        k1 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen;
  vtkm::Id outDimX;
  vtkm::Id outDimY;
  //vtkm::Id outDimZ; Not used
  vtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
  const IndexTranslator6CubesFrontBack translator;
};

//=============================================================================

//  ---------------------------------------------------
//  |      |          |      |      |          |      |
//  |      |          |      |      |          |      |
//  | ext1 |    cA    | ext2 | ext3 |    cD    | ext4 |
//  | (x1) |   (xa)   | (x2) | (x3) |   (xd)   | (x4) |
//  |      |          |      |      |          |      |
//  ----------------------------------------------------
//  matrix1: ext1
//  matrix2: ext2
//  matrix3: ext3
//  matrix4: ext4
//  matrix5: cA + cD
class IndexTranslator6Matrices
{
public:
  IndexTranslator6Matrices(vtkm::Id x_1,
                           vtkm::Id y_1,
                           vtkm::Id x_a,
                           vtkm::Id y_a,
                           vtkm::Id x_2,
                           vtkm::Id y_2,
                           vtkm::Id x_3,
                           vtkm::Id y_3,
                           vtkm::Id x_d,
                           vtkm::Id y_d,
                           vtkm::Id x_4,
                           vtkm::Id y_4,
                           vtkm::Id x_5,
                           vtkm::Id y_5, // actual size of matrix5
                           vtkm::Id start_x5,
                           vtkm::Id start_y5, // start indices of pretend matrix
                           bool mode)
    : x1(x_1)
    , y1(y_1)
    , xa(x_a)
    , ya(y_a)
    , x2(x_2)
    , y2(y_2)
    , x3(x_3)
    , y3(y_3)
    , xd(x_d)
    , yd(y_d)
    , x4(x_4)
    , y4(y_4)
    , x5(x_5)
    , y5(y_5)
    , startX5(start_x5)
    , startY5(start_y5)
    , modeLR(mode)
  {
    // Get pretend matrix dims
    if (modeLR)
    {
      pretendX5 = xa + xd;
      pretendY5 = y1;
    }
    else
    {
      pretendX5 = x1;
      pretendY5 = ya + yd;
    }
    (void)y5;
  }

  VTKM_EXEC_CONT
  void Translate2Dto1D(vtkm::Id inX,
                       vtkm::Id inY, // 2D indices as input
                       vtkm::Id& mat,
                       vtkm::Id& idx) const // which matrix, and idx of that matrix
  {
    if (modeLR) // left-right mode
    {
      if (0 <= inX && inX < x1)
      {
        mat = 1; // ext1
        idx = inY * x1 + inX;
      }
      else if (x1 <= inX && inX < (x1 + xa))
      {
        mat = 5; // cAcD
        idx = (inY + startY5) * x5 + (inX - x1 + startX5);
      }
      else if ((x1 + xa) <= inX && inX < (x1 + xa + x2))
      {
        mat = 2; // ext2
        idx = inY * x2 + (inX - x1 - xa);
      }
      else if ((x1 + xa + x2) <= inX && inX < (x1 + xa + x2 + x3))
      {
        mat = 3; // ext3
        idx = inY * x3 + (inX - x1 - xa - x2);
      }
      else if ((x1 + xa + x2 + x3) <= inX && inX < (x1 + xa + x2 + x3 + xd))
      {
        mat = 5; // cAcD
        idx = (inY + startY5) * x5 + (inX - x1 - x2 - x3 + startX5);
      }
      else if ((x1 + xa + x2 + x3 + xd) <= inX && inX < (x1 + xa + x2 + x3 + xd + x4))
      {
        mat = 4; // ext4
        idx = inY * x4 + (inX - x1 - xa - x2 - x3 - xd);
      }
    }
    else // top-down mode
    {
      if (0 <= inY && inY < y1)
      {
        mat = 1; // ext1
        idx = inY * x1 + inX;
      }
      else if (y1 <= inY && inY < (y1 + ya))
      {
        mat = 5; // cAcD
        idx = (inY - y1 + startY5) * x5 + inX + startX5;
      }
      else if ((y1 + ya) <= inY && inY < (y1 + ya + y2))
      {
        mat = 2; // ext2
        idx = (inY - y1 - ya) * x2 + inX;
      }
      else if ((y1 + ya + y2) <= inY && inY < (y1 + ya + y2 + y3))
      {
        mat = 3; // ext3
        idx = (inY - y1 - ya - y2) * x3 + inX;
      }
      else if ((y1 + ya + y2 + y3) <= inY && inY < (y1 + ya + y2 + y3 + yd))
      {
        mat = 5; // cAcD
        idx = (inY - y1 - y2 - y3 + startY5) * x5 + inX + startX5;
      }
      else if ((y1 + ya + y2 + y3 + yd) <= inY && inY < (y1 + ya + y2 + y3 + yd + y4))
      {
        mat = 4; // ext4
        idx = (inY - y1 - ya - y2 - y3 - yd) * x4 + inX;
      }
    }
  }

private:
  const vtkm::Id x1, y1, xa, ya, x2, y2, x3, y3, xd, yd, x4, y4;
  vtkm::Id x5, y5, startX5, startY5, pretendX5, pretendY5;
  const bool modeLR; // true = left-right mode; false = top-down mode.
};

//       ................
//       .              .
//  -----.--------------.-----
//  |    . |          | .    |
//  |    . |          | .    |
//  | ext1 |   mat2   | ext2 |
//  | (x1) |   (x2)   | (x3) |
//  |    . |          | .    |
//  -----.--------------.-----
//       ................
class IndexTranslator3Matrices
{
public:
  IndexTranslator3Matrices(vtkm::Id x_1,
                           vtkm::Id y_1,
                           vtkm::Id x_2,
                           vtkm::Id y_2, // actual dims of mat2
                           vtkm::Id startx_2,
                           vtkm::Id starty_2, // start idx of pretend
                           vtkm::Id pretendx_2,
                           vtkm::Id pretendy_2, // pretend dims
                           vtkm::Id x_3,
                           vtkm::Id y_3,
                           bool mode)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , startX2(startx_2)
    , startY2(starty_2)
    , pretendDimX2(pretendx_2)
    , pretendDimY2(pretendy_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , mode_lr(mode)
  {
    (void)dimY2;
  }

  VTKM_EXEC_CONT
  void Translate2Dto1D(vtkm::Id inX,
                       vtkm::Id inY, // 2D indices as input
                       vtkm::Id& mat,
                       vtkm::Id& idx) const // which matrix, and idx of that matrix
  {
    if (mode_lr) // left-right mode
    {
      if (0 <= inX && inX < dimX1)
      {
        mat = 1;
        idx = inY * dimX1 + inX;
      }
      else if (dimX1 <= inX && inX < (dimX1 + pretendDimX2))
      {
        mat = 2;
        idx = (inY + startY2) * dimX2 + (inX + startX2 - dimX1);
      }
      else if ((dimX1 + pretendDimX2) <= inX && inX < (dimX1 + pretendDimX2 + dimX3))
      {
        mat = 3;
        idx = inY * dimX3 + (inX - dimX1 - pretendDimX2);
      }
    }
    else // top-down mode
    {
      if (0 <= inY && inY < dimY1)
      {
        mat = 1;
        idx = inY * dimX1 + inX;
      }
      else if (dimY1 <= inY && inY < (dimY1 + pretendDimY2))
      {
        mat = 2;
        idx = (inY + startY2 - dimY1) * dimX2 + inX + startX2;
      }
      else if ((dimY1 + pretendDimY2) <= inY && inY < (dimY1 + pretendDimY2 + dimY3))
      {
        mat = 3;
        idx = (inY - dimY1 - pretendDimY2) * dimX3 + inX;
      }
    }
  }

private:
  const vtkm::Id dimX1, dimY1;
  const vtkm::Id dimX2, dimY2, startX2, startY2, pretendDimX2, pretendDimY2;
  const vtkm::Id dimX3, dimY3;
  const bool mode_lr; // true: left right mode; false: top down mode.
};

// Worklet for 2D signal extension
// This implementation operates on a specified part of a big rectangle
class ExtensionWorklet2D : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  ExtensionWorklet2D(vtkm::Id extdimX,
                     vtkm::Id extdimY,
                     vtkm::Id sigdimX,
                     vtkm::Id sigdimY,
                     vtkm::Id sigstartX,
                     vtkm::Id sigstartY,
                     vtkm::Id sigpretendX,
                     vtkm::Id sigpretendY,
                     DWTMode m,
                     ExtensionDirection dir,
                     bool pad_zero)
    : extDimX(extdimX)
    , extDimY(extdimY)
    , sigDimX(sigdimX)
    , sigDimY(sigdimY)
    , sigStartX(sigstartX)
    , sigStartY(sigstartY)
    , sigPretendDimX(sigpretendX)
    , sigPretendDimY(sigpretendY)
    , mode(m)
    , direction(dir)
    , padZero(pad_zero)
  {
    (void)sigDimY;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  void Ext1Dto2D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y) const
  {
    x = idx % extDimX;
    y = idx / extDimX;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  vtkm::Id Sig2Dto1D(vtkm::Id x, vtkm::Id y) const { return y * sigDimX + x; }

  // Index translation helper
  VTKM_EXEC_CONT
  vtkm::Id SigPretend2Dto1D(vtkm::Id x, vtkm::Id y) const
  {
    return (y + sigStartY) * sigDimX + x + sigStartX;
  }

  template <typename PortalOutType, typename PortalInType>
  VTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const vtkm::Id& workIndex) const
  {
    vtkm::Id extX, extY, sigPretendX, sigPretendY;
    sigPretendX = sigPretendY = 0;
    Ext1Dto2D(workIndex, extX, extY);
    typename PortalOutType::ValueType sym = 1.0;
    if (mode == ASYMH || mode == ASYMW)
    {
      sym = -1.0;
    }
    if (direction == LEFT)
    {
      sigPretendY = extY;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendX = extDimX - extX - 1;
      }
      else // mode == SYMW || mode == ASYMW
      {
        sigPretendX = extDimX - extX;
      }
    }
    else if (direction == TOP)
    {
      sigPretendX = extX;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendY = extDimY - extY - 1;
      }
      else // mode == SYMW || mode == ASYMW
      {
        sigPretendY = extDimY - extY;
      }
    }
    else if (direction == RIGHT)
    {
      sigPretendY = extY;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendX = sigPretendDimX - extX - 1;
      }
      else
      {
        sigPretendX = sigPretendDimX - extX - 2;
      }
      if (padZero)
      {
        sigPretendX++;
      }
    }
    else if (direction == BOTTOM)
    {
      sigPretendX = extX;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendY = sigPretendDimY - extY - 1;
      }
      else
      {
        sigPretendY = sigPretendDimY - extY - 2;
      }
      if (padZero)
      {
        sigPretendY++;
      }
    }

    if (sigPretendX == sigPretendDimX || sigPretendY == sigPretendDimY)
    {
      portalOut.Set(workIndex, 0.0);
    }
    else
    {
      portalOut.Set(workIndex, sym * portalIn.Get(SigPretend2Dto1D(sigPretendX, sigPretendY)));
    }
  }

private:
  const vtkm::Id extDimX, extDimY, sigDimX, sigDimY;
  const vtkm::Id sigStartX, sigStartY, sigPretendDimX, sigPretendDimY;
  const DWTMode mode;
  const ExtensionDirection direction;
  const bool padZero; // treat sigIn as having a column/row zeros
};

// Worklet: perform a simple 2D forward transform
class ForwardTransform2D : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // left/top extension
                                WholeArrayIn,   // sigIn
                                WholeArrayIn,   // right/bottom extension
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // cA followed by cD
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);
  using InputDomain = _6;

  // Constructor
  ForwardTransform2D(vtkm::Id filter_len,
                     vtkm::Id approx_len,
                     bool odd_low,
                     bool mode_lr,
                     vtkm::Id x1,
                     vtkm::Id y1, // dims of left/top extension
                     vtkm::Id x2,
                     vtkm::Id y2, // dims of signal
                     vtkm::Id startx2,
                     vtkm::Id starty2, // start idx of signal
                     vtkm::Id pretendx2,
                     vtkm::Id pretendy2, // pretend dims of signal
                     vtkm::Id x3,
                     vtkm::Id y3) // dims of right/bottom extension
    : filterLen(filter_len),
      approxLen(approx_len),
      outDimX(pretendx2),
      //outDimY(pretendy2),
      oddlow(odd_low),
      modeLR(mode_lr),
      translator(x1, y1, x2, y2, startx2, starty2, pretendx2, pretendy2, x3, y3, mode_lr)
  {
    this->SetStartPosition();
  }

  VTKM_EXEC_CONT
  void Output1Dto2D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y) const
  {
    x = idx % outDimX;
    y = idx / outDimX;
  }
  VTKM_EXEC_CONT
  vtkm::Id Output2Dto1D(vtkm::Id x, vtkm::Id y) const { return y * outDimX + x; }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1, typename InPortalType2, typename InPortalType3>
  VTKM_EXEC_CONT VAL GetVal(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            vtkm::Id inMatrix,
                            vtkm::Id inIdx) const
  {
    if (inMatrix == 1)
    {
      return MAKEVAL(portal1.Get(inIdx));
    }
    else if (inMatrix == 2)
    {
      return MAKEVAL(portal2.Get(inIdx));
    }
    else if (inMatrix == 3)
    {
      return MAKEVAL(portal3.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename FilterPortalType,
            typename OutputPortalType>
  VTKM_EXEC_CONT void operator()(const InPortalType1& inPortal1, // left/top extension
                                 const InPortalType2& inPortal2, // signal
                                 const InPortalType3& inPortal3, // right/bottom extension
                                 const FilterPortalType& lowFilter,
                                 const FilterPortalType& highFilter,
                                 OutputPortalType& coeffOut,
                                 const vtkm::Id& workIndex) const
  {
    vtkm::Id workX, workY, output1D;
    Output1Dto2D(workIndex, workX, workY);
    vtkm::Id inputMatrix = 0, inputIdx = 0;
    using OutputValueType = typename OutputPortalType::ValueType;

    if (modeLR)
    {
      if (workX % 2 == 0) // calculate cA
      {
        vtkm::Id xl = lstart + workX;
        VAL sum = MAKEVAL(0.0);
        for (vtkm::Id k = filterLen - 1; k > -1; k--)
        {
          translator.Translate2Dto1D(xl, workY, inputMatrix, inputIdx);
          sum += lowFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx);
          xl++;
        }
        output1D = Output2Dto1D(workX / 2, workY);
        coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
      }
      else // calculate cD
      {
        vtkm::Id xh = hstart + workX - 1;
        VAL sum = MAKEVAL(0.0);
        for (vtkm::Id k = filterLen - 1; k > -1; k--)
        {
          translator.Translate2Dto1D(xh, workY, inputMatrix, inputIdx);
          sum += highFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx);
          xh++;
        }
        output1D = Output2Dto1D((workX - 1) / 2 + approxLen, workY);
        coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
      }
    }
    else // top-down order
    {
      if (workY % 2 == 0) // calculate cA
      {
        vtkm::Id yl = lstart + workY;
        VAL sum = MAKEVAL(0.0);
        for (vtkm::Id k = filterLen - 1; k > -1; k--)
        {
          translator.Translate2Dto1D(workX, yl, inputMatrix, inputIdx);
          sum += lowFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx);
          yl++;
        }
        output1D = Output2Dto1D(workX, workY / 2);
        coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
      }
      else // calculate cD
      {
        vtkm::Id yh = hstart + workY - 1;
        VAL sum = MAKEVAL(0.0);
        for (vtkm::Id k = filterLen - 1; k > -1; k--)
        {
          translator.Translate2Dto1D(workX, yh, inputMatrix, inputIdx);
          sum += highFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx);
          yh++;
        }
        output1D = Output2Dto1D(workX, (workY - 1) / 2 + approxLen);
        coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
      }
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen, approxLen;
  const vtkm::Id outDimX;
  //const vtkm::Id outDimY; Not used
  bool oddlow;
  bool modeLR; // true = left right; false = top down.
  const IndexTranslator3Matrices translator;
  vtkm::Id lstart, hstart;

  VTKM_EXEC_CONT
  void SetStartPosition()
  {
    this->lstart = this->oddlow ? 1 : 0;
    this->hstart = 1;
  }
};

//  ---------------------------------------------------
//  |      |          |      |      |          |      |
//  |      |          |      |      |          |      |
//  | ext1 |    cA    | ext2 | ext3 |    cD    | ext4 |
//  | (x1) |   (xa)   | (x2) | (x3) |   (xd)   | (x4) |
//  |      |          |      |      |          |      |
//  ----------------------------------------------------
//  portal1: ext1
//  portal2: ext2
//  portal3: ext3
//  portal4: ext4
//  portal5: cA + cD
// Worklet: perform a simple 2D inverse transform
class InverseTransform2D : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, // ext1
                                WholeArrayIn, // ext2
                                WholeArrayIn, // ext3
                                WholeArrayIn, // ext4
                                WholeArrayIn, // cA+cD (signal)
                                WholeArrayIn, // lowFilter
                                WholeArrayIn, // highFilter
                                FieldOut);    // outptu coeffs
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex);
  using InputDomain = _8;

  // Constructor
  InverseTransform2D(vtkm::Id fil_len,
                     vtkm::Id x_1,
                     vtkm::Id y_1, // ext1
                     vtkm::Id x_a,
                     vtkm::Id y_a, // cA
                     vtkm::Id x_2,
                     vtkm::Id y_2, // ext2
                     vtkm::Id x_3,
                     vtkm::Id y_3, // ext3
                     vtkm::Id x_d,
                     vtkm::Id y_d, // cD
                     vtkm::Id x_4,
                     vtkm::Id y_4, // ext4
                     vtkm::Id x_5,
                     vtkm::Id y_5,
                     vtkm::Id startX5,
                     vtkm::Id startY5,
                     bool mode_lr)
    : filterLen(fil_len)
    , translator(x_1,
                 y_1,
                 x_a,
                 y_a,
                 x_2,
                 y_2,
                 x_3,
                 y_3,
                 x_d,
                 y_d,
                 x_4,
                 y_4,
                 x_5,
                 y_5,
                 startX5,
                 startY5,
                 mode_lr)
    , modeLR(mode_lr)
  {
    if (modeLR)
    {
      outputDimX = x_a + x_d;
      outputDimY = y_1;
      cALenExtended = x_1 + x_a + x_2;
    }
    else
    {
      outputDimX = x_1;
      outputDimY = y_a + y_d;
      cALenExtended = y_1 + y_a + y_2;
    }
  }

  VTKM_EXEC_CONT
  void Output1Dto2D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y) const
  {
    x = idx % outputDimX;
    y = idx / outputDimX;
  }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalTypecAcD>
  VTKM_EXEC_CONT VAL GetVal(const InPortalType1& ext1,
                            const InPortalType2& ext2,
                            const InPortalType3& ext3,
                            const InPortalType4& ext4,
                            const InPortalTypecAcD& cAcD,
                            vtkm::Id inMatrix,
                            vtkm::Id inIdx) const
  {
    if (inMatrix == 1)
    {
      return MAKEVAL(ext1.Get(inIdx));
    }
    else if (inMatrix == 2)
    {
      return MAKEVAL(ext2.Get(inIdx));
    }
    else if (inMatrix == 3)
    {
      return MAKEVAL(ext3.Get(inIdx));
    }
    else if (inMatrix == 4)
    {
      return MAKEVAL(ext4.Get(inIdx));
    }
    else if (inMatrix == 5)
    {
      return MAKEVAL(cAcD.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalTypecAcD,
            typename FilterPortalType,
            typename OutputValueType>
  VTKM_EXEC void operator()(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            const InPortalType4& portal4,
                            const InPortalTypecAcD& portalcAcD,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputValueType& coeffOut,
                            const vtkm::Id& workIdx) const
  {
    vtkm::Id workX, workY;
    vtkm::Id k1 = 0, k2 = 0, xi = 0, yi = 0, inputMatrix = 0, inputIdx = 0;
    Output1Dto2D(workIdx, workX, workY);

    // left-right, odd filter
    if (modeLR && (filterLen % 2 != 0))
    {
      if (workX % 2 != 0)
      {
        k1 = filterLen - 2;
        k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;
        k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      xi = (workX + 1) / 2;
      while (k1 > -1)
      {
        translator.Translate2Dto1D(xi, workY, inputMatrix, inputIdx);
        sum += lowFilter.Get(k1) *
          GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        xi++;
        k1 -= 2;
      }
      xi = workX / 2;
      while (k2 > -1)
      {
        translator.Translate2Dto1D(xi + cALenExtended, workY, inputMatrix, inputIdx);
        sum += highFilter.Get(k2) *
          GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        xi++;
        k2 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }

    // top-down, odd filter
    else if (!modeLR && (filterLen % 2 != 0))
    {
      if (workY % 2 != 0)
      {
        k1 = filterLen - 2;
        k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;
        k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      yi = (workY + 1) / 2;
      while (k1 > -1)
      {
        translator.Translate2Dto1D(workX, yi, inputMatrix, inputIdx);
        VAL cA = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        sum += lowFilter.Get(k1) * cA;
        yi++;
        k1 -= 2;
      }
      yi = workY / 2;
      while (k2 > -1)
      {
        translator.Translate2Dto1D(workX, yi + cALenExtended, inputMatrix, inputIdx);
        VAL cD = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        sum += highFilter.Get(k2) * cD;
        yi++;
        k2 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }

    // left-right, even filter
    else if (modeLR && (filterLen % 2 == 0))
    {
      if ((filterLen / 2) % 2 != 0) // odd length half filter
      {
        xi = workX / 2;
        if (workX % 2 != 0)
        {
          k1 = filterLen - 1;
        }
        else
        {
          k1 = filterLen - 2;
        }
      }
      else // even length half filter
      {
        xi = (workX + 1) / 2;
        if (workX % 2 != 0)
        {
          k1 = filterLen - 2;
        }
        else
        {
          k1 = filterLen - 1;
        }
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while (k1 > -1)
      {
        translator.Translate2Dto1D(xi, workY, inputMatrix, inputIdx);
        cA = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        translator.Translate2Dto1D(xi + cALenExtended, workY, inputMatrix, inputIdx);
        cD = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        xi++;
        k1 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }

    // top-down, even filter
    else
    {
      if ((filterLen / 2) % 2 != 0)
      {
        yi = workY / 2;
        if (workY % 2 != 0)
        {
          k1 = filterLen - 1;
        }
        else
        {
          k1 = filterLen - 2;
        }
      }
      else
      {
        yi = (workY + 1) / 2;
        if (workY % 2 != 0)
        {
          k1 = filterLen - 2;
        }
        else
        {
          k1 = filterLen - 1;
        }
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while (k1 > -1)
      {
        translator.Translate2Dto1D(workX, yi, inputMatrix, inputIdx);
        cA = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        translator.Translate2Dto1D(workX, yi + cALenExtended, inputMatrix, inputIdx);
        cD = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        yi++;
        k1 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen;
  vtkm::Id outputDimX, outputDimY;
  vtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
  const IndexTranslator6Matrices translator;
  const bool modeLR;
};

// Worklet: perform a simple 1D forward transform
class ForwardTransform : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // sigIn
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // cA followed by cD
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);
  using InputDomain = _1;

  // Constructor
  ForwardTransform(vtkm::Id filLen,
                   vtkm::Id approx_len,
                   vtkm::Id detail_len,
                   bool odd_low,
                   bool odd_high)
    : filterLen(filLen)
    , approxLen(approx_len)
    , detailLen(detail_len)
    , oddlow(odd_low)
    , oddhigh(odd_high)
  {
    this->SetStartPosition();
  }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InputPortalType, typename FilterPortalType, typename OutputPortalType>
  VTKM_EXEC void operator()(const InputPortalType& signalIn,
                            const FilterPortalType lowFilter,
                            const FilterPortalType highFilter,
                            OutputPortalType& coeffOut,
                            const vtkm::Id& workIndex) const
  {
    using OutputValueType = typename OutputPortalType::ValueType;
    if (workIndex < approxLen + detailLen)
    {
      if (workIndex % 2 == 0) // calculate cA
      {
        vtkm::Id xl = xlstart + workIndex;
        VAL sum = MAKEVAL(0.0);
        for (vtkm::Id k = filterLen - 1; k >= 0; k--)
        {
          sum += lowFilter.Get(k) * MAKEVAL(signalIn.Get(xl++));
        }
        vtkm::Id outputIdx = workIndex / 2; // put cA at the beginning
        coeffOut.Set(outputIdx, static_cast<OutputValueType>(sum));
      }
      else // calculate cD
      {
        VAL sum = MAKEVAL(0.0);
        vtkm::Id xh = xhstart + workIndex - 1;
        for (vtkm::Id k = filterLen - 1; k >= 0; k--)
        {
          sum += highFilter.Get(k) * MAKEVAL(signalIn.Get(xh++));
        }
        vtkm::Id outputIdx = approxLen + (workIndex - 1) / 2; // put cD after cA
        coeffOut.Set(outputIdx, static_cast<OutputValueType>(sum));
      }
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen, approxLen, detailLen; // filter and outcome coeff length.
  bool oddlow, oddhigh;
  vtkm::Id xlstart, xhstart;

  VTKM_EXEC_CONT
  void SetStartPosition()
  {
    this->xlstart = this->oddlow ? 1 : 0;
    this->xhstart = this->oddhigh ? 1 : 0;
  }
};

// Worklet: perform an 1D inverse transform for odd length, symmetric filters.
class InverseTransformOdd : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // Input: coeffs, cA followed by cD
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // output
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  InverseTransformOdd(vtkm::Id filLen, vtkm::Id ca_len, vtkm::Id ext_len)
    : filterLen(filLen)
    //, cALen(ca_len)
    , cALen2(ca_len * 2)
    , cALenExtended(ext_len)
  {
  }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InputPortalType, typename FilterPortalType, typename OutputPortalType>
  VTKM_EXEC void operator()(const InputPortalType& coeffs,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputPortalType& sigOut,
                            vtkm::Id workIndex) const
  {
    if (workIndex >= cALen2) // valid calculation region
    {
      return;
    }
    vtkm::Id xi1 = (workIndex + 1) / 2;                     // coeff indices
    vtkm::Id xi2 = this->cALenExtended + ((workIndex) / 2); // coeff indices
    VAL sum = 0.0;

    const bool odd = workIndex % 2 != 0;
    if (odd)
    {
      vtkm::Id k1 = this->filterLen - 2;
      vtkm::Id k2 = this->filterLen - 1;
      for (; k1 >= 0; k1 -= 2, k2 -= 2)
      {
        sum += lowFilter.Get(k1) * MAKEVAL(coeffs.Get(xi1++));
        sum += highFilter.Get(k2) * MAKEVAL(coeffs.Get(xi2++));
      }
      if (k2 >= 0)
      {
        sum += highFilter.Get(k2) * MAKEVAL(coeffs.Get(xi2++));
      }
    }
    else //even
    {
      vtkm::Id k1 = this->filterLen - 1;
      vtkm::Id k2 = this->filterLen - 2;
      for (; k2 >= 0; k1 -= 2, k2 -= 2)
      {
        sum += lowFilter.Get(k1) * MAKEVAL(coeffs.Get(xi1++));
        sum += highFilter.Get(k2) * MAKEVAL(coeffs.Get(xi2++));
      }
      if (k1 >= 0)
      {
        sum += lowFilter.Get(k1) * MAKEVAL(coeffs.Get(xi1++));
      }
    }

    sigOut.Set(workIndex, static_cast<typename OutputPortalType::ValueType>(sum));
  }

#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen; // filter length.
  //const vtkm::Id cALen;         // Number of actual cAs  (Not used)
  const vtkm::Id cALen2;        //  = cALen * 2
  const vtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
};

// Worklet: perform an 1D inverse transform for even length, symmetric filters.
class InverseTransformEven : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // Input: coeffs, cA followed by cD
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // output
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);
  using InputDomain = _1;

  // Constructor
  InverseTransformEven(vtkm::Id filtL, vtkm::Id cAL, vtkm::Id cALExt, bool m)
    : filterLen(filtL)
    //, cALen(cAL)
    , cALen2(cAL * 2)
    , cALenExtended(cALExt)
    , matlab(m)
  {
  }

// Use 64-bit float for convolution calculation
#define VAL vtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InputPortalType, typename FilterPortalType, typename OutputPortalType>
  VTKM_EXEC void operator()(const InputPortalType& coeffs,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputPortalType& sigOut,
                            const vtkm::Id& workIndex) const
  {
    if (workIndex < cALen2) // valid calculation region
    {
      vtkm::Id xi; // coeff indices
      vtkm::Id k;  // indices for low and high filter
      VAL sum = 0.0;

      if (matlab || (filterLen / 2) % 2 != 0) // odd length half filter
      {
        xi = workIndex / 2;
        if (workIndex % 2 != 0)
        {
          k = filterLen - 1;
        }
        else
        {
          k = filterLen - 2;
        }
      }
      else
      {
        xi = (workIndex + 1) / 2;
        if (workIndex % 2 != 0)
        {
          k = filterLen - 2;
        }
        else
        {
          k = filterLen - 1;
        }
      }

      while (k > -1) // k >= 0
      {
        sum += lowFilter.Get(k) * MAKEVAL(coeffs.Get(xi)) +            // cA
          highFilter.Get(k) * MAKEVAL(coeffs.Get(xi + cALenExtended)); // cD
        xi++;
        k -= 2;
      }

      sigOut.Set(workIndex, static_cast<typename OutputPortalType::ValueType>(sum));
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const vtkm::Id filterLen; // filter length.
  //const vtkm::Id cALen;         // Number of actual cAs (not used)
  const vtkm::Id cALen2;        //  = cALen * 2
  const vtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
  bool matlab;                  // followed the naming convention from VAPOR
                                // It's always false for the 1st 4 filters.
};

class ThresholdWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut); // Thresholding in-place
  using ExecutionSignature = void(_1);
  using InputDomain = _1;

  // Constructor
  ThresholdWorklet(vtkm::Float64 t)
    : threshold(t)
    , // must pass in a positive val
    neg_threshold(t * -1.0)
  {
  }

  template <typename ValueType>
  VTKM_EXEC void operator()(ValueType& coeffVal) const
  {
    if (neg_threshold < coeffVal && coeffVal < threshold)
    {
      coeffVal = 0.0;
    }
  }

private:
  vtkm::Float64 threshold;     // positive
  vtkm::Float64 neg_threshold; // negative
};

class SquaredDeviation : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);
  using InputDomain = _1;

  // Constructor
  template <typename ValueType>
  VTKM_EXEC_CONT SquaredDeviation(ValueType t)
  {
    this->mean = static_cast<vtkm::Float64>(t);
  }

  template <typename ValueType>
  VTKM_EXEC ValueType operator()(const ValueType& num) const
  {
    vtkm::Float64 num64 = static_cast<vtkm::Float64>(num);
    vtkm::Float64 diff = this->mean - num64;
    return static_cast<ValueType>(diff * diff);
  }

private:
  vtkm::Float64 mean;
};

class Differencer : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = _3(_1, _2);
  using InputDomain = _1;

  template <typename ValueType1, typename ValueType2>
  VTKM_EXEC ValueType1 operator()(const ValueType1& v1, const ValueType2& v2) const
  {
    return v1 - static_cast<ValueType1>(v2);
  }
};

class SquareWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);
  using InputDomain = _1;

  template <typename ValueType>
  VTKM_EXEC ValueType operator()(const ValueType& v) const
  {
    return (v * v);
  }
};

class CopyWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  CopyWorklet(vtkm::Id idx) { this->startIdx = idx; }

  template <typename PortalInType, typename PortalOutType>
  VTKM_EXEC void operator()(const PortalInType& portalIn,
                            PortalOutType& portalOut,
                            const vtkm::Id& workIndex) const
  {
    portalOut.Set((startIdx + workIndex), portalIn.Get(workIndex));
  }

private:
  vtkm::Id startIdx;
};

// Worklet for 1D signal extension no. 1
class LeftSYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  LeftSYMHExtentionWorklet(vtkm::Id len)
    : addLen(len)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  VTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const vtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(this->addLen - workIndex - 1));
  }

private:
  vtkm::Id addLen;
};

// Worklet for 1D signal extension no. 2
class LeftSYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  LeftSYMWExtentionWorklet(vtkm::Id len)
    : addLen(len)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  VTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const vtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(this->addLen - workIndex));
  }

private:
  vtkm::Id addLen;
};

// Worklet for 1D signal extension no. 3
class LeftASYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  LeftASYMHExtentionWorklet(vtkm::Id len)
    : addLen(len)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  VTKM_EXEC_CONT void operator()(PortalOutType& portalOut,
                                 const PortalInType& portalIn,
                                 const vtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(addLen - workIndex - 1) * (-1.0));
  }

private:
  vtkm::Id addLen;
};

// Worklet for 1D signal extension no. 4
class LeftASYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  LeftASYMWExtentionWorklet(vtkm::Id len)
    : addLen(len)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  VTKM_EXEC_CONT void operator()(PortalOutType& portalOut,
                                 const PortalInType& portalIn,
                                 const vtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(addLen - workIndex) * (-1.0));
  }

private:
  vtkm::Id addLen;
};

// Worklet for 1D signal extension no. 5
class RightSYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  RightSYMHExtentionWorklet(vtkm::Id sigInl)
    : sigInLen(sigInl)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  VTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const vtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(this->sigInLen - workIndex - 1));
  }

private:
  vtkm::Id sigInLen;
};

// Worklet for 1D signal extension no. 6
class RightSYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  RightSYMWExtentionWorklet(vtkm::Id sigInl)
    : sigInLen(sigInl)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  VTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const vtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(this->sigInLen - workIndex - 2));
  }

private:
  vtkm::Id sigInLen;
};

// Worklet for 1D signal extension no. 7
class RightASYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  RightASYMHExtentionWorklet(vtkm::Id sigInl)
    : sigInLen(sigInl)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  VTKM_EXEC_CONT void operator()(PortalOutType& portalOut,
                                 const PortalInType& portalIn,
                                 const vtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(sigInLen - workIndex - 1) * (-1.0));
  }

private:
  vtkm::Id sigInLen;
};

// Worklet for 1D signal extension no. 8
class RightASYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  VTKM_EXEC_CONT
  RightASYMWExtentionWorklet(vtkm::Id sigInl)
    : sigInLen(sigInl)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  VTKM_EXEC_CONT void operator()(PortalOutType& portalOut,
                                 const PortalInType& portalIn,
                                 const vtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(sigInLen - workIndex - 2) * (-1.0));
  }

private:
  vtkm::Id sigInLen;
};

// Assign zero to a single index
class AssignZeroWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  // Constructor
  VTKM_EXEC_CONT
  AssignZeroWorklet(vtkm::Id idx)
    : zeroIdx(idx)
  {
  }

  template <typename PortalType>
  VTKM_EXEC void operator()(PortalType& array, const vtkm::Id& workIdx) const
  {
    if (workIdx == this->zeroIdx)
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
  }

private:
  vtkm::Id zeroIdx;
};

// Assign zero to a row or a column in a 2D array.
// Change row or column is controlled by negative indices.
class AssignZero2DWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  // Constructor
  VTKM_EXEC_CONT
  AssignZero2DWorklet(vtkm::Id x, vtkm::Id y, vtkm::Id zero_x, vtkm::Id zero_y)
    : dimX(x)
    , dimY(y)
    , zeroX(zero_x)
    , zeroY(zero_y)
  {
    (void)dimY;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  void GetLogicalDim(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y) const
  {
    x = idx % dimX;
    y = idx / dimX;
  }

  template <typename PortalType>
  VTKM_EXEC void operator()(PortalType& array, const vtkm::Id& workIdx) const
  {
    vtkm::Id x, y;
    GetLogicalDim(workIdx, x, y);
    if (zeroY < 0 && x == zeroX) // assign zero to a column
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
    else if (zeroX < 0 && y == zeroY) // assign zero to a row
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
  }

private:
  vtkm::Id dimX, dimY;
  vtkm::Id zeroX, zeroY; // element at (zeroX, zeroY) will be assigned zero.
                         // each becomes a wild card if negative
};

// Assign zero to a plane (2D) in a 3D cube.
// Which plane to assign zero is controlled by negative indices.
class AssignZero3DWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  // Constructor
  VTKM_EXEC_CONT
  AssignZero3DWorklet(vtkm::Id x,
                      vtkm::Id y,
                      vtkm::Id z,
                      vtkm::Id zero_x,
                      vtkm::Id zero_y,
                      vtkm::Id zero_z)
    : dimX(x)
    , dimY(y)
    , dimZ(z)
    , zeroX(zero_x)
    , zeroY(zero_y)
    , zeroZ(zero_z)
  {
    (void)dimZ;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  void GetLogicalDim(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y, vtkm::Id& z) const
  {
    z = idx / (dimX * dimY);
    y = (idx - z * dimX * dimY) / dimX;
    x = idx % dimX;
  }

  template <typename PortalType>
  VTKM_EXEC void operator()(PortalType& array, const vtkm::Id& workIdx) const
  {
    vtkm::Id x, y, z;
    GetLogicalDim(workIdx, x, y, z);
    if (zeroZ < 0 && zeroY < 0 && x == zeroX) // plane perpendicular to X axis
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
    else if (zeroZ < 0 && zeroX < 0 && y == zeroY) // plane perpendicular to Y axis
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
    else if (zeroY < 0 && zeroX < 0 && z == zeroZ) // plane perpendicular to Z axis
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
  }

private:
  vtkm::Id dimX, dimY, dimZ;
  vtkm::Id zeroX, zeroY, zeroZ; // element at (zeroX, zeroY, zeroZ) will be assigned zero.
                                // each becomes a wild card if negative
};

// Worklet: Copies a small rectangle to become a part of a big rectangle
class RectangleCopyTo : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn,        // Input, small rectangle
                                WholeArrayOut); // Output, big rectangle
  using ExecutionSignature = void(_1, _2, WorkIndex);

  // Constructor
  VTKM_EXEC_CONT
  RectangleCopyTo(vtkm::Id inx,
                  vtkm::Id iny,
                  vtkm::Id outx,
                  vtkm::Id outy,
                  vtkm::Id xStart,
                  vtkm::Id yStart)
    : inXLen(inx)
    , inYLen(iny)
    , outXLen(outx)
    , outYLen(outy)
    , outXStart(xStart)
    , outYStart(yStart)
  {
    (void)outYLen;
    (void)inYLen;
  }

  VTKM_EXEC_CONT
  void GetLogicalDimOfInputRect(const vtkm::Id& idx, vtkm::Id& x, vtkm::Id& y) const
  {
    x = idx % inXLen;
    y = idx / inXLen;
  }

  VTKM_EXEC_CONT
  vtkm::Id Get1DIdxOfOutputRect(vtkm::Id x, vtkm::Id y) const { return y * outXLen + x; }

  template <typename ValueInType, typename PortalOutType>
  VTKM_EXEC void operator()(const ValueInType& valueIn,
                            PortalOutType& arrayOut,
                            const vtkm::Id& workIdx) const
  {
    vtkm::Id xOfIn, yOfIn;
    GetLogicalDimOfInputRect(workIdx, xOfIn, yOfIn);
    vtkm::Id outputIdx = Get1DIdxOfOutputRect(xOfIn + outXStart, yOfIn + outYStart);
    arrayOut.Set(outputIdx, valueIn);
  }

private:
  vtkm::Id inXLen, inYLen;
  vtkm::Id outXLen, outYLen;
  vtkm::Id outXStart, outYStart;
};

// Worklet: Copies a small cube to become a part of a big cube
class CubeCopyTo : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn,        // Input, small cube
                                WholeArrayOut); // Output, big cube
  using ExecutionSignature = void(_1, _2, WorkIndex);

  // Constructor
  VTKM_EXEC_CONT
  CubeCopyTo(vtkm::Id inx,
             vtkm::Id iny,
             vtkm::Id inz,
             vtkm::Id outx,
             vtkm::Id outy,
             vtkm::Id outz,
             vtkm::Id xStart,
             vtkm::Id yStart,
             vtkm::Id zStart)
    : inDimX(inx)
    , inDimY(iny)
    , inDimZ(inz)
    , outDimX(outx)
    , outDimY(outy)
    , outDimZ(outz)
    , outStartX(xStart)
    , outStartY(yStart)
    , outStartZ(zStart)
  {
    (void)outDimZ;
    (void)inDimZ;
  }

  VTKM_EXEC_CONT
  void GetLogicalDimOfInputCube(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y, vtkm::Id& z) const
  {
    z = idx / (inDimX * inDimY);
    y = (idx - z * inDimX * inDimY) / inDimX;
    x = idx % inDimX;
  }

  VTKM_EXEC_CONT
  vtkm::Id Get1DIdxOfOutputCube(vtkm::Id x, vtkm::Id y, vtkm::Id z) const
  {
    return z * outDimX * outDimY + y * outDimX + x;
  }

  template <typename ValueInType, typename PortalOutType>
  VTKM_EXEC void operator()(const ValueInType& valueIn,
                            PortalOutType& arrayOut,
                            const vtkm::Id& workIdx) const
  {
    vtkm::Id inX, inY, inZ;
    GetLogicalDimOfInputCube(workIdx, inX, inY, inZ);
    vtkm::Id outputIdx = Get1DIdxOfOutputCube(inX + outStartX, inY + outStartY, inZ + outStartZ);
    arrayOut.Set(outputIdx, valueIn);
  }

private:
  const vtkm::Id inDimX, inDimY, inDimZ;          // input small cube
  const vtkm::Id outDimX, outDimY, outDimZ;       // output big cube
  const vtkm::Id outStartX, outStartY, outStartZ; // where to put
};

} // namespace wavelets
} // namespace worlet
} // namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
