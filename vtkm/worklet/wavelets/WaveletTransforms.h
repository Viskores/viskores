//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_Wavelets_h
#define vtk_m_worklet_Wavelets_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace worklet {
namespace wavelets {

enum DWTMode {    // boundary extension modes
  SYMH, 
  SYMW,
  ASYMH, 
  ASYMW
};

/*
enum ExtensionDirection2D {  // which side of a matrix to extend
  LEFT,
  RIGHT,
  TOP,
  BOTTOM
};
*/

enum ExtensionDirection {  // which side of a cube to extend
  LEFT,       // X direction
  RIGHT,      // X direction     Y
  TOP,        // Y direction     |   Z
  BOTTOM,     // Y direction     |  /
  FRONT,      // Z direction     | /
  BACK        // Z direction     |/________ X
};


// Worklet for 3D signal extension
// It operates on a specified part of a big cube
//
class ExtensionWorklet3D : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension
                                 WholeArrayIn  < ScalarAll > ); // signal
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  ExtensionWorklet3D  ( vtkm::Id extdimX,     vtkm::Id extdimY,     vtkm::Id extdimZ,
                        vtkm::Id sigdimX,     vtkm::Id sigdimY,     vtkm::Id sigdimZ,
                        vtkm::Id sigstartX,   vtkm::Id sigstartY,   vtkm::Id sigstartZ,
                        vtkm::Id sigpretendX, vtkm::Id sigpretendY, vtkm::Id sigpretendZ,
                        DWTMode               m,  // SYMH, SYMW, etc.
                        ExtensionDirection    dir, 
                        bool                  pad_zero )
                     : 
                        extDimX( extdimX ),       extDimY( extdimY ),       extDimZ( extdimZ ),
                        sigDimX( sigdimX ),       sigDimY( sigdimY ),       sigDimZ( sigdimZ ),
                        sigStartX( sigstartX ),   sigStartY( sigstartY ),   sigStartZ( sigstartZ ),
                        sigPretendDimX( sigpretendX ), 
                        sigPretendDimY( sigpretendY ), 
                        sigPretendDimZ( sigpretendZ ), 
                        mode(m), 
                        direction( dir ), 
                        padZero( pad_zero )  
  {}

  // Index translation helper
  VTKM_EXEC_CONT
  void Ext1Dto3D ( vtkm::Id idx, vtkm::Id &x, vtkm::Id &y, vtkm::Id &z ) const
  {
    z = idx / (extDimX * extDimY);
    y = (idx - z * extDimX * extDimY) / extDimX;
    x = idx % extDimX;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  vtkm::Id Sig3Dto1D( vtkm::Id x, vtkm::Id y, vtkm::Id z) const
  {
    return z * sigDimX * sigDimY + y * sigDimX + x;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  vtkm::Id SigPretend3Dto1D( vtkm::Id x, vtkm::Id y, vtkm::Id z ) const
  {
    return (z + sigStartZ) * sigDimX * sigDimY + (y + sigStartY) * sigDimX + x + sigStartX;
  }

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    vtkm::Id    extX, 			 extY, 				extZ;
    vtkm::Id    sigPretendX, sigPretendY, sigPretendZ;
    Ext1Dto3D( workIndex, extX, extY, extZ );
    typename PortalOutType::ValueType sym = 1.0;
    if( mode == ASYMH || mode == ASYMW )
      sym = -1.0;
    if( direction == LEFT )
    {
      sigPretendY = extY;
      sigPretendZ = extZ;
      if( mode == SYMH || mode == ASYMH )
        sigPretendX = extDimX - extX - 1;
      else    // mode == SYMW || mode == ASYMW
        sigPretendX = extDimX - extX; 
    }
    else if( direction == RIGHT )
    {
      sigPretendY = extY;
      sigPretendZ = extZ;
      if( mode == SYMH || mode == ASYMH )
        sigPretendX = sigPretendDimX - extX - 1;
      else
        sigPretendX = sigPretendDimX - extX - 2;
      if( padZero )
        sigPretendX++;
    }
    else if( direction == TOP ) 
    {
      sigPretendX = extX;
      sigPretendZ = extZ;
      if( mode == SYMH || mode == ASYMH )
        sigPretendY = extDimY - extY - 1;
      else    // mode == SYMW || mode == ASYMW
        sigPretendY = extDimY - extY; 
    }
    else if( direction == BOTTOM )
    {
      sigPretendX = extX;
      sigPretendZ = extZ;
      if( mode == SYMH || mode == ASYMH )
        sigPretendY = sigPretendDimY - extY - 1;
      else
        sigPretendY = sigPretendDimY - extY - 2;
      if( padZero )
        sigPretendY++;
    }
    else if( direction == FRONT ) 
    {
      sigPretendX = extX;
      sigPretendY = extY;
      if( mode == SYMH || mode == ASYMH )
        sigPretendZ = extDimZ - extZ - 1;
      else    // mode == SYMW || mode == ASYMW
        sigPretendZ = extDimZ - extZ; 
    }
    else if( direction == BACK )
    {
      sigPretendX = extX;
      sigPretendY = extY;
      if( mode == SYMH || mode == ASYMH )
        sigPretendZ = sigPretendDimZ - extZ - 1;
      else
        sigPretendZ = sigPretendDimZ - extZ - 2;
      if( padZero )
        sigPretendZ++;
    }
		else
      vtkm::cont::ErrorControlInternal("Invalid extension mode for cubes!");

    if( sigPretendX == sigPretendDimX || 		// decides to pad a zero 
				sigPretendY == sigPretendDimY ||
				sigPretendZ == sigPretendDimZ  )
      portalOut.Set( workIndex, 0.0 );
    else
      portalOut.Set( workIndex, sym * portalIn.Get( 
										 SigPretend3Dto1D(sigPretendX, sigPretendY, sigPretendZ) ));
  }

private:
  const vtkm::Id              extDimX, extDimY, extDimZ, sigDimX, sigDimY, sigDimZ;
  const vtkm::Id              sigStartX, sigStartY, sigStartZ;  // defines a small cube to work on
  const vtkm::Id              sigPretendDimX, sigPretendDimY, sigPretendDimZ;   // small cube dims
  const DWTMode               mode;
  const ExtensionDirection    direction;
  const bool                  padZero;  // treat sigIn as having a zero at the end
};



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
class IndexTranslator3CubesLeftRight
{
public:
  IndexTranslator3CubesLeftRight	( 
						vtkm::Id x_1,             vtkm::Id y_1,             vtkm::Id z_1,
            vtkm::Id x_2,             vtkm::Id y_2,             vtkm::Id z_2,
            vtkm::Id startx_2,        vtkm::Id starty_2,        vtkm::Id startz_2,
            vtkm::Id pretendx_2,      vtkm::Id pretendy_2,      vtkm::Id pretendz_2,
            vtkm::Id x_3,             vtkm::Id y_3,             vtkm::Id z_3 )
          :  
            dimX1(x_1),               dimY1(y_1),               dimZ1(z_1),
            dimX2(x_2),               dimY2(y_2),               dimZ2(z_2),
            startX2( startx_2 ),      startY2( starty_2 ),      startZ2(startz_2),
            pretendDimX2(pretendx_2), pretendDimY2(pretendy_2), pretendDimZ2(pretendz_2),
            dimX3(x_3),               dimY3(y_3),               dimZ3(z_3)
  { (void)dimY2; }

  VTKM_EXEC_CONT
  void Translate3Dto1D( vtkm::Id  inX,  vtkm::Id  inY,  vtkm::inZ,    // 2D indices as input
                        vtkm::Id  &mat, vtkm::Id  &idx ) const // which cube, and idx of that cube
  {
    if ( 0 <= inX && inX < dimX1 )
    {
      mat = 1;
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    } 
    else if ( dimX1 <= inX && inX < (dimX1 + pretendDimX2) )
    {
      mat = 2;
      idx = (inZ + startZ2) * dimX2 * dimY2 + (inY + startY2) * dimX2 + (inX + startX2 - dimX1);
    }
    else if ( (dimX1 + pretendDimX2) <= inX && inX < (dimX1 + pretendDimX2 + dimX3) )
    {
      mat = 3;  
      idx = inZ * dimX3 * dimY3 + inY * dimX3 + (inX - dimX1 - pretendDimX2);
    }
    else
      vtkm::cont::ErrorControlInternal("Invalid index!");
  }

private:
  const vtkm::Id      dimX1, dimY1, dimZ1;		// left extension
  const vtkm::Id      dimX2, dimY2, dimZ2;		// actual signal dims
	const vtkm::Id			startX2, startY2, startZ2, pretendDimX2, pretendDimY2, pretendDimZ2;
  const vtkm::Id      dimX3, dimY3, dimZ3;		// right extension
};

// TODO: translator for top-down and front-back


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
  IndexTranslator6Matrices( vtkm::Id x_1,       vtkm::Id y_1, 
                            vtkm::Id x_a,       vtkm::Id y_a, 
                            vtkm::Id x_2,       vtkm::Id y_2, 
                            vtkm::Id x_3,       vtkm::Id y_3,
                            vtkm::Id x_d,       vtkm::Id y_d, 
                            vtkm::Id x_4,       vtkm::Id y_4,
                            vtkm::Id x_5,       vtkm::Id y_5,     // actual size of matrix5
                            vtkm::Id start_x5,  vtkm::Id start_y5,// start indices of pretend matrix
                            bool mode )
                         :  x1(x_1),            y1(y_1), 
                            xa(x_a),            ya(y_a), 
                            x2(x_2),            y2(y_2), 
                            x3(x_3),            y3(y_3), 
                            xd(x_d),            yd(y_d), 
                            x4(x_4),            y4(y_4), 
                            x5(x_5),            y5(y_5), 
                            startX5(start_x5),  startY5(start_y5), 
                            modeLR (mode)  
  {
    // Get pretend matrix dims
    if( modeLR )
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
  void Translate2Dto1D( vtkm::Id  inX,  vtkm::Id  inY,         // 2D indices as input
                        vtkm::Id  &mat, vtkm::Id  &idx ) const // which matrix, and idx of that matrix
  {
    if( modeLR )   // left-right mode
    {
      if ( 0 <= inX && inX < x1 )
      {
        mat = 1;  // ext1
        idx = inY * x1 + inX;
      } 
      else if ( x1 <= inX && inX < (x1 + xa) )
      {
        mat = 5;  // cAcD
        idx = (inY + startY5) * x5 + (inX - x1 + startX5 );
      }
      else if ( (x1 + xa) <= inX && inX < (x1 + xa + x2) )
      {
        mat = 2;  // ext2
        idx = inY * x2 + (inX - x1 - xa);
      }
      else if ( (x1 + xa + x2) <= inX && inX < (x1 + xa + x2 + x3) )
      {
        mat = 3;  // ext3
        idx = inY * x3 + (inX - x1 - xa - x2);
      }
      else if ( (x1 + xa + x2 + x3) <= inX && inX < (x1 + xa + x2 + x3 + xd) )
      {
        mat = 5;  // cAcD
        idx = (inY + startY5) * x5 + (inX - x1 - x2 - x3 + startX5 );
      }
      else if ( (x1 + xa + x2 + x3 + xd) <= inX && inX < (x1 + xa + x2 + x3 + xd + x4) )
      {
        mat = 4;  // ext4
        idx = inY * x4 + (inX - x1 - xa - x2 - x3 - xd);
      }
      else
        vtkm::cont::ErrorControlInternal("Invalid index!");
    }
    else          // top-down mode
    {
      if ( 0 <= inY && inY < y1 )
      {
        mat = 1;  // ext1
        idx = inY * x1 + inX;
      }
      else if ( y1 <= inY && inY < (y1 + ya) )
      {
        mat = 5;  // cAcD
        idx = (inY - y1 + startY5 ) * x5 + inX + startX5;
      }
      else if ( (y1 + ya) <= inY && inY < (y1 + ya + y2) )
      {
        mat = 2;  // ext2
        idx = (inY - y1 - ya) * x1 + inX;
      }
      else if ( (y1 + ya + y2) <= inY && inY < (y1 + ya + y2 + y3) )
      {
        mat = 3;  // ext3
        idx = (inY - y1 - ya - y2) * x1 + inX;
      }
      else if ( (y1 + ya + y2 + y3) <= inY && inY < (y1 + ya + y2 + y3 + yd) )
      {
        mat = 5;  // cAcD
        idx = (inY - y1 - y2 - y3 + startY5 ) * x5 + inX + startX5;
      }
      else if ( (y1 + ya + y2 + y3 + yd) <= inY && inY < (y1 + ya + y2 + y3 + yd + y4) )
      {
        mat = 4;  // ext4
        idx = (inY - y1 - ya - y2 - y3 - yd) * x1 + inX;
      }
      else
        vtkm::cont::ErrorControlInternal("Invalid index!");
    }
  }

private:
  const vtkm::Id      x1, y1, xa, ya, x2, y2, x3, y3, xd, yd, x4, y4;
        vtkm::Id      x5, y5, startX5, startY5, pretendX5, pretendY5;
  const bool          modeLR ;     // true = left-right mode; false = top-down mode.
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
  IndexTranslator3Matrices( vtkm::Id x_1,               vtkm::Id y_1, 
                            vtkm::Id x_2,               vtkm::Id y_2,       // actual dims of mat2
                            vtkm::Id startx_2,          vtkm::Id starty_2,  // start idx of pretend
                            vtkm::Id pretendx_2,        vtkm::Id pretendy_2,// pretend dims 
                            vtkm::Id x_3, 							vtkm::Id y_3, 
														bool mode )
                         :  
                            dimX1(x_1),                 dimY1(y_1), 
                            dimX2(x_2),                 dimY2(y_2),
                            startX2( startx_2 ),        startY2( starty_2 ),
                            pretendDimX2( pretendx_2 ), pretendDimY2( pretendy_2 ),
                            dimX3(x_3),                 dimY3(y_3), 
                            mode_lr(mode)  
  { (void)dimY2; }

  VTKM_EXEC_CONT
  void Translate2Dto1D( vtkm::Id  inX,  vtkm::Id  inY,         // 2D indices as input
                        vtkm::Id  &mat, vtkm::Id  &idx ) const // which matrix, and idx of that matrix
  {
    if( mode_lr )   // left-right mode
    {
      if ( 0 <= inX && inX < dimX1 )
      {
        mat = 1;
        idx = inY * dimX1 + inX;
      } 
      else if ( dimX1 <= inX && inX < (dimX1 + pretendDimX2) )
      {
        mat = 2;
        idx = (inY + startY2) * dimX2 + (inX + startX2 - dimX1);
      }
      else if ( (dimX1 + pretendDimX2) <= inX && inX < (dimX1 + pretendDimX2 + dimX3) )
      {
        mat = 3;  
        idx = inY * dimX3 + (inX - dimX1 - pretendDimX2);
      }
      else
        vtkm::cont::ErrorControlInternal("Invalid index!");
    }
    else          // top-down mode
    {
      if ( 0 <= inY && inY < dimY1 )
      {
        mat = 1;
        idx = inY * dimX1 + inX;
      }
      else if ( dimY1 <= inY && inY < (dimY1 + pretendDimY2) )
      {
        mat = 2;
        idx = (inY + startY2 - dimY1) * dimX2 + inX + startX2;
      }
      else if ( (dimY1 + pretendDimY2) <= inY && inY < (dimY1 + pretendDimY2 + dimY3) )
      {
        mat = 3;
        idx = (inY - dimY1 - pretendDimY2) * dimX3 + inX;
      }
      else
        vtkm::cont::ErrorControlInternal("Invalid index!");
    }
  }

private:
  const vtkm::Id      dimX1, dimY1;
  const vtkm::Id      dimX2, dimY2, startX2, startY2, pretendDimX2, pretendDimY2;
  const vtkm::Id      dimX3, dimY3;
  const bool          mode_lr;     // true: left right mode; false: top down mode.
};



// Worklet for 2D signal extension
// This implementation operates on a specified part of a big rectangle
class ExtensionWorklet2D : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  ExtensionWorklet2D  ( vtkm::Id extdimX,     vtkm::Id extdimY, 
                        vtkm::Id sigdimX,     vtkm::Id sigdimY, 
                        vtkm::Id sigstartX,   vtkm::Id sigstartY,
                        vtkm::Id sigpretendX, vtkm::Id sigpretendY,
                        DWTMode m, ExtensionDirection dir, bool pad_zero)
                     : 
                        extDimX( extdimX ),           extDimY( extdimY ), 
                        sigDimX( sigdimX ),           sigDimY( sigdimY ), 
                        sigStartX( sigstartX ),       sigStartY( sigstartY ), 
                        sigPretendDimX( sigpretendX ), sigPretendDimY( sigpretendY ), 
                        mode(m), direction( dir ), padZero( pad_zero )  
  { (void)sigDimY; }

  // Index translation helper
  VTKM_EXEC_CONT
  void Ext1Dto2D ( vtkm::Id idx, vtkm::Id &x, vtkm::Id &y ) const
  {
    x = idx % extDimX;
    y = idx / extDimX;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  vtkm::Id Sig2Dto1D( vtkm::Id x, vtkm::Id y ) const
  {
    return y * sigDimX + x;
  }

  // Index translation helper
  VTKM_EXEC_CONT
  vtkm::Id SigPretend2Dto1D( vtkm::Id x, vtkm::Id y ) const
  {
    return (y + sigStartY) * sigDimX + x + sigStartX;
  }

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    vtkm::Id extX, extY, sigPretendX, sigPretendY;
		sigPretendX = sigPretendY = 0;
    Ext1Dto2D( workIndex, extX, extY );
    typename PortalOutType::ValueType sym = 1.0;
    if( mode == ASYMH || mode == ASYMW )
      sym = -1.0;
    if( direction == LEFT )     
    {
      sigPretendY = extY;
      if( mode == SYMH || mode == ASYMH )
        sigPretendX = extDimX - extX - 1;
      else    // mode == SYMW || mode == ASYMW
        sigPretendX = extDimX - extX; 
    }
    else if( direction == TOP ) 
    {
      sigPretendX = extX;
      if( mode == SYMH || mode == ASYMH )
        sigPretendY = extDimY - extY - 1;
      else    // mode == SYMW || mode == ASYMW
        sigPretendY = extDimY - extY; 
    }
    else if( direction == RIGHT )
    {
      sigPretendY = extY;
      if( mode == SYMH || mode == ASYMH )
        sigPretendX = sigPretendDimX - extX - 1;
      else
        sigPretendX = sigPretendDimX - extX - 2;
      if( padZero )
        sigPretendX++;
    }
    else if( direction == BOTTOM )
    {
      sigPretendX = extX;
      if( mode == SYMH || mode == ASYMH )
        sigPretendY = sigPretendDimY - extY - 1;
      else
        sigPretendY = sigPretendDimY - extY - 2;
      if( padZero )
        sigPretendY++;
    }
		else
      vtkm::cont::ErrorControlInternal("Invalid extension mode for matrices!");

    if( sigPretendX == sigPretendDimX || sigPretendY == sigPretendDimY )
      portalOut.Set( workIndex, 0.0 );
    else
      portalOut.Set( workIndex, sym * 
                     portalIn.Get( SigPretend2Dto1D(sigPretendX, sigPretendY) ));
  }

private:
  const vtkm::Id              extDimX, extDimY, sigDimX, sigDimY;
  const vtkm::Id              sigStartX, sigStartY, sigPretendDimX, sigPretendDimY;
  const DWTMode               mode;
  const ExtensionDirection    direction;
  const bool                  padZero;  // treat sigIn as having a column/row zeros
};



// Worklet: perform a simple 2D forward transform
template< typename DeviceTag >
class ForwardTransform2D: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // left/top extension
                                WholeArrayIn<ScalarAll>,     // sigIn
                                WholeArrayIn<ScalarAll>,     // right/bottom extension
                                WholeArrayOut<ScalarAll>);   // cA followed by cD
  typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
  typedef _4   InputDomain;


  // Constructor
  VTKM_EXEC_CONT
  ForwardTransform2D  ( const vtkm::cont::ArrayHandle<vtkm::Float64> &loFilter,
                        const vtkm::cont::ArrayHandle<vtkm::Float64> &hiFilter,
                        vtkm::Id filter_len, vtkm::Id approx_len, 
                        bool odd_low, bool mode_lr,
                        vtkm::Id x1,        vtkm::Id y1,   // dims of left/top extension
                        vtkm::Id x2,        vtkm::Id y2,        // dims of signal
                        vtkm::Id startx2,   vtkm::Id starty2,   // start idx of signal
                        vtkm::Id pretendx2, vtkm::Id pretendy2, // pretend dims of signal
                        vtkm::Id x3,        vtkm::Id y3 )  // dims of right/bottom extension
                   :    
                        lowFilter(  loFilter.PrepareForInput( DeviceTag() ) ),
                        highFilter( hiFilter.PrepareForInput( DeviceTag() ) ),
                        filterLen(  filter_len ), approxLen(  approx_len ),
                        outDimX( pretendx2 ),   outDimY( pretendy2 ),
                        oddlow( odd_low ),      modeLR( mode_lr ),
                        translator( x1,         y1, 
                                    x2,         y2, 
                                    startx2,    starty2, 
                                    pretendx2,  pretendy2, 
                                    x3,         y3, 
                                    mode_lr )
  { this->SetStartPosition(); }

  VTKM_EXEC_CONT
  void Output1Dto2D( vtkm::Id idx, vtkm::Id &x, vtkm::Id &y ) const     
  {
    x = idx % outDimX;
    y = idx / outDimX;
  }
  VTKM_EXEC_CONT
  vtkm::Id Output2Dto1D( vtkm::Id x, vtkm::Id y ) const     
  {
    return y * outDimX + x;
  }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1, typename InPortalType2, typename InPortalType3 >
  VTKM_EXEC_CONT
  VAL GetVal( const InPortalType1 &portal1, const InPortalType2 &portal2,
              const InPortalType3 &portal3, vtkm::Id inMatrix, vtkm::Id inIdx ) const
  {
    if( inMatrix == 1 )
      return MAKEVAL( portal1.Get(inIdx) );
    else if( inMatrix == 2 )
      return MAKEVAL( portal2.Get(inIdx) );
    else if( inMatrix == 3 )
      return MAKEVAL( portal3.Get(inIdx) );
    else
    {
        vtkm::cont::ErrorControlInternal("Invalid matrix index!");
        return -1;
    }
  }
  
  template <typename InPortalType1, typename InPortalType2, 
            typename InPortalType3, typename OutputPortalType>
  VTKM_EXEC_CONT
  void operator()(const InPortalType1       &inPortal1, // left/top extension
                  const InPortalType2       &inPortal2, // signal
                  const InPortalType3       &inPortal3, // right/bottom extension
                     OutputPortalType       &coeffOut,
                  const vtkm::Id            &workIndex) const
  {
    vtkm::Id workX, workY, output1D;
    Output1Dto2D( workIndex, workX, workY );
    vtkm::Id inputMatrix, inputIdx;
    typedef typename OutputPortalType::ValueType OutputValueType;

    if( modeLR )
    {
      if( workX % 2 == 0 )  // calculate cA
      {
        vtkm::Id xl = lstart + workX;
        VAL sum = MAKEVAL(0.0);
        for( vtkm::Id k = filterLen - 1; k > -1; k-- )
        {
          translator.Translate2Dto1D( xl, workY, inputMatrix, inputIdx );
          sum += lowFilter.Get(k) * 
                 GetVal( inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx );
          xl++;
        }
        output1D = Output2Dto1D( workX/2, workY );
        coeffOut.Set( output1D, static_cast<OutputValueType>(sum) );
      }
      else                      // calculate cD
      {
        vtkm::Id xh = hstart + workX - 1;
        VAL sum=MAKEVAL(0.0);
        for( vtkm::Id k = filterLen - 1; k > -1; k-- )
        {
          translator.Translate2Dto1D( xh, workY, inputMatrix, inputIdx );
          sum += highFilter.Get(k) * 
                 GetVal( inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx );
          xh++;
        }
        output1D = Output2Dto1D( (workX-1)/2 + approxLen, workY );
        coeffOut.Set( output1D, static_cast<OutputValueType>(sum) );
      }
    }
    else    // top-down order 
    {
      if( workY % 2 == 0 )  // calculate cA
      {
        vtkm::Id yl = lstart + workY;
        VAL sum = MAKEVAL(0.0);
        for( vtkm::Id k = filterLen - 1; k > -1; k-- )
        {
          translator.Translate2Dto1D( workX, yl, inputMatrix, inputIdx );
          sum += lowFilter.Get(k) * 
                 GetVal( inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx );
          yl++;
        }
        output1D = Output2Dto1D( workX, workY/2 );
        coeffOut.Set( output1D, static_cast<OutputValueType>(sum) );
      }
      else                      // calculate cD
      {
        vtkm::Id yh = hstart + workY - 1;
        VAL sum=MAKEVAL(0.0);
        for( vtkm::Id k = filterLen - 1; k > -1; k-- )
        {
          translator.Translate2Dto1D( workX, yh, inputMatrix, inputIdx );
          sum += highFilter.Get(k) * 
                 GetVal( inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx );
          yh++;
        }
        output1D = Output2Dto1D( workX, (workY-1)/2 + approxLen );
        coeffOut.Set( output1D, static_cast<OutputValueType>(sum) );
      }
    }
  }
  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::ExecutionTypes<DeviceTag>::
      PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen, approxLen;
  const vtkm::Id outDimX, outDimY;
  bool  oddlow;
  bool  modeLR;             // true = left right; false = top down.
  const IndexTranslator3Matrices  translator;
  vtkm::Id lstart, hstart;
  
  VTKM_EXEC_CONT
  void SetStartPosition()
  {
    this->lstart = this->oddlow  ? 1 : 0;
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
template< typename DeviceTag >
class InverseTransform2D: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayIn< ScalarAll >, // ext1
                                 WholeArrayIn< ScalarAll >, // ext2
                                 WholeArrayIn< ScalarAll >, // ext3
                                 WholeArrayIn< ScalarAll >, // ext4
                                 WholeArrayIn< ScalarAll >, // cA+cD (signal)
                                 FieldOut<     ScalarAll> ); // outptu coeffs
  typedef void ExecutionSignature( _1, _2, _3, _4, _5, _6, WorkIndex );
  typedef _6   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  InverseTransform2D( const vtkm::cont::ArrayHandle<vtkm::Float64> &lo_fil,
                      const vtkm::cont::ArrayHandle<vtkm::Float64> &hi_fil,
                      vtkm::Id fil_len, 
                      vtkm::Id x_1,       vtkm::Id y_1,   // ext1 
                      vtkm::Id x_a,       vtkm::Id y_a,   // cA 
                      vtkm::Id x_2,       vtkm::Id y_2,   // ext2
                      vtkm::Id x_3,       vtkm::Id y_3,   // ext3
                      vtkm::Id x_d,       vtkm::Id y_d,   // cD
                      vtkm::Id x_4,       vtkm::Id y_4,   // ext4
                      vtkm::Id x_5,       vtkm::Id y_5,
                      vtkm::Id startX5,   vtkm::Id startY5,
                      bool mode_lr )
                   :  lowFilter(  lo_fil.PrepareForInput( DeviceTag() ) ),
                      highFilter( hi_fil.PrepareForInput( DeviceTag() ) ),
                      filterLen( fil_len ), 
                      translator(x_1, y_1, x_a, y_a, x_2, y_2,
                                 x_3, y_3, x_d, y_d, x_4, y_4, 
                                 x_5, y_5, startX5, startY5, mode_lr ),
                      modeLR( mode_lr )   
  {
    if( modeLR )
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
  void Output1Dto2D( vtkm::Id idx, vtkm::Id &x, vtkm::Id &y ) const
  {
    x = idx % outputDimX;
    y = idx / outputDimX;
  }
  
  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1, typename InPortalType2, typename InPortalType3,
            typename InPortalType4, typename InPortalTypecAcD >
  VTKM_EXEC_CONT
  VAL GetVal( const InPortalType1     &ext1, 
              const InPortalType2     &ext2,
              const InPortalType3     &ext3, 
              const InPortalType4     &ext4,
              const InPortalTypecAcD  &cAcD, 
              vtkm::Id inMatrix, vtkm::Id inIdx ) const
  {
    if( inMatrix == 1 )
      return MAKEVAL( ext1.Get(inIdx) );
    else if( inMatrix == 2 )
      return MAKEVAL( ext2.Get(inIdx) );
    else if( inMatrix == 3 )
      return MAKEVAL( ext3.Get(inIdx) );
    else if( inMatrix == 4 )
      return MAKEVAL( ext4.Get(inIdx) );
    else if( inMatrix == 5 )
      return MAKEVAL( cAcD.Get(inIdx) );
    else
    {
        vtkm::cont::ErrorControlInternal("Invalid matrix index!");
        return -1;
    }
  }

  template< typename InPortalType1, typename InPortalType2, typename InPortalType3,
            typename InPortalType4, typename InPortalTypecAcD,
            typename OutputValueType >
  VTKM_EXEC
  void operator() (const InPortalType1        &portal1,
                   const InPortalType2        &portal2,
                   const InPortalType3        &portal3,
                   const InPortalType4        &portal4,
                   const InPortalTypecAcD     &portalcAcD,
                         OutputValueType      &coeffOut,
                   const vtkm::Id             &workIdx ) const
  {
    vtkm::Id workX, workY;
    vtkm::Id k1, k2, xi, yi, inputMatrix, inputIdx; 
    Output1Dto2D( workIdx, workX, workY );

    // left-right, odd filter
    if( modeLR && (filterLen % 2 != 0) )
    {
      if( workX % 2 != 0 )
      {
        k1 = filterLen - 2;   k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;   k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      xi = (workX + 1) / 2;
      while( k1 > -1 )
      {
        translator.Translate2Dto1D( xi, workY, inputMatrix, inputIdx );
        sum += lowFilter.Get(k1) * GetVal( portal1, portal2, portal3, portal4, 
                                           portalcAcD, inputMatrix, inputIdx );
        xi++;   
        k1 -= 2;
      }
      xi = workX / 2;
      while( k2 > -1 )
      {
        translator.Translate2Dto1D( xi + cALenExtended, workY, inputMatrix, inputIdx );
        sum += highFilter.Get(k2) * GetVal( portal1, portal2, portal3, portal4,
                                            portalcAcD, inputMatrix, inputIdx );
        xi++;   
        k2 -= 2;
      }
      coeffOut = static_cast< OutputValueType> (sum);
    }

    // top-down, odd filter
    else if ( !modeLR && (filterLen % 2 != 0) ) 
    {
      if( workY % 2 != 0 )
      {
        k1 = filterLen - 2;   k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;   k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      yi = (workY + 1) / 2;
      while( k1 > -1 )
      {
        translator.Translate2Dto1D( workX, yi, inputMatrix, inputIdx );
        VAL cA =  GetVal( portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx );
        sum += lowFilter.Get(k1) * cA;
        yi++;
        k1 -= 2;
      }
      yi = workY / 2;
      while( k2 > -1 )
      {
        translator.Translate2Dto1D( workX, yi + cALenExtended, inputMatrix, inputIdx );
        VAL cD = GetVal( portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx );
        sum += highFilter.Get(k2) * cD;
        yi++;
        k2 -= 2;
      }
      coeffOut = static_cast< OutputValueType >(sum);
    }

    // left-right, even filter
    else if( modeLR && (filterLen % 2 == 0) )
    {
      if( (filterLen/2) % 2 != 0 )  // odd length half filter
      {
        xi = workX / 2;
        if( workX % 2 != 0 )
          k1 = filterLen - 1;
        else
          k1 = filterLen - 2;
      }
      else                          // even length half filter
      {
        xi = (workX + 1) / 2;
        if( workX % 2 != 0 )
          k1 = filterLen - 2;
        else
          k1 = filterLen - 1;
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while( k1 > -1 )
      {
        translator.Translate2Dto1D( xi, workY, inputMatrix, inputIdx );
        cA = GetVal( portal1, portal2, portal3, portal4, portalcAcD, 
                     inputMatrix, inputIdx );
        translator.Translate2Dto1D( xi + cALenExtended, workY, inputMatrix, inputIdx );
        cD = GetVal( portal1, portal2, portal3, portal4, portalcAcD, 
                     inputMatrix, inputIdx );
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        xi++;
        k1 -= 2;
      }
      coeffOut = static_cast< OutputValueType >(sum);
    }
  
    // top-down, even filter
    else
    {
      if( (filterLen/2) % 2 != 0 )
      {
        yi = workY / 2;
        if( workY % 2 != 0 )
          k1 = filterLen - 1;
        else
          k1 = filterLen - 2;
      }
      else
      {
        yi = (workY + 1) / 2;
        if( workY % 2 != 0 )
          k1 = filterLen - 2;
        else
          k1 = filterLen - 1;
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while( k1 > -1 )
      {
        translator.Translate2Dto1D( workX, yi, inputMatrix, inputIdx );
        cA = GetVal( portal1, portal2, portal3, portal4, portalcAcD, 
                     inputMatrix, inputIdx );
        translator.Translate2Dto1D( workX, yi + cALenExtended, inputMatrix, inputIdx );
        cD = GetVal( portal1, portal2, portal3, portal4, portalcAcD,
                     inputMatrix, inputIdx );
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        yi++;
        k1 -= 2;
      }
      coeffOut = static_cast< OutputValueType >(sum);
    }
  }
  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::
        ExecutionTypes<DeviceTag>::PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen;
        vtkm::Id outputDimX, outputDimY;
        vtkm::Id cALenExtended;   // Number of cA at the beginning of input, followed by cD
  const IndexTranslator6Matrices  translator;
  const bool modeLR;
};



// Worklet: perform a simple 1D forward transform
template< typename DeviceTag >
class ForwardTransform: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // sigIn
                                WholeArrayOut<ScalarAll>);   // cA followed by cD
  typedef void ExecutionSignature(_1, _2, WorkIndex);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  ForwardTransform( const vtkm::cont::ArrayHandle<vtkm::Float64> &loFilter,
                    const vtkm::cont::ArrayHandle<vtkm::Float64> &hiFilter,
                    vtkm::Id filLen, vtkm::Id approx_len, vtkm::Id detail_len,
                    bool odd_low, bool odd_high )  :
                    lowFilter(  loFilter.PrepareForInput(DeviceTag()) ), 
                    highFilter( hiFilter.PrepareForInput(DeviceTag()) ), 
                    filterLen( filLen ), 
                    approxLen( approx_len ), 
                    detailLen( detail_len ),
                    oddlow   ( odd_low ),
                    oddhigh  ( odd_high )
  { this->SetStartPosition(); }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InputPortalType, typename OutputPortalType>
  VTKM_EXEC
  void operator()(const InputPortalType       &signalIn, 
                  OutputPortalType            &coeffOut,
                  const vtkm::Id &workIndex) const
  {
    typedef typename OutputPortalType::ValueType OutputValueType;
    if( workIndex < approxLen + detailLen )
    {
      if( workIndex % 2 == 0 )  // calculate cA
      {
        vtkm::Id xl = xlstart + workIndex;
        VAL sum=MAKEVAL(0.0);
        for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
          sum += lowFilter.Get(k) * MAKEVAL( signalIn.Get(xl++) );
        vtkm::Id outputIdx = workIndex / 2; // put cA at the beginning 
        coeffOut.Set( outputIdx, static_cast<OutputValueType>(sum) );
      }
      else                      // calculate cD
      {
        VAL sum=MAKEVAL(0.0);
        vtkm::Id xh = xhstart + workIndex - 1;
        for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
          sum += highFilter.Get(k) * MAKEVAL( signalIn.Get(xh++) );
        vtkm::Id outputIdx = approxLen + (workIndex-1) / 2; // put cD after cA
        coeffOut.Set( outputIdx, static_cast<OutputValueType>(sum) );
      }
    }
  }
  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::
        ExecutionTypes<DeviceTag>::PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen, approxLen, detailLen;  // filter and outcome coeff length.
  bool oddlow, oddhigh;
  vtkm::Id xlstart, xhstart;
  
  VTKM_EXEC_CONT
  void SetStartPosition()
  {
    this->xlstart = this->oddlow  ? 1 : 0;
    this->xhstart = this->oddhigh ? 1 : 0;
  }
};



// Worklet: perform an 1D inverse transform for odd length, symmetric filters.
template< typename DeviceTag >
class InverseTransformOdd: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // Input: coeffs,
                                                             // cA followed by cD
                                WholeArrayOut<ScalarAll>);   // output
  typedef void ExecutionSignature(_1, _2, WorkIndex);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  InverseTransformOdd( const vtkm::cont::ArrayHandle<vtkm::Float64> &loFilter,
                       const vtkm::cont::ArrayHandle<vtkm::Float64> &hiFilter,
                       vtkm::Id filLen, vtkm::Id ca_len, vtkm::Id ext_len )
                    :  lowFilter(  loFilter.PrepareForInput(DeviceTag()) ),
                       highFilter( hiFilter.PrepareForInput(DeviceTag()) ),
                       filterLen( filLen ), cALen( ca_len ),
                       cALen2( ca_len * 2 ), cALenExtended( ext_len )  {}
                       
  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InputPortalType, typename OutputPortalType>
  VTKM_EXEC
  void operator()(const InputPortalType       &coeffs,
                  OutputPortalType            &sigOut,
                  const vtkm::Id &workIndex) const
  {
    if( workIndex < cALen2 )   // valid calculation region
    {
      vtkm::Id xi;         // coeff indices
      vtkm::Id k1, k2;     // indices for low and high filter
      VAL sum = 0.0;    

      if( workIndex % 2 != 0 )
      {
        k1 = this->filterLen - 2;
        k2 = this->filterLen - 1;
      }
      else
      {
        k1 = this->filterLen - 1;
        k2 = this->filterLen - 2;
      }

      xi = (workIndex+1) / 2;
      while( k1 > -1 )  // k1 >= 0
      {
        sum += lowFilter.Get(k1) * MAKEVAL( coeffs.Get(xi++) );
        k1 -= 2;
      }

      xi = workIndex / 2;
      while( k2 > -1 )  // k2 >= 0
      {
        sum += highFilter.Get(k2) * MAKEVAL( coeffs.Get( this->cALenExtended + xi++ ) );
        k2 -= 2;
      }
    
      sigOut.Set(workIndex, static_cast<typename OutputPortalType::ValueType>( sum ) );
    }

  }
  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::ExecutionTypes<DeviceTag>::
      PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen;       // filter length.
  const vtkm::Id cALen;           // Number of actual cAs 
  const vtkm::Id cALen2;          //  = cALen * 2
  const vtkm::Id cALenExtended;   // Number of cA at the beginning of input, followed by cD
};



// Worklet: perform an 1D inverse transform for even length, symmetric filters.
template< typename DeviceTag >
class InverseTransformEven: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // Input: coeffs,
                                                             // cA followed by cD
                                WholeArrayOut<ScalarAll>);   // output
  typedef void ExecutionSignature(_1, _2, WorkIndex);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  InverseTransformEven( const vtkm::cont::ArrayHandle<vtkm::Float64> &loFilter,
                        const vtkm::cont::ArrayHandle<vtkm::Float64> &hiFilter,
                        vtkm::Id filtL, vtkm::Id cAL, vtkm::Id cALExt, bool m )
                    :   lowFilter(  loFilter.PrepareForInput(DeviceTag()) ),
                        highFilter( hiFilter.PrepareForInput(DeviceTag()) ),
                        filterLen(  filtL ), cALen( cAL ), cALen2( cAL * 2 ),
                        cALenExtended( cALExt ), matlab( m )    {}

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InputPortalType, typename OutputPortalType>
  VTKM_EXEC
  void operator()(const InputPortalType       &coeffs,
                  OutputPortalType            &sigOut,
                  const vtkm::Id &workIndex) const
  {
    if( workIndex < cALen2 )   // valid calculation region
    {
      vtkm::Id xi;         // coeff indices
      vtkm::Id k;          // indices for low and high filter
      VAL sum = 0.0;    

      if( matlab || (filterLen/2) % 2 != 0 )  // odd length half filter
      {
        xi = workIndex / 2;
        if( workIndex % 2 != 0 )
          k = filterLen - 1;
        else
          k = filterLen - 2;
      }
      else
      {
        xi = (workIndex + 1) / 2;
        if( workIndex % 2 != 0 )
          k = filterLen - 2;
        else
          k = filterLen - 1;
      }

      while( k > -1 )   // k >= 0
      {
        sum += lowFilter.Get(k)  * MAKEVAL( coeffs.Get( xi ) ) +               // cA
               highFilter.Get(k) * MAKEVAL( coeffs.Get( xi + cALenExtended) ); // cD
        xi++;
        k -= 2;
      }

      sigOut.Set(workIndex, static_cast<typename OutputPortalType::ValueType>( sum ) );
    }
  }
  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::ExecutionTypes<DeviceTag>::
      PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen;       // filter length.
  const vtkm::Id cALen;           // Number of actual cAs 
  const vtkm::Id cALen2;          //  = cALen * 2
  const vtkm::Id cALenExtended;   // Number of cA at the beginning of input, followed by cD 
  bool     matlab;          // followed the naming convention from VAPOR 
                            // It's always false for the 1st 4 filters.
};    



class ThresholdWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( FieldInOut <ScalarAll> );  // Thresholding in-place
  typedef void  ExecutionSignature( _1 );
  typedef _1    InputDomain;

  // Constructor
  ThresholdWorklet( vtkm::Float64 t ) : threshold( t ),     // must pass in a positive val
                                        neg_threshold( t*-1.0 )  {}
  
  template <typename ValueType >
  VTKM_EXEC
  void operator()( ValueType    &coeffVal ) const
  {
    if( neg_threshold < coeffVal && coeffVal < threshold )
      coeffVal = 0.0;
  }

private:
  vtkm::Float64 threshold;      // positive 
  vtkm::Float64 neg_threshold;  // negative 
};    



class SquaredDeviation: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<ScalarAll>,     
                                FieldOut<ScalarAll>);        
  typedef _2   ExecutionSignature( _1 );
  typedef _1   InputDomain;

  // Constructor
  template <typename ValueType>
  VTKM_EXEC_CONT
  SquaredDeviation( ValueType t ) 
  {
    this->mean = static_cast<vtkm::Float64>(t);
  }

  template <typename ValueType>
  VTKM_EXEC
  ValueType operator()( const ValueType &num ) const
  {
    vtkm::Float64 num64 = static_cast<vtkm::Float64>( num );
    vtkm::Float64 diff = this->mean - num64;
    return static_cast<ValueType>( diff * diff );
  }

private:
  vtkm::Float64 mean;
};   



class Differencer: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<ScalarAll>,     
                                FieldIn<ScalarAll>,
                                FieldOut<ScalarAll>);        
  typedef _3   ExecutionSignature( _1, _2 );
  typedef _1   InputDomain;

  template <typename ValueType1, typename ValueType2 >
  VTKM_EXEC
  ValueType1 operator()( const ValueType1 &v1, const ValueType2 &v2 ) const
  {
    return v1 - static_cast<ValueType1>(v2);
  }
};   



class SquareWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn< ScalarAll>,     
                                FieldOut<ScalarAll>);        
  typedef _2   ExecutionSignature( _1 );
  typedef _1   InputDomain;

  template <typename ValueType>
  VTKM_EXEC
  ValueType operator()( const ValueType &v ) const
  {
    return (v * v);
  }
};    



class CopyWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayIn<  ScalarAll >,
                                 WholeArrayOut< ScalarAll > );
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  CopyWorklet( vtkm::Id idx ) 
  {
    this->startIdx = idx;
  }

  template< typename PortalInType, typename PortalOutType >
  VTKM_EXEC
  void operator()( const PortalInType     &portalIn,
                         PortalOutType    &portalOut,
                   const vtkm::Id         &workIndex) const
  {
    portalOut.Set( (startIdx + workIndex), portalIn.Get(workIndex) );
  }

private:
  vtkm::Id startIdx;
};



// Worklet for 1D signal extension no. 1
class LeftSYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  LeftSYMHExtentionWorklet( vtkm::Id len ) : addLen( len ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get(this->addLen - workIndex - 1) );
  }

private:
  vtkm::Id addLen;
};



// Worklet for 1D signal extension no. 2
class LeftSYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  LeftSYMWExtentionWorklet( vtkm::Id len ) : addLen( len ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get(this->addLen - workIndex) );
  }

private:
  vtkm::Id addLen;
};



// Worklet for 1D signal extension no. 3
class LeftASYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  LeftASYMHExtentionWorklet( vtkm::Id len ) : addLen (len) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_CONT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get( addLen - workIndex - 1) * (-1.0) );
  }

private:
  vtkm::Id addLen;
};



// Worklet for 1D signal extension no. 4
class LeftASYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  LeftASYMWExtentionWorklet( vtkm::Id len ) : addLen (len) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_CONT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get( addLen - workIndex ) * (-1.0) );
  }

private:
  vtkm::Id addLen;
};



// Worklet for 1D signal extension no. 5
class RightSYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  RightSYMHExtentionWorklet ( vtkm::Id sigInl ) : sigInLen( sigInl ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get(this->sigInLen - workIndex - 1) );
  }

private:
  vtkm::Id sigInLen;
};



// Worklet for 1D signal extension no. 6
class RightSYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  RightSYMWExtentionWorklet ( vtkm::Id sigInl ) : sigInLen( sigInl ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get(this->sigInLen - workIndex - 2) );
  }

private:
  vtkm::Id sigInLen;
};



// Worklet for 1D signal extension no. 7
class RightASYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  RightASYMHExtentionWorklet ( vtkm::Id sigInl ) : sigInLen( sigInl ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_CONT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get( sigInLen - workIndex - 1) * (-1.0) );
  }

private:
  vtkm::Id sigInLen;
};



// Worklet for 1D signal extension no. 8
class RightASYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  RightASYMWExtentionWorklet ( vtkm::Id sigInl ) : sigInLen( sigInl ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_CONT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get( sigInLen - workIndex - 2) * (-1.0) );
  }

private:
  vtkm::Id sigInLen;
};



// Assign zero to a single index 
class AssignZeroWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayInOut< ScalarAll > );
  typedef void ExecutionSignature( _1, WorkIndex );

  // Constructor
  VTKM_EXEC_CONT
  AssignZeroWorklet( vtkm::Id idx ) : zeroIdx( idx )  { }

  template< typename PortalType >
  VTKM_EXEC
  void operator()(       PortalType   &array,
                   const vtkm::Id     &workIdx ) const
  {
    if( workIdx == this->zeroIdx )
      array.Set( workIdx, static_cast<typename PortalType::ValueType>(0.0) );
  }

private:
  vtkm::Id zeroIdx;
};



// Assign zero to a row, or a column, or a single element in a 2D array.
class AssignZero2DWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayInOut< ScalarAll > );
  typedef void ExecutionSignature( _1, WorkIndex );

  // Constructor
  VTKM_EXEC_CONT
  AssignZero2DWorklet( vtkm::Id x, vtkm::Id y, vtkm::Id zero_x, vtkm::Id zero_y )
        : dimX( x ), dimY( y ), zeroX( zero_x ), zeroY( zero_y )  
  { (void)dimY; }

  // Index translation helper
  VTKM_EXEC_CONT
  void GetLogicalDim( const Id &idx, Id &x, Id &y ) const
  {
    x = idx % dimX;
    y = idx / dimX;
  }

  template< typename PortalType >
  VTKM_EXEC
  void operator()(       PortalType   &array,
                   const vtkm::Id     &workIdx ) const
  {
    vtkm::Id x, y;
    GetLogicalDim( workIdx, x, y );
    if( zeroY < 0 && x == zeroX )         // assign zero to a column
      array.Set( workIdx, static_cast<typename PortalType::ValueType>(0.0) );
    else if( zeroX < 0 && y == zeroY )    // assign zero to a row
      array.Set( workIdx, static_cast<typename PortalType::ValueType>(0.0) );
    else if( x == zeroX && y == zeroY )   // assign zero to an element
      array.Set( workIdx, static_cast<typename PortalType::ValueType>(0.0) );
  }

private:
  vtkm::Id dimX, dimY;
  vtkm::Id zeroX, zeroY;  // element at (zeroX, zeroY) will be assigned zero.
                          // each becomes a wild card if negative
};



// Worklet: Copys a small rectangle to part of a big rectangle
// WARNING: this worklet only supports basic ArrayHandle types.
class RectangleCopyTo : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( FieldIn<       ScalarAll >,    // Input, small rectangle
                                 WholeArrayOut< ScalarAll > );  // Output, big rectangle
  typedef void ExecutionSignature( _1, _2, WorkIndex );

  // Constructor
  VTKM_EXEC_CONT
  RectangleCopyTo( vtkm::Id inx,       vtkm::Id iny, 
                   vtkm::Id outx,      vtkm::Id outy,
                   vtkm::Id xStart,    vtkm::Id yStart )
  {
    this->inXLen    = inx;      this->inYLen    = iny;
    this->outXLen   = outx;     this->outYLen   = outy;
    this->outXStart = xStart;   this->outYStart = yStart;
  }

  VTKM_EXEC_CONT
  void GetLogicalDimOfInputRect( const vtkm::Id    &idx,    
                                       vtkm::Id    &x,      
                                       vtkm::Id    &y ) const     
  {
    x = idx % inXLen;
    y = idx / inXLen;
  }

  VTKM_EXEC_CONT
  vtkm::Id Get1DIdxOfOutputRect( vtkm::Id    x,      
                                 vtkm::Id    y ) const     
  {
    return y * outXLen + x;
  }

  template< typename ValueInType, typename PortalOutType >
  VTKM_EXEC
  void operator()( const ValueInType    &valueIn,
                         PortalOutType  &arrayOut,
                   const vtkm::Id       &workIdx ) const
  {
    vtkm::Id xOfIn, yOfIn;
    GetLogicalDimOfInputRect( workIdx, xOfIn, yOfIn );
    vtkm::Id outputIdx = Get1DIdxOfOutputRect( xOfIn+outXStart, yOfIn+outYStart );
    arrayOut.Set( outputIdx, valueIn );
  }

private:
  vtkm::Id inXLen,    inYLen;
  vtkm::Id outXLen,   outYLen;
  vtkm::Id outXStart, outYStart;
};



// Worklet: Copys a part of a big rectangle to a small rectangle
// WARNING: this worklet only supports basic ArrayHandle types.
class RectangleCopyFrom : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( FieldInOut<   ScalarAll >,    // small rectangle to be filled
                                 WholeArrayIn< ScalarAll > );  // big rectangle to read from
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  RectangleCopyFrom( vtkm::Id smallx,    vtkm::Id smally, 
                     vtkm::Id bigx,      vtkm::Id bigy,
                     vtkm::Id xStart,    vtkm::Id yStart )
  {
    this->smallXLen = smallx;   this->smallYLen = smally;
    this->bigXLen   = bigx;     this->bigYLen   = bigy;
    this->bigXStart = xStart;   this->bigYStart = yStart;
  }

  VTKM_EXEC_CONT
  void GetLogicalDimOfSmallRect( const vtkm::Id    &idx,    
                                       vtkm::Id    &x,      
                                       vtkm::Id    &y ) const     
  {
    x = idx % smallXLen;
    y = idx / smallXLen;
  }

  VTKM_EXEC_CONT
  vtkm::Id Get1DIdxOfBigRect( vtkm::Id    x,      
                              vtkm::Id    y ) const     
  {
    return y * bigXLen + x;
  }

  template< typename ValueType, typename PortalType >
  VTKM_EXEC
  void operator()(       ValueType      &value,        
                   const PortalType     &array,
                   const vtkm::Id       &workIdx ) const
  {
    vtkm::Id xOfValue, yOfValue;
    GetLogicalDimOfSmallRect( workIdx, xOfValue, yOfValue );
    vtkm::Id bigRectIdx = Get1DIdxOfBigRect( xOfValue+bigXStart, yOfValue+bigYStart );
    value = static_cast<ValueType>( array.Get( bigRectIdx ) );
  }

private:
  vtkm::Id smallXLen,    smallYLen;
  vtkm::Id bigXLen,      bigYLen;
  vtkm::Id bigXStart,    bigYStart;
};



}     // namespace wavelets
}     // namespace worlet
}     // namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
