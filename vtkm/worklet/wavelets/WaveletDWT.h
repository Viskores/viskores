//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_wavelets_waveletdwt_h
#define vtk_m_worklet_wavelets_waveletdwt_h

#include <vtkm/worklet/wavelets/WaveletBase.h>

#include <vtkm/worklet/wavelets/WaveletTransforms.h>

#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vtkm/Math.h>
#include <vtkm/cont/Timer.h>

namespace vtkm
{
namespace worklet
{
namespace wavelets
{

class WaveletDWT : public WaveletBase
{
public:
  // Constructor
  WaveletDWT(WaveletName name)
    : WaveletBase(name)
  {
  }

  // Function: extend a cube in X direction
  template <typename SigInArrayType, typename ExtensionArrayType, typename DeviceTag>
  vtkm::Id Extend3DLeftRight(const SigInArrayType& sigIn, // input
                             vtkm::Id sigDimX,
                             vtkm::Id sigDimY,
                             vtkm::Id sigDimZ,
                             vtkm::Id sigStartX,
                             vtkm::Id sigStartY,
                             vtkm::Id sigStartZ,
                             vtkm::Id sigPretendDimX,
                             vtkm::Id sigPretendDimY,
                             vtkm::Id sigPretendDimZ,
                             ExtensionArrayType& ext1, // output
                             ExtensionArrayType& ext2, // output
                             vtkm::Id addLen,
                             vtkm::worklet::wavelets::DWTMode ext1Method,
                             vtkm::worklet::wavelets::DWTMode ext2Method,
                             bool pretendSigPaddedZero,
                             bool padZeroAtExt2,
                             DeviceTag)
  {
    // pretendSigPaddedZero and padZeroAtExt2 cannot happen at the same time
    VTKM_ASSERT(!pretendSigPaddedZero || !padZeroAtExt2);

    if (addLen == 0) // Haar kernel
    {
      ext1.PrepareForOutput(0, DeviceTag());     // No extension on the left side
      if (pretendSigPaddedZero || padZeroAtExt2) // plane of size 1*dimY*dimZ
      {
        ext2.PrepareForOutput(sigPretendDimY * sigPretendDimZ, DeviceTag());
        WaveletBase::DeviceAssignZero3DPlaneX(
          ext2, 1, sigPretendDimY, sigPretendDimZ, 0, DeviceTag());
      }
      else
      {
        ext2.PrepareForOutput(0, DeviceTag()); // No extension on the right side
      }
      return 0;
    }

    using ValueType = typename SigInArrayType::ValueType;
    using ExtendArrayType = vtkm::cont::ArrayHandle<ValueType>;
    using ExtensionWorklet = vtkm::worklet::wavelets::ExtensionWorklet3D;
    using DispatcherType = typename vtkm::worklet::DispatcherMapField<ExtensionWorklet, DeviceTag>;
    vtkm::Id extDimX, extDimY, extDimZ;
    vtkm::worklet::wavelets::ExtensionDirection dir;

    { // First work on left extension
      dir = LEFT;
      extDimX = addLen;
      extDimY = sigPretendDimY;
      extDimZ = sigPretendDimZ;

      ext1.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext1Method,
                               dir,
                               false); // not treating input signal as having zeros
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext1, sigIn);
    }

    // Then work on right extension
    dir = RIGHT;
    extDimY = sigPretendDimY;
    extDimZ = sigPretendDimZ;
    if (!pretendSigPaddedZero && !padZeroAtExt2)
    {
      extDimX = addLen;
      ext2.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else if (!pretendSigPaddedZero && padZeroAtExt2)
    { // This case is not exactly padding a zero at the end of Ext2.
      // Rather, it is to increase extension length by one and fill it
      //         to be whatever mirrored.
      extDimX = addLen + 1;
      ext2.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else // pretendSigPaddedZero
    {
      ExtendArrayType ext2Temp;
      extDimX = addLen;
      ext2Temp.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               true); // pretend sig is padded a zero
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2Temp, sigIn);

      // Give ext2 one layer thicker to hold the pretend zeros from signal.
      ext2.PrepareForOutput((extDimX + 1) * extDimY * extDimZ, DeviceTag());
      WaveletBase::DeviceCubeCopyTo(ext2Temp,
                                    extDimX,
                                    extDimY,
                                    extDimZ,
                                    ext2,
                                    extDimX + 1,
                                    extDimY,
                                    extDimZ,
                                    1,
                                    0,
                                    0,
                                    DeviceTag());
      WaveletBase::DeviceAssignZero3DPlaneX(ext2, extDimX + 1, extDimY, extDimZ, 0, DeviceTag());
    }
    return 0;
  }

  // Function: extend a cube in Y direction
  template <typename SigInArrayType, typename ExtensionArrayType, typename DeviceTag>
  vtkm::Id Extend3DTopDown(const SigInArrayType& sigIn, // input
                           vtkm::Id sigDimX,
                           vtkm::Id sigDimY,
                           vtkm::Id sigDimZ,
                           vtkm::Id sigStartX,
                           vtkm::Id sigStartY,
                           vtkm::Id sigStartZ,
                           vtkm::Id sigPretendDimX,
                           vtkm::Id sigPretendDimY,
                           vtkm::Id sigPretendDimZ,
                           ExtensionArrayType& ext1, // output
                           ExtensionArrayType& ext2, // output
                           vtkm::Id addLen,
                           vtkm::worklet::wavelets::DWTMode ext1Method,
                           vtkm::worklet::wavelets::DWTMode ext2Method,
                           bool pretendSigPaddedZero,
                           bool padZeroAtExt2,
                           DeviceTag)
  {
    // pretendSigPaddedZero and padZeroAtExt2 cannot happen at the same time
    VTKM_ASSERT(!pretendSigPaddedZero || !padZeroAtExt2);

    if (addLen == 0) // Haar kernel
    {
      ext1.PrepareForOutput(0, DeviceTag());     // No extension on the top side
      if (pretendSigPaddedZero || padZeroAtExt2) // plane of size dimX*dimZ
      {
        ext2.PrepareForOutput(sigPretendDimX * 1 * sigPretendDimZ, DeviceTag());
        WaveletBase::DeviceAssignZero3DPlaneY(
          ext2, sigPretendDimX, 1, sigPretendDimZ, 0, DeviceTag());
      }
      else
      {
        ext2.PrepareForOutput(0, DeviceTag()); // No extension on the right side
      }
      return 0;
    }

    using ValueType = typename SigInArrayType::ValueType;
    using ExtendArrayType = vtkm::cont::ArrayHandle<ValueType>;
    using ExtensionWorklet = vtkm::worklet::wavelets::ExtensionWorklet3D;
    using DispatcherType = typename vtkm::worklet::DispatcherMapField<ExtensionWorklet, DeviceTag>;
    vtkm::Id extDimX, extDimY, extDimZ;
    vtkm::worklet::wavelets::ExtensionDirection dir;

    { // First work on top extension
      dir = TOP;
      extDimX = sigPretendDimX;
      extDimY = addLen;
      extDimZ = sigPretendDimZ;

      ext1.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext1Method,
                               dir,
                               false); // not treating input signal as having zeros
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext1, sigIn);
    }

    // Then work on bottom extension
    dir = BOTTOM;
    extDimX = sigPretendDimX;
    extDimZ = sigPretendDimZ;
    if (!pretendSigPaddedZero && !padZeroAtExt2)
    {
      extDimY = addLen;
      ext2.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else if (!pretendSigPaddedZero && padZeroAtExt2)
    { // This case is not exactly padding a zero at the end of Ext2.
      // Rather, it is to increase extension length by one and fill it
      //         to be whatever mirrored.
      extDimY = addLen + 1;
      ext2.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else // pretendSigPaddedZero
    {
      ExtendArrayType ext2Temp;
      extDimY = addLen;
      ext2Temp.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               true); // pretend sig is padded a zero
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2Temp, sigIn);

      // Give ext2 one layer thicker to hold the pretend zeros from signal.
      ext2.PrepareForOutput(extDimX * (extDimY + 1) * extDimZ, DeviceTag());
      WaveletBase::DeviceCubeCopyTo(ext2Temp,
                                    extDimX,
                                    extDimY,
                                    extDimZ,
                                    ext2,
                                    extDimX,
                                    extDimY + 1,
                                    extDimZ,
                                    0,
                                    1,
                                    0,
                                    DeviceTag());
      WaveletBase::DeviceAssignZero3DPlaneY(ext2, extDimX, extDimY + 1, extDimZ, 0, DeviceTag());
    }
    return 0;
  }

  // Function: extend a cube in Z direction
  template <typename SigInArrayType, typename ExtensionArrayType, typename DeviceTag>
  vtkm::Id Extend3DFrontBack(const SigInArrayType& sigIn, // input
                             vtkm::Id sigDimX,
                             vtkm::Id sigDimY,
                             vtkm::Id sigDimZ,
                             vtkm::Id sigStartX,
                             vtkm::Id sigStartY,
                             vtkm::Id sigStartZ,
                             vtkm::Id sigPretendDimX,
                             vtkm::Id sigPretendDimY,
                             vtkm::Id sigPretendDimZ,
                             ExtensionArrayType& ext1, // output
                             ExtensionArrayType& ext2, // output
                             vtkm::Id addLen,
                             vtkm::worklet::wavelets::DWTMode ext1Method,
                             vtkm::worklet::wavelets::DWTMode ext2Method,
                             bool pretendSigPaddedZero,
                             bool padZeroAtExt2,
                             DeviceTag)
  {
    // pretendSigPaddedZero and padZeroAtExt2 cannot happen at the same time
    VTKM_ASSERT(!pretendSigPaddedZero || !padZeroAtExt2);

    if (addLen == 0) // Haar kernel
    {
      ext1.PrepareForOutput(0, DeviceTag());     // No extension on the front side
      if (pretendSigPaddedZero || padZeroAtExt2) // plane of size dimX * dimY
      {
        ext2.PrepareForOutput(sigPretendDimX * sigPretendDimY * 1, DeviceTag());
        WaveletBase::DeviceAssignZero3DPlaneZ(
          ext2, sigPretendDimX, sigPretendDimY, 1, 0, DeviceTag());
      }
      else
      {
        ext2.PrepareForOutput(0, DeviceTag()); // No extension on the right side
      }
      return 0;
    }

    using ValueType = typename SigInArrayType::ValueType;
    using ExtendArrayType = vtkm::cont::ArrayHandle<ValueType>;
    using ExtensionWorklet = vtkm::worklet::wavelets::ExtensionWorklet3D;
    using DispatcherType = typename vtkm::worklet::DispatcherMapField<ExtensionWorklet, DeviceTag>;
    vtkm::Id extDimX, extDimY, extDimZ;
    vtkm::worklet::wavelets::ExtensionDirection dir;

    { // First work on front extension
      dir = FRONT;
      extDimX = sigPretendDimX;
      extDimY = sigPretendDimY;
      extDimZ = addLen;

      ext1.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext1Method,
                               dir,
                               false); // not treating input signal as having zeros
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext1, sigIn);
    }

    // Then work on back extension
    dir = BACK;
    extDimX = sigPretendDimX;
    extDimY = sigPretendDimY;
    if (!pretendSigPaddedZero && !padZeroAtExt2)
    {
      extDimZ = addLen;
      ext2.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else if (!pretendSigPaddedZero && padZeroAtExt2)
    { // This case is not exactly padding a zero at the end of Ext2.
      // Rather, it is to increase extension length by one and fill it
      //         to be whatever mirrored.
      extDimZ = addLen + 1;
      ext2.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else // pretendSigPaddedZero
    {
      ExtendArrayType ext2Temp;
      extDimZ = addLen;
      ext2Temp.PrepareForOutput(extDimX * extDimY * extDimZ, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               true); // pretend sig is padded a zero
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2Temp, sigIn);

      // Give ext2 one layer thicker to hold the pretend zeros from signal.
      ext2.PrepareForOutput(extDimX * extDimY * (extDimZ + 1), DeviceTag());
      WaveletBase::DeviceCubeCopyTo(ext2Temp,
                                    extDimX,
                                    extDimY,
                                    extDimZ,
                                    ext2,
                                    extDimX,
                                    extDimY,
                                    extDimZ + 1,
                                    0,
                                    0,
                                    1,
                                    DeviceTag());
      WaveletBase::DeviceAssignZero3DPlaneZ(ext2, extDimX, extDimY, extDimZ + 1, 0, DeviceTag());
    }
    return 0;
  }

  //                  L[3]        L[15]
  //               -----------------------
  //              /          /          /|
  //        L[5] /          /          / |
  //            /  LLH     /  HLH     /  |
  //           /          /          /   | L[16]
  //          -----------------------    |
  //         /          /          /|    |
  //   L[2] /          /          / |   /|
  //       /          /          /  |  / |
  //      /___L[0]___/___L[12]__/   | /  | L[22]
  //      |          |          |   |/   |
  // L[1] |          |          |   /HHH /
  //      |   LLL    |   HLL    |  /|   /
  //      |          |          | / |  / L[23]
  //      |---------------------|/  | /
  //      |          |          |   |/
  //      |          |          |   /
  // L[7] |   LHL    |   HHL    |  /
  //      |          |          | / L[20]
  //      |__________|__________|/
  //          L[6]       L[18]
  //
  // Performs one level of 3D discrete wavelet transform on a small cube of input array
  // The output has the same size as the small cube
  template <typename ArrayInType, typename ArrayOutType, typename DeviceTag>
  vtkm::Float64 DWT3D(ArrayInType& sigIn,
                      vtkm::Id sigDimX,
                      vtkm::Id sigDimY,
                      vtkm::Id sigDimZ,
                      vtkm::Id sigStartX,
                      vtkm::Id sigStartY,
                      vtkm::Id sigStartZ,
                      vtkm::Id sigPretendDimX,
                      vtkm::Id sigPretendDimY,
                      vtkm::Id sigPretendDimZ,
                      ArrayOutType& coeffOut,
                      bool discardSigIn, // discard sigIn on devices
                      DeviceTag)
  {
    std::vector<vtkm::Id> L(27, 0);

    // LLL
    L[0] = WaveletBase::GetApproxLength(sigPretendDimX);
    L[1] = WaveletBase::GetApproxLength(sigPretendDimY);
    L[2] = WaveletBase::GetApproxLength(sigPretendDimZ);
    // LLH
    L[3] = L[0];
    L[4] = L[1];
    L[5] = WaveletBase::GetDetailLength(sigPretendDimZ);
    // LHL
    L[6] = L[0];
    L[7] = WaveletBase::GetDetailLength(sigPretendDimY);
    L[8] = L[2];
    // LHH
    L[9] = L[0];
    L[10] = L[7];
    L[11] = L[5];
    // HLL
    L[12] = WaveletBase::GetDetailLength(sigPretendDimX);
    L[13] = L[1];
    L[14] = L[2];
    // HLH
    L[15] = L[12];
    L[16] = L[1];
    L[17] = L[5];
    // HHL
    L[18] = L[12];
    L[19] = L[7];
    L[20] = L[2];
    // HHH
    L[21] = L[12];
    L[22] = L[7];
    L[23] = L[5];

    L[24] = sigPretendDimX;
    L[25] = sigPretendDimY;
    L[26] = sigPretendDimZ;

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    bool oddLow = true;
    if (filterLen % 2 != 0)
    {
      oddLow = false;
    }
    vtkm::Id addLen = filterLen / 2;

    using ValueType = typename ArrayInType::ValueType;
    using ArrayType = vtkm::cont::ArrayHandle<ValueType>;
    using LeftRightXFormType = vtkm::worklet::wavelets::ForwardTransform3DLeftRight<DeviceTag>;
    using TopDownXFormType = vtkm::worklet::wavelets::ForwardTransform3DTopDown<DeviceTag>;
    using FrontBackXFormType = vtkm::worklet::wavelets::ForwardTransform3DFrontBack<DeviceTag>;
    using LeftRightDispatcherType =
      vtkm::worklet::DispatcherMapField<LeftRightXFormType, DeviceTag>;
    using TopDownDispatcherType = vtkm::worklet::DispatcherMapField<TopDownXFormType, DeviceTag>;
    using FrontBackDispatcherType =
      vtkm::worklet::DispatcherMapField<FrontBackXFormType, DeviceTag>;

    vtkm::cont::Timer<DeviceTag> timer;
    vtkm::Float64 computationTime = 0.0;

    // First transform in X direction
    ArrayType afterX;
    afterX.PrepareForOutput(sigPretendDimX * sigPretendDimY * sigPretendDimZ, DeviceTag());
    {
      ArrayType leftExt, rightExt;
      this->Extend3DLeftRight(sigIn,
                              sigDimX,
                              sigDimY,
                              sigDimZ,
                              sigStartX,
                              sigStartY,
                              sigStartZ,
                              sigPretendDimX,
                              sigPretendDimY,
                              sigPretendDimZ,
                              leftExt,
                              rightExt,
                              addLen,
                              WaveletBase::wmode,
                              WaveletBase::wmode,
                              false,
                              false,
                              DeviceTag());
      LeftRightXFormType worklet(WaveletBase::filter.GetLowDecomposeFilter(),
                                 WaveletBase::filter.GetHighDecomposeFilter(),
                                 filterLen,
                                 L[0],
                                 oddLow,
                                 addLen,
                                 sigPretendDimY,
                                 sigPretendDimZ,
                                 sigDimX,
                                 sigDimY,
                                 sigDimZ,
                                 sigStartX,
                                 sigStartY,
                                 sigStartZ,
                                 sigPretendDimX,
                                 sigPretendDimY,
                                 sigPretendDimZ,
                                 addLen,
                                 sigPretendDimY,
                                 sigPretendDimZ);
      LeftRightDispatcherType dispatcher(worklet);
      timer.Reset();
      dispatcher.Invoke(leftExt, sigIn, rightExt, afterX);
      computationTime += timer.GetElapsedTime();
    }

    if (discardSigIn)
    {
      sigIn.ReleaseResourcesExecution();
    }

    // Then do transform in Y direction
    ArrayType afterY;
    afterY.PrepareForOutput(sigPretendDimX * sigPretendDimY * sigPretendDimZ, DeviceTag());
    {
      ArrayType topExt, bottomExt;
      this->Extend3DTopDown(afterX,
                            sigPretendDimX,
                            sigPretendDimY,
                            sigPretendDimZ,
                            0,
                            0,
                            0,
                            sigPretendDimX,
                            sigPretendDimY,
                            sigPretendDimZ,
                            topExt,
                            bottomExt,
                            addLen,
                            WaveletBase::wmode,
                            WaveletBase::wmode,
                            false,
                            false,
                            DeviceTag());
      TopDownXFormType worklet(WaveletBase::filter.GetLowDecomposeFilter(),
                               WaveletBase::filter.GetHighDecomposeFilter(),
                               filterLen,
                               L[1],
                               oddLow,
                               sigPretendDimX,
                               addLen,
                               sigPretendDimZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               0,
                               0,
                               0,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               sigPretendDimX,
                               addLen,
                               sigPretendDimZ);
      TopDownDispatcherType dispatcher(worklet);
      timer.Reset();
      dispatcher.Invoke(topExt, afterX, bottomExt, afterY);
      computationTime += timer.GetElapsedTime();
    }

    // Then do transform in Z direction
    afterX.ReleaseResources(); // release afterX
    {
      ArrayType frontExt, backExt;
      coeffOut.PrepareForOutput(sigPretendDimX * sigPretendDimY * sigPretendDimZ, DeviceTag());
      this->Extend3DFrontBack(afterY,
                              sigPretendDimX,
                              sigPretendDimY,
                              sigPretendDimZ,
                              0,
                              0,
                              0,
                              sigPretendDimX,
                              sigPretendDimY,
                              sigPretendDimZ,
                              frontExt,
                              backExt,
                              addLen,
                              WaveletBase::wmode,
                              WaveletBase::wmode,
                              false,
                              false,
                              DeviceTag());
      FrontBackXFormType worklet(WaveletBase::filter.GetLowDecomposeFilter(),
                                 WaveletBase::filter.GetHighDecomposeFilter(),
                                 filterLen,
                                 L[1],
                                 oddLow,
                                 sigPretendDimX,
                                 sigPretendDimY,
                                 addLen,
                                 sigPretendDimX,
                                 sigPretendDimY,
                                 sigPretendDimZ,
                                 0,
                                 0,
                                 0,
                                 sigPretendDimX,
                                 sigPretendDimY,
                                 sigPretendDimZ,
                                 sigPretendDimX,
                                 sigPretendDimY,
                                 addLen);
      FrontBackDispatcherType dispatcher(worklet);
      timer.Reset();
      dispatcher.Invoke(frontExt, afterY, backExt, coeffOut);
      computationTime += timer.GetElapsedTime();
    }

    return computationTime;
  }

  // Performs one level of IDWT on a small cube of a big cube
  // The output array has the same dimensions as the small cube.
  template <typename ArrayInType, typename ArrayOutType, typename DeviceTag>
  vtkm::Float64 IDWT3D(ArrayInType& coeffIn,
                       vtkm::Id inDimX,
                       vtkm::Id inDimY,
                       vtkm::Id inDimZ,
                       vtkm::Id inStartX,
                       vtkm::Id inStartY,
                       vtkm::Id inStartZ,
                       const std::vector<vtkm::Id>& L,
                       ArrayOutType& sigOut,
                       bool discardCoeffIn, // can we discard coeffIn?
                       DeviceTag)
  {
    //VTKM_ASSERT( L.size() == 27 );
    //VTKM_ASSERT( inDimX * inDimY * inDimZ == coeffIn.GetNumberOfValues() );
    vtkm::Id inPretendDimX = L[0] + L[12];
    vtkm::Id inPretendDimY = L[1] + L[7];
    vtkm::Id inPretendDimZ = L[2] + L[5];

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    using BasicArrayType = vtkm::cont::ArrayHandle<typename ArrayInType::ValueType>;
    using LeftRightXFormType = vtkm::worklet::wavelets::InverseTransform3DLeftRight<DeviceTag>;
    using TopDownXFormType = vtkm::worklet::wavelets::InverseTransform3DTopDown<DeviceTag>;
    using FrontBackXFormType = vtkm::worklet::wavelets::InverseTransform3DFrontBack<DeviceTag>;
    using LeftRightDispatcherType =
      vtkm::worklet::DispatcherMapField<LeftRightXFormType, DeviceTag>;
    using TopDownDispatcherType = vtkm::worklet::DispatcherMapField<TopDownXFormType, DeviceTag>;
    using FrontBackDispatcherType =
      vtkm::worklet::DispatcherMapField<FrontBackXFormType, DeviceTag>;

    vtkm::cont::Timer<DeviceTag> timer;
    vtkm::Float64 computationTime = 0.0;

    // First, inverse transform in Z direction
    BasicArrayType afterZ;
    afterZ.PrepareForOutput(inPretendDimX * inPretendDimY * inPretendDimZ, DeviceTag());
    {
      BasicArrayType ext1, ext2, ext3, ext4;
      vtkm::Id extDimX = inPretendDimX;
      vtkm::Id extDimY = inPretendDimY;
      vtkm::Id ext1DimZ = 0, ext2DimZ = 0, ext3DimZ = 0, ext4DimZ = 0;
      this->IDWTHelper3DFrontBack(coeffIn,
                                  inDimX,
                                  inDimY,
                                  inDimZ,
                                  inStartX,
                                  inStartY,
                                  inStartZ,
                                  inPretendDimX,
                                  inPretendDimY,
                                  inPretendDimZ,
                                  L[2],
                                  L[5],
                                  ext1,
                                  ext2,
                                  ext3,
                                  ext4,
                                  ext1DimZ,
                                  ext2DimZ,
                                  ext3DimZ,
                                  ext4DimZ,
                                  filterLen,
                                  wmode,
                                  DeviceTag());
      FrontBackXFormType worklet(WaveletBase::filter.GetLowReconstructFilter(),
                                 WaveletBase::filter.GetHighReconstructFilter(),
                                 filterLen,
                                 extDimX,
                                 extDimY,
                                 ext1DimZ, // ext1
                                 extDimX,
                                 extDimY,
                                 ext2DimZ, // ext2
                                 extDimX,
                                 extDimY,
                                 ext3DimZ, // ext3
                                 extDimX,
                                 extDimY,
                                 ext4DimZ, // ext4
                                 inPretendDimX,
                                 inPretendDimY,
                                 L[2], // cA
                                 inPretendDimX,
                                 inPretendDimY,
                                 L[5], // cD
                                 inDimX,
                                 inDimY,
                                 inDimZ, // coeffIn
                                 inStartX,
                                 inStartY,
                                 inStartZ); // coeffIn
      FrontBackDispatcherType dispatcher(worklet);
      timer.Reset();
      dispatcher.Invoke(ext1, ext2, ext3, ext4, coeffIn, afterZ);
      computationTime += timer.GetElapsedTime();
    }

    if (discardCoeffIn)
    {
      coeffIn.ReleaseResourcesExecution();
    }

    // Second, inverse transform in Y direction
    BasicArrayType afterY;
    afterY.PrepareForOutput(inPretendDimX * inPretendDimY * inPretendDimZ, DeviceTag());
    {
      BasicArrayType ext1, ext2, ext3, ext4;
      vtkm::Id extDimX = inPretendDimX;
      vtkm::Id extDimZ = inPretendDimZ;
      vtkm::Id ext1DimY = 0, ext2DimY = 0, ext3DimY = 0, ext4DimY = 0;
      this->IDWTHelper3DTopDown(afterZ,
                                inPretendDimX,
                                inPretendDimY,
                                inPretendDimZ,
                                0,
                                0,
                                0,
                                inPretendDimX,
                                inPretendDimY,
                                inPretendDimZ,
                                L[1],
                                L[7],
                                ext1,
                                ext2,
                                ext3,
                                ext4,
                                ext1DimY,
                                ext2DimY,
                                ext3DimY,
                                ext4DimY,
                                filterLen,
                                wmode,
                                DeviceTag());
      TopDownXFormType worklet(WaveletBase::filter.GetLowReconstructFilter(),
                               WaveletBase::filter.GetHighReconstructFilter(),
                               filterLen,
                               extDimX,
                               ext1DimY,
                               extDimZ, // ext1
                               extDimX,
                               ext2DimY,
                               extDimZ, // ext2
                               extDimX,
                               ext3DimY,
                               extDimZ, // ext3
                               extDimX,
                               ext4DimY,
                               extDimZ, // ext4
                               inPretendDimX,
                               L[1],
                               inPretendDimZ, // cA
                               inPretendDimX,
                               L[7],
                               inPretendDimZ, // cD
                               inPretendDimX,
                               inPretendDimY,
                               inPretendDimZ, // actual signal
                               0,
                               0,
                               0);
      TopDownDispatcherType dispatcher(worklet);
      timer.Reset();
      dispatcher.Invoke(ext1, ext2, ext3, ext4, afterZ, afterY);
      computationTime += timer.GetElapsedTime();
    }

    // Lastly, inverse transform in X direction
    afterZ.ReleaseResources();
    {
      BasicArrayType ext1, ext2, ext3, ext4;
      vtkm::Id extDimY = inPretendDimY;
      vtkm::Id extDimZ = inPretendDimZ;
      vtkm::Id ext1DimX = 0, ext2DimX = 0, ext3DimX = 0, ext4DimX = 0;
      this->IDWTHelper3DLeftRight(afterY,
                                  inPretendDimX,
                                  inPretendDimY,
                                  inPretendDimZ,
                                  0,
                                  0,
                                  0,
                                  inPretendDimX,
                                  inPretendDimY,
                                  inPretendDimZ,
                                  L[0],
                                  L[12],
                                  ext1,
                                  ext2,
                                  ext3,
                                  ext4,
                                  ext1DimX,
                                  ext2DimX,
                                  ext3DimX,
                                  ext4DimX,
                                  filterLen,
                                  wmode,
                                  DeviceTag());
      sigOut.PrepareForOutput(inPretendDimX * inPretendDimY * inPretendDimZ, DeviceTag());
      LeftRightXFormType worklet(WaveletBase::filter.GetLowReconstructFilter(),
                                 WaveletBase::filter.GetHighReconstructFilter(),
                                 filterLen,
                                 ext1DimX,
                                 extDimY,
                                 extDimZ, // ext1
                                 ext2DimX,
                                 extDimY,
                                 extDimZ, // ext2
                                 ext3DimX,
                                 extDimY,
                                 extDimZ, // ext3
                                 ext4DimX,
                                 extDimY,
                                 extDimZ, // ext4
                                 L[0],
                                 inPretendDimY,
                                 inPretendDimZ, // cA
                                 L[12],
                                 inPretendDimY,
                                 inPretendDimZ, // cD
                                 inPretendDimX,
                                 inPretendDimY,
                                 inPretendDimZ, // actual signal
                                 0,
                                 0,
                                 0);
      LeftRightDispatcherType dispatcher(worklet);
      timer.Reset();
      dispatcher.Invoke(ext1, ext2, ext3, ext4, afterY, sigOut);
      computationTime += timer.GetElapsedTime();
    }

    return computationTime;
  }

  //=============================================================================

  template <typename SigInArrayType, typename ExtensionArrayType, typename DeviceTag>
  vtkm::Id Extend2D(const SigInArrayType& sigIn, // Input
                    vtkm::Id sigDimX,
                    vtkm::Id sigDimY,
                    vtkm::Id sigStartX,
                    vtkm::Id sigStartY,
                    vtkm::Id sigPretendDimX,
                    vtkm::Id sigPretendDimY,
                    ExtensionArrayType& ext1, // left/top extension
                    ExtensionArrayType& ext2, // right/bottom extension
                    vtkm::Id addLen,
                    vtkm::worklet::wavelets::DWTMode ext1Method,
                    vtkm::worklet::wavelets::DWTMode ext2Method,
                    bool pretendSigPaddedZero,
                    bool padZeroAtExt2,
                    bool modeLR, // true = left-right
                                 // false = top-down
                    DeviceTag)
  {
    // pretendSigPaddedZero and padZeroAtExt2 cannot happen at the same time
    VTKM_ASSERT(!pretendSigPaddedZero || !padZeroAtExt2);

    if (addLen == 0) // Haar kernel
    {
      ext1.PrepareForOutput(0, DeviceTag()); // no need to extend on left/top
      if (pretendSigPaddedZero || padZeroAtExt2)
      {
        if (modeLR) // right extension
        {
          ext2.PrepareForOutput(sigPretendDimY, DeviceTag());
          WaveletBase::DeviceAssignZero2DColumn(ext2, 1, sigPretendDimY, 0, DeviceTag());
        }
        else // bottom extension
        {
          ext2.PrepareForOutput(sigPretendDimX, DeviceTag());
          WaveletBase::DeviceAssignZero2DRow(ext2, sigPretendDimX, 1, 0, DeviceTag());
        }
      }
      else
      {
        ext2.PrepareForOutput(0, DeviceTag());
      }
      return 0;
    }

    using ValueType = typename SigInArrayType::ValueType;
    using ExtendArrayType = vtkm::cont::ArrayHandle<ValueType>;
    using ExtensionWorklet = vtkm::worklet::wavelets::ExtensionWorklet2D;
    using DispatcherType = typename vtkm::worklet::DispatcherMapField<ExtensionWorklet, DeviceTag>;
    vtkm::Id extDimX, extDimY;
    vtkm::worklet::wavelets::ExtensionDirection dir;

    { // Work on left/top extension
      if (modeLR)
      {
        dir = LEFT;
        extDimX = addLen;
        extDimY = sigPretendDimY;
      }
      else
      {
        dir = TOP;
        extDimX = sigPretendDimX;
        extDimY = addLen;
      }
      ext1.PrepareForOutput(extDimX * extDimY, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               sigDimX,
                               sigDimY,
                               sigStartX,
                               sigStartY,
                               sigPretendDimX,
                               sigPretendDimY, // use this area
                               ext1Method,
                               dir,
                               false); // not treating sigIn as having zeros
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext1, sigIn);
    }

    // Work on right/bottom extension
    if (!pretendSigPaddedZero && !padZeroAtExt2)
    {
      if (modeLR)
      {
        dir = RIGHT;
        extDimX = addLen;
        extDimY = sigPretendDimY;
      }
      else
      {
        dir = BOTTOM;
        extDimX = sigPretendDimX;
        extDimY = addLen;
      }
      ext2.PrepareForOutput(extDimX * extDimY, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               sigDimX,
                               sigDimY,
                               sigStartX,
                               sigStartY,
                               sigPretendDimX,
                               sigPretendDimY, // use this area
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else if (!pretendSigPaddedZero && padZeroAtExt2)
    {
      if (modeLR)
      {
        dir = RIGHT;
        extDimX = addLen + 1;
        extDimY = sigPretendDimY;
      }
      else
      {
        dir = BOTTOM;
        extDimX = sigPretendDimX;
        extDimY = addLen + 1;
      }
      ext2.PrepareForOutput(extDimX * extDimY, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               sigDimX,
                               sigDimY,
                               sigStartX,
                               sigStartY,
                               sigPretendDimX,
                               sigPretendDimY,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
      /* Pad a zero at the end of cDTemp, when cDTemp is forced to have the same
         length as cATemp. For example, with odd length signal, cA is 1 element
         longer than cD.
       */
      /* Update 10/24/2016: the extra element of cD shouldn't be zero, just be
       * whatever it extends to be.
       * if( modeLR )
       *   WaveletBase::DeviceAssignZero2DColumn( ext2, extDimX, extDimY,
       *                                          extDimX-1, DeviceTag() );
       * else
       *   WaveletBase::DeviceAssignZero2DRow( ext2, extDimX, extDimY,
       *                                       extDimY-1, DeviceTag() );
       */
    }
    else // pretendSigPaddedZero
    {
      ExtendArrayType ext2Temp;
      if (modeLR)
      {
        dir = RIGHT;
        extDimX = addLen;
        extDimY = sigPretendDimY;
      }
      else
      {
        dir = BOTTOM;
        extDimX = sigPretendDimX;
        extDimY = addLen;
      }
      ext2Temp.PrepareForOutput(extDimX * extDimY, DeviceTag());
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               sigDimX,
                               sigDimY,
                               sigStartX,
                               sigStartY,
                               sigPretendDimX,
                               sigPretendDimY,
                               ext2Method,
                               dir,
                               true); // pretend sig is padded a zero
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2Temp, sigIn);

      if (modeLR)
      {
        ext2.PrepareForOutput((extDimX + 1) * extDimY, DeviceTag());
        WaveletBase::DeviceRectangleCopyTo(
          ext2Temp, extDimX, extDimY, ext2, extDimX + 1, extDimY, 1, 0, DeviceTag());
        WaveletBase::DeviceAssignZero2DColumn(ext2, extDimX + 1, extDimY, 0, DeviceTag());
      }
      else
      {
        ext2.PrepareForOutput(extDimX * (extDimY + 1), DeviceTag());
        WaveletBase::DeviceRectangleCopyTo(
          ext2Temp, extDimX, extDimY, ext2, extDimX, extDimY + 1, 0, 1, DeviceTag());
        WaveletBase::DeviceAssignZero2DRow(ext2, extDimX, extDimY + 1, 0, DeviceTag());
      }
    }
    return 0;
  }

  // Extend 1D signal
  template <typename SigInArrayType, typename SigExtendedArrayType, typename DeviceTag>
  vtkm::Id Extend1D(const SigInArrayType& sigIn,  // Input
                    SigExtendedArrayType& sigOut, // Output
                    vtkm::Id addLen,
                    vtkm::worklet::wavelets::DWTMode leftExtMethod,
                    vtkm::worklet::wavelets::DWTMode rightExtMethod,
                    bool attachZeroRightLeft,
                    bool attachZeroRightRight,
                    DeviceTag)
  {
    // "right extension" can be attached a zero on either end, but not both ends.
    VTKM_ASSERT(!attachZeroRightRight || !attachZeroRightLeft);

    using ValueType = typename SigInArrayType::ValueType;
    using ExtensionArrayType = vtkm::cont::ArrayHandle<ValueType>;
    using ArrayConcat = vtkm::cont::ArrayHandleConcatenate<ExtensionArrayType, SigInArrayType>;

    ExtensionArrayType leftExtend, rightExtend;

    if (addLen == 0) // Haar kernel
    {
      if (attachZeroRightLeft || attachZeroRightRight)
      {
        leftExtend.PrepareForOutput(0, DeviceTag());
        rightExtend.PrepareForOutput(1, DeviceTag());
        WaveletBase::DeviceAssignZero(rightExtend, 0, DeviceTag());
      }
      else
      {
        leftExtend.PrepareForOutput(0, DeviceTag());
        rightExtend.PrepareForOutput(0, DeviceTag());
      }
      ArrayConcat leftOn(leftExtend, sigIn);
      sigOut = vtkm::cont::make_ArrayHandleConcatenate(leftOn, rightExtend);
      return 0;
    }

    leftExtend.PrepareForOutput(addLen, DeviceTag());
    vtkm::Id sigInLen = sigIn.GetNumberOfValues();

    using LeftSYMH = vtkm::worklet::wavelets::LeftSYMHExtentionWorklet;
    using LeftSYMW = vtkm::worklet::wavelets::LeftSYMWExtentionWorklet;
    using RightSYMH = vtkm::worklet::wavelets::RightSYMHExtentionWorklet;
    using RightSYMW = vtkm::worklet::wavelets::RightSYMWExtentionWorklet;
    using LeftASYMH = vtkm::worklet::wavelets::LeftASYMHExtentionWorklet;
    using LeftASYMW = vtkm::worklet::wavelets::LeftASYMWExtentionWorklet;
    using RightASYMH = vtkm::worklet::wavelets::RightASYMHExtentionWorklet;
    using RightASYMW = vtkm::worklet::wavelets::RightASYMWExtentionWorklet;

    switch (leftExtMethod)
    {
      case SYMH:
      {
        LeftSYMH worklet(addLen);
        vtkm::worklet::DispatcherMapField<LeftSYMH, DeviceTag> dispatcher(worklet);
        dispatcher.Invoke(leftExtend, sigIn);
        break;
      }
      case SYMW:
      {
        LeftSYMW worklet(addLen);
        vtkm::worklet::DispatcherMapField<LeftSYMW, DeviceTag> dispatcher(worklet);
        dispatcher.Invoke(leftExtend, sigIn);
        break;
      }
      case ASYMH:
      {
        LeftASYMH worklet(addLen);
        vtkm::worklet::DispatcherMapField<LeftASYMH, DeviceTag> dispatcher(worklet);
        dispatcher.Invoke(leftExtend, sigIn);
        break;
      }
      case ASYMW:
      {
        LeftASYMW worklet(addLen);
        vtkm::worklet::DispatcherMapField<LeftASYMW, DeviceTag> dispatcher(worklet);
        dispatcher.Invoke(leftExtend, sigIn);
        break;
      }
      default:
      {
        vtkm::cont::ErrorInternal("Left extension mode not supported!");
        return 1;
      }
    }

    if (!attachZeroRightLeft) // no attach zero, or only attach on RightRight
    {
      // Allocate memory
      if (attachZeroRightRight)
      {
        rightExtend.PrepareForOutput(addLen + 1, DeviceTag());
      }
      else
      {
        rightExtend.PrepareForOutput(addLen, DeviceTag());
      }

      switch (rightExtMethod)
      {
        case SYMH:
        {
          RightSYMH worklet(sigInLen);
          vtkm::worklet::DispatcherMapField<RightSYMH, DeviceTag> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigIn);
          break;
        }
        case SYMW:
        {
          RightSYMW worklet(sigInLen);
          vtkm::worklet::DispatcherMapField<RightSYMW, DeviceTag> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigIn);
          break;
        }
        case ASYMH:
        {
          RightASYMH worklet(sigInLen);
          vtkm::worklet::DispatcherMapField<RightASYMH, DeviceTag> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigIn);
          break;
        }
        case ASYMW:
        {
          RightASYMW worklet(sigInLen);
          vtkm::worklet::DispatcherMapField<RightASYMW, DeviceTag> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigIn);
          break;
        }
        default:
        {
          vtkm::cont::ErrorInternal("Right extension mode not supported!");
          return 1;
        }
      }
      if (attachZeroRightRight)
      {
        WaveletBase::DeviceAssignZero(rightExtend, addLen, DeviceTag());
      }
    }
    else // attachZeroRightLeft mode
    {
      using ConcatArray = vtkm::cont::ArrayHandleConcatenate<SigInArrayType, ExtensionArrayType>;
      // attach a zero at the end of sigIn
      ExtensionArrayType singleValArray;
      singleValArray.PrepareForOutput(1, DeviceTag());
      WaveletBase::DeviceAssignZero(singleValArray, 0, DeviceTag());
      ConcatArray sigInPlusOne(sigIn, singleValArray);

      // allocate memory for extension
      rightExtend.PrepareForOutput(addLen, DeviceTag());

      switch (rightExtMethod)
      {
        case SYMH:
        {
          RightSYMH worklet(sigInLen + 1);
          vtkm::worklet::DispatcherMapField<RightSYMH, DeviceTag> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigInPlusOne);
          break;
        }
        case SYMW:
        {
          RightSYMW worklet(sigInLen + 1);
          vtkm::worklet::DispatcherMapField<RightSYMW, DeviceTag> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigInPlusOne);
          break;
        }
        case ASYMH:
        {
          RightASYMH worklet(sigInLen + 1);
          vtkm::worklet::DispatcherMapField<RightASYMH, DeviceTag> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigInPlusOne);
          break;
        }
        case ASYMW:
        {
          RightASYMW worklet(sigInLen + 1);
          vtkm::worklet::DispatcherMapField<RightASYMW, DeviceTag> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigInPlusOne);
          break;
        }
        default:
        {
          vtkm::cont::ErrorInternal("Right extension mode not supported!");
          return 1;
        }
      }

      // make a copy of rightExtend with a zero attached to the left
      ExtensionArrayType rightExtendPlusOne;
      rightExtendPlusOne.PrepareForOutput(addLen + 1, DeviceTag());
      WaveletBase::DeviceCopyStartX(rightExtend, rightExtendPlusOne, 1, DeviceTag());
      WaveletBase::DeviceAssignZero(rightExtendPlusOne, 0, DeviceTag());
      rightExtend = rightExtendPlusOne;
    }

    ArrayConcat leftOn(leftExtend, sigIn);
    sigOut = vtkm::cont::make_ArrayHandleConcatenate(leftOn, rightExtend);

    return 0;
  }

  // Performs one level of 1D discrete wavelet transform
  // It takes care of boundary conditions, etc.
  template <typename SignalArrayType, typename CoeffArrayType, typename DeviceTag>
  vtkm::Float64 DWT1D(const SignalArrayType& sigIn, // Input
                      CoeffArrayType& coeffOut,     // Output: cA followed by cD
                      std::vector<vtkm::Id>& L,     // Output: how many cA and cD.
                      DeviceTag)
  {
    vtkm::Id sigInLen = sigIn.GetNumberOfValues();
    if (GetWaveletMaxLevel(sigInLen) < 1)
    {
      vtkm::cont::ErrorInternal("Signal is too short to perform DWT!");
      return -1;
    }

    //VTKM_ASSERT( L.size() == 3 );
    L[0] = WaveletBase::GetApproxLength(sigInLen);
    L[1] = WaveletBase::GetDetailLength(sigInLen);
    L[2] = sigInLen;

    //VTKM_ASSERT( L[0] + L[1] == L[2] );

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();

    bool doSymConv = false;
    if (WaveletBase::filter.isSymmetric())
    {
      if ((WaveletBase::wmode == SYMW && (filterLen % 2 != 0)) ||
          (WaveletBase::wmode == SYMH && (filterLen % 2 == 0)))
      {
        doSymConv = true;
      }
    }

    vtkm::Id sigConvolvedLen = L[0] + L[1]; // approx + detail coeffs
    vtkm::Id addLen;                        // for extension
    bool oddLow = true;
    bool oddHigh = true;
    if (filterLen % 2 != 0)
    {
      oddLow = false;
    }
    if (doSymConv)
    {
      addLen = filterLen / 2;
      if (sigInLen % 2 != 0)
      {
        sigConvolvedLen += 1;
      }
    }
    else
    {
      addLen = filterLen - 1;
    }

    vtkm::Id sigExtendedLen = sigInLen + 2 * addLen;

    using SigInValueType = typename SignalArrayType::ValueType;
    using SigInBasic = vtkm::cont::ArrayHandle<SigInValueType>;

    using ConcatType1 = vtkm::cont::ArrayHandleConcatenate<SigInBasic, SignalArrayType>;
    using ConcatType2 = vtkm::cont::ArrayHandleConcatenate<ConcatType1, SigInBasic>;

    ConcatType2 sigInExtended;

    this->Extend1D(sigIn,
                   sigInExtended,
                   addLen,
                   WaveletBase::wmode,
                   WaveletBase::wmode,
                   false,
                   false,
                   DeviceTag());
    //VTKM_ASSERT( sigInExtended.GetNumberOfValues() == sigExtendedLen );

    // initialize a worklet for forward transform
    vtkm::worklet::wavelets::ForwardTransform<DeviceTag> forwardTransform(
      WaveletBase::filter.GetLowDecomposeFilter(),
      WaveletBase::filter.GetHighDecomposeFilter(),
      filterLen,
      L[0],
      L[1],
      oddLow,
      oddHigh);

    coeffOut.PrepareForOutput(sigExtendedLen, DeviceTag());
    vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::ForwardTransform<DeviceTag>,
                                      DeviceTag>
      dispatcher(forwardTransform);
    // put a timer
    vtkm::cont::Timer<DeviceTag> timer;
    dispatcher.Invoke(sigInExtended, coeffOut);
    vtkm::Float64 elapsedTime = timer.GetElapsedTime();

    //VTKM_ASSERT( L[0] + L[1] <= coeffOut.GetNumberOfValues() );
    coeffOut.Shrink(L[0] + L[1]);

    return elapsedTime;
  }

  // Performs one level of inverse wavelet transform
  // It takes care of boundary conditions, etc.
  template <typename CoeffArrayType, typename SignalArrayType, typename DeviceTag>
  vtkm::Float64 IDWT1D(const CoeffArrayType& coeffIn, // Input, cA followed by cD
                       std::vector<vtkm::Id>& L,      // Input, how many cA and cD
                       SignalArrayType& sigOut,       // Output
                       DeviceTag)
  {
    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    bool doSymConv = false;
    vtkm::worklet::wavelets::DWTMode cALeftMode = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cARightMode = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cDLeftMode = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cDRightMode = WaveletBase::wmode;

    if (WaveletBase::filter.isSymmetric()) // this is always true with the 1st 4 filters.
    {
      if ((WaveletBase::wmode == SYMW && (filterLen % 2 != 0)) ||
          (WaveletBase::wmode == SYMH && (filterLen % 2 == 0)))
      {
        doSymConv = true; // doSymConv is always true with the 1st 4 filters.

        if (WaveletBase::wmode == SYMH)
        {
          cDLeftMode = ASYMH;
          if (L[2] % 2 != 0)
          {
            cARightMode = SYMW;
            cDRightMode = ASYMW;
          }
          else
          {
            cDRightMode = ASYMH;
          }
        }
        else
        {
          cDLeftMode = SYMH;
          if (L[2] % 2 != 0)
          {
            cARightMode = SYMW;
            cDRightMode = SYMH;
          }
          else
          {
            cARightMode = SYMH;
          }
        }
      }
    }

    vtkm::Id cATempLen, cDTempLen; //, reconTempLen;
    vtkm::Id addLen = 0;
    vtkm::Id cDPadLen = 0;
    if (doSymConv) // extend cA and cD
    {
      addLen = filterLen / 4; // addLen == 0 for Haar kernel
      if ((L[0] > L[1]) && (WaveletBase::wmode == SYMH))
      {
        cDPadLen = L[0];
      }
      cATempLen = L[0] + 2 * addLen;
      cDTempLen = cATempLen; // same length
    }
    else // not extend cA and cD
    {    //  (biorthogonal kernels won't come into this case)
      cATempLen = L[0];
      cDTempLen = L[1];
    }

    using IdArrayType = vtkm::cont::ArrayHandleCounting<vtkm::Id>;
    using PermutArrayType = vtkm::cont::ArrayHandlePermutation<IdArrayType, CoeffArrayType>;

    // Separate cA and cD
    IdArrayType approxIndices(0, 1, L[0]);
    IdArrayType detailIndices(L[0], 1, L[1]);
    PermutArrayType cA(approxIndices, coeffIn);
    PermutArrayType cD(detailIndices, coeffIn);

    using CoeffValueType = typename CoeffArrayType::ValueType;
    using ExtensionArrayType = vtkm::cont::ArrayHandle<CoeffValueType>;
    using Concat1 = vtkm::cont::ArrayHandleConcatenate<ExtensionArrayType, PermutArrayType>;
    using Concat2 = vtkm::cont::ArrayHandleConcatenate<Concat1, ExtensionArrayType>;

    Concat2 cATemp, cDTemp;

    if (doSymConv) // Actually extend cA and cD
    {
      // first extend cA to be cATemp
      this->Extend1D(cA, cATemp, addLen, cALeftMode, cARightMode, false, false, DeviceTag());

      // Then extend cD based on extension needs
      if (cDPadLen > 0)
      {
        // Add back the missing final cD, 0.0, before doing extension
        this->Extend1D(cD, cDTemp, addLen, cDLeftMode, cDRightMode, true, false, DeviceTag());
      }
      else
      {
        vtkm::Id cDTempLenWouldBe = L[1] + 2 * addLen;
        if (cDTempLenWouldBe == cDTempLen)
        {
          this->Extend1D(cD, cDTemp, addLen, cDLeftMode, cDRightMode, false, false, DeviceTag());
        }
        else if (cDTempLenWouldBe == cDTempLen - 1)
        {
          this->Extend1D(cD, cDTemp, addLen, cDLeftMode, cDRightMode, false, true, DeviceTag());
        }
        else
        {
          vtkm::cont::ErrorInternal("cDTemp Length not match!");
          return 1;
        }
      }
    }
    else
    {
      // make cATemp
      ExtensionArrayType dummyArray;
      dummyArray.PrepareForOutput(0, DeviceTag());
      Concat1 cALeftOn(dummyArray, cA);
      cATemp =
        vtkm::cont::make_ArrayHandleConcatenate<Concat1, ExtensionArrayType>(cALeftOn, dummyArray);

      // make cDTemp
      Concat1 cDLeftOn(dummyArray, cD);
      cDTemp =
        vtkm::cont::make_ArrayHandleConcatenate<Concat1, ExtensionArrayType>(cDLeftOn, dummyArray);
    }

    vtkm::cont::ArrayHandleConcatenate<Concat2, Concat2> coeffInExtended(cATemp, cDTemp);

    // Allocate memory for sigOut
    sigOut.PrepareForOutput(cATempLen + cDTempLen, DeviceTag());

    vtkm::Float64 elapsedTime = 0;
    if (filterLen % 2 != 0)
    {
      vtkm::worklet::wavelets::InverseTransformOdd<DeviceTag> inverseXformOdd(
        WaveletBase::filter.GetLowReconstructFilter(),
        WaveletBase::filter.GetHighReconstructFilter(),
        filterLen,
        L[0],
        cATempLen);
      vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::InverseTransformOdd<DeviceTag>,
                                        DeviceTag>
        dispatcher(inverseXformOdd);
      // use a timer
      vtkm::cont::Timer<DeviceTag> timer;
      dispatcher.Invoke(coeffInExtended, sigOut);
      elapsedTime = timer.GetElapsedTime();
    }
    else
    {
      vtkm::worklet::wavelets::InverseTransformEven<DeviceTag> inverseXformEven(
        WaveletBase::filter.GetLowReconstructFilter(),
        WaveletBase::filter.GetHighReconstructFilter(),
        filterLen,
        L[0],
        cATempLen,
        !doSymConv);
      vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::InverseTransformEven<DeviceTag>,
                                        DeviceTag>
        dispatcher(inverseXformEven);
      // use a timer
      vtkm::cont::Timer<DeviceTag> timer;
      dispatcher.Invoke(coeffInExtended, sigOut);
      elapsedTime = timer.GetElapsedTime();
    }

    sigOut.Shrink(L[2]);

    return elapsedTime;
  }

  // Performs one level of 2D discrete wavelet transform
  // It takes care of boundary conditions, etc.
  // N.B.
  //  L[0] == L[2]
  //  L[1] == L[5]
  //  L[3] == L[7]
  //  L[4] == L[6]
  //
  //      ____L[0]_______L[4]____
  //      |          |          |
  // L[1] |  cA      |  cDv     | L[5]
  //      |  (LL)    |  (HL)    |
  //      |          |          |
  //      |---------------------|
  //      |          |          |
  //      |  cDh     |  cDd     | L[7]
  // L[3] |  (LH)    |  (HH)    |
  //      |          |          |
  //      |__________|__________|
  //         L[2]       L[6]
  //
  // Performs one level of 2D discrete wavelet transform on a small rectangle of input array
  // The output has the same size as the small rectangle
  template <typename ArrayInType, typename ArrayOutType, typename DeviceTag>
  vtkm::Float64 DWT2D(const ArrayInType& sigIn,
                      vtkm::Id sigDimX,
                      vtkm::Id sigDimY,
                      vtkm::Id sigStartX,
                      vtkm::Id sigStartY,
                      vtkm::Id sigPretendDimX,
                      vtkm::Id sigPretendDimY,
                      ArrayOutType& coeffOut,
                      std::vector<vtkm::Id>& L,
                      DeviceTag)
  {
    L[0] = WaveletBase::GetApproxLength(sigPretendDimX);
    L[2] = L[0];
    L[1] = WaveletBase::GetApproxLength(sigPretendDimY);
    L[5] = L[1];
    L[3] = WaveletBase::GetDetailLength(sigPretendDimY);
    L[7] = L[3];
    L[4] = WaveletBase::GetDetailLength(sigPretendDimX);
    L[6] = L[4];
    L[8] = sigPretendDimX;
    L[9] = sigPretendDimY;

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    bool oddLow = true;
    if (filterLen % 2 != 0)
    {
      oddLow = false;
    }
    vtkm::Id addLen = filterLen / 2;

    using ValueType = typename ArrayInType::ValueType;
    using ArrayType = vtkm::cont::ArrayHandle<ValueType>;
    using ForwardXForm = vtkm::worklet::wavelets::ForwardTransform2D<DeviceTag>;
    using DispatcherType = typename vtkm::worklet::DispatcherMapField<ForwardXForm, DeviceTag>;

    vtkm::cont::Timer<DeviceTag> timer;
    vtkm::Float64 computationTime = 0.0;

    ArrayType afterX;
    afterX.PrepareForOutput(sigPretendDimX * sigPretendDimY, DeviceTag());

    // First transform on rows
    {
      ArrayType leftExt, rightExt;
      this->Extend2D(sigIn,
                     sigDimX,
                     sigDimY,
                     sigStartX,
                     sigStartY,
                     sigPretendDimX,
                     sigPretendDimY,
                     leftExt,
                     rightExt,
                     addLen,
                     WaveletBase::wmode,
                     WaveletBase::wmode,
                     false,
                     false,
                     true,
                     DeviceTag()); // Extend in left-right direction
      ForwardXForm worklet(WaveletBase::filter.GetLowDecomposeFilter(),
                           WaveletBase::filter.GetHighDecomposeFilter(),
                           filterLen,
                           L[0],
                           oddLow,
                           true, // left-right
                           addLen,
                           sigPretendDimY,
                           sigDimX,
                           sigDimY,
                           sigStartX,
                           sigStartY,
                           sigPretendDimX,
                           sigPretendDimY,
                           addLen,
                           sigPretendDimY);
      DispatcherType dispatcher(worklet);
      timer.Reset();
      dispatcher.Invoke(leftExt, sigIn, rightExt, afterX);
      computationTime += timer.GetElapsedTime();
    }

    // Then do transform in Y direction
    {
      ArrayType topExt, bottomExt;
      coeffOut.PrepareForOutput(sigPretendDimX * sigPretendDimY, DeviceTag());
      this->Extend2D(afterX,
                     sigPretendDimX,
                     sigPretendDimY,
                     0,
                     0,
                     sigPretendDimX,
                     sigPretendDimY,
                     topExt,
                     bottomExt,
                     addLen,
                     WaveletBase::wmode,
                     WaveletBase::wmode,
                     false,
                     false,
                     false,
                     DeviceTag()); // Extend in top-down direction
      ForwardXForm worklet(WaveletBase::filter.GetLowDecomposeFilter(),
                           WaveletBase::filter.GetHighDecomposeFilter(),
                           filterLen,
                           L[1],
                           oddLow,
                           false, // top-down
                           sigPretendDimX,
                           addLen,
                           sigPretendDimX,
                           sigPretendDimY,
                           0,
                           0,
                           sigPretendDimX,
                           sigPretendDimY,
                           sigPretendDimX,
                           addLen);
      DispatcherType dispatcher(worklet);
      timer.Reset();
      dispatcher.Invoke(topExt, afterX, bottomExt, coeffOut);
      computationTime += timer.GetElapsedTime();
    }

    return computationTime;
  }

  // Performs one level of IDWT.
  // The output array has the same dimensions as the small rectangle.
  template <typename ArrayInType, typename ArrayOutType, typename DeviceTag>
  vtkm::Float64 IDWT2D(const ArrayInType& coeffIn,
                       vtkm::Id inDimX,
                       vtkm::Id inDimY,
                       vtkm::Id inStartX,
                       vtkm::Id inStartY,
                       const std::vector<vtkm::Id>& L,
                       ArrayOutType& sigOut,
                       DeviceTag)
  {
    vtkm::Id inPretendDimX = L[0] + L[4];
    vtkm::Id inPretendDimY = L[1] + L[3];

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    using BasicArrayType = vtkm::cont::ArrayHandle<typename ArrayInType::ValueType>;
    using IDWT2DWorklet = vtkm::worklet::wavelets::InverseTransform2D<DeviceTag>;
    using Dispatcher = vtkm::worklet::DispatcherMapField<IDWT2DWorklet, DeviceTag>;
    vtkm::cont::Timer<DeviceTag> timer;
    vtkm::Float64 computationTime = 0.0;

    // First inverse transform on columns
    BasicArrayType afterY;
    {
      BasicArrayType ext1, ext2, ext3, ext4;
      vtkm::Id extDimX = inPretendDimX;
      vtkm::Id ext1DimY = 0, ext2DimY = 0, ext3DimY = 0, ext4DimY = 0;
      this->IDWTHelper2DTopDown(coeffIn,
                                inDimX,
                                inDimY,
                                inStartX,
                                inStartY,
                                inPretendDimX,
                                inPretendDimY,
                                L[1],
                                L[3],
                                ext1,
                                ext2,
                                ext3,
                                ext4,
                                ext1DimY,
                                ext2DimY,
                                ext3DimY,
                                ext4DimY,
                                filterLen,
                                wmode,
                                DeviceTag());

      afterY.PrepareForOutput(inPretendDimX * inPretendDimY, DeviceTag());
      IDWT2DWorklet worklet(WaveletBase::filter.GetLowReconstructFilter(),
                            WaveletBase::filter.GetHighReconstructFilter(),
                            filterLen,
                            extDimX,
                            ext1DimY, // ext1
                            inPretendDimX,
                            L[1], // cA
                            extDimX,
                            ext2DimY, // ext2
                            extDimX,
                            ext3DimY, // ext3
                            inPretendDimX,
                            L[3], // cD
                            extDimX,
                            ext4DimY, // ext4
                            inDimX,
                            inDimY, // coeffIn
                            inStartX,
                            inStartY, // coeffIn
                            false);   // top-down
      Dispatcher dispatcher(worklet);
      timer.Reset();
      dispatcher.Invoke(ext1, ext2, ext3, ext4, coeffIn, afterY);
      computationTime += timer.GetElapsedTime();
    }

    // Then inverse transform on rows
    {
      BasicArrayType ext1, ext2, ext3, ext4;
      vtkm::Id extDimY = inPretendDimY;
      vtkm::Id ext1DimX = 0, ext2DimX = 0, ext3DimX = 0, ext4DimX = 0;
      this->IDWTHelper2DLeftRight(afterY,
                                  inPretendDimX,
                                  inPretendDimY,
                                  0,
                                  0,
                                  inPretendDimX,
                                  inPretendDimY,
                                  L[0],
                                  L[4],
                                  ext1,
                                  ext2,
                                  ext3,
                                  ext4,
                                  ext1DimX,
                                  ext2DimX,
                                  ext3DimX,
                                  ext4DimX,
                                  filterLen,
                                  wmode,
                                  DeviceTag());
      sigOut.PrepareForOutput(inPretendDimX * inPretendDimY, DeviceTag());
      IDWT2DWorklet worklet(WaveletBase::filter.GetLowReconstructFilter(),
                            WaveletBase::filter.GetHighReconstructFilter(),
                            filterLen,
                            ext1DimX,
                            extDimY, // ext1
                            L[0],
                            inPretendDimY, // cA
                            ext2DimX,
                            extDimY, // ext2
                            ext3DimX,
                            extDimY, // ext3
                            L[4],
                            inPretendDimY, // cA
                            ext4DimX,
                            extDimY, // ext4
                            inPretendDimX,
                            inPretendDimY,
                            0,
                            0,
                            true); // left-right
      Dispatcher dispatcher(worklet);
      timer.Reset();
      dispatcher.Invoke(ext1, ext2, ext3, ext4, afterY, sigOut);
      computationTime += timer.GetElapsedTime();
    }

    return computationTime;
  }

  // decides the correct extension modes for cA and cD separately,
  // and fill the extensions (2D matrices)
  template <typename ArrayInType, typename ArrayOutType, typename DeviceTag>
  void IDWTHelper2DLeftRight(const ArrayInType& coeffIn,
                             vtkm::Id inDimX,
                             vtkm::Id inDimY,
                             vtkm::Id inStartX,
                             vtkm::Id inStartY,
                             vtkm::Id inPretendDimX,
                             vtkm::Id inPretendDimY,
                             vtkm::Id cADimX,
                             vtkm::Id cDDimX,
                             ArrayOutType& ext1,
                             ArrayOutType& ext2, // output
                             ArrayOutType& ext3,
                             ArrayOutType& ext4, // output
                             vtkm::Id& ext1DimX,
                             vtkm::Id& ext2DimX, // output
                             vtkm::Id& ext3DimX,
                             vtkm::Id& ext4DimX, // output
                             vtkm::Id filterLen,
                             DWTMode mode,
                             DeviceTag)
  {
    VTKM_ASSERT(inPretendDimX == (cADimX + cDDimX));

    // determine extension modes
    DWTMode cALeft, cARight, cDLeft, cDRight;
    cALeft = cARight = cDLeft = cDRight = mode;
    if (mode == SYMH)
    {
      cDLeft = ASYMH;
      if (inPretendDimX % 2 != 0)
      {
        cARight = SYMW;
        cDRight = ASYMW;
      }
      else
      {
        cDRight = ASYMH;
      }
    }
    else // mode == SYMW
    {
      cDLeft = SYMH;
      if (inPretendDimX % 2 != 0)
      {
        cARight = SYMW;
        cDRight = SYMH;
      }
      else
      {
        cARight = SYMH;
      }
    }
    // determine length after extension
    vtkm::Id cAExtendedDimX, cDExtendedDimX;
    vtkm::Id cDPadLen = 0;
    vtkm::Id addLen = filterLen / 4; // addLen == 0 for Haar kernel
    if ((cADimX > cDDimX) && (mode == SYMH))
    {
      cDPadLen = cADimX;
    }
    cAExtendedDimX = cADimX + 2 * addLen;
    cDExtendedDimX = cAExtendedDimX;

    // extend cA
    vtkm::Id cADimY = inPretendDimY;
    this->Extend2D(coeffIn,
                   inDimX,
                   inDimY,
                   inStartX,
                   inStartY,
                   cADimX,
                   cADimY,
                   ext1,
                   ext2,
                   addLen,
                   cALeft,
                   cARight,
                   false,
                   false,
                   true,
                   DeviceTag());

    ext1DimX = ext2DimX = addLen;

    // extend cD
    vtkm::Id cDDimY = inPretendDimY;
    if (cDPadLen > 0)
    {
      this->Extend2D(coeffIn,
                     inDimX,
                     inDimY,
                     inStartX + cADimX,
                     inStartY,
                     cDDimX,
                     cDDimY,
                     ext3,
                     ext4,
                     addLen,
                     cDLeft,
                     cDRight,
                     true,
                     false,
                     true,
                     DeviceTag());
      ext3DimX = addLen;
      ext4DimX = addLen + 1;
    }
    else
    {
      vtkm::Id cDExtendedWouldBe = cDDimX + 2 * addLen;
      if (cDExtendedWouldBe == cDExtendedDimX)
      {
        this->Extend2D(coeffIn,
                       inDimX,
                       inDimY,
                       inStartX + cADimX,
                       inStartY,
                       cDDimX,
                       cDDimY,
                       ext3,
                       ext4,
                       addLen,
                       cDLeft,
                       cDRight,
                       false,
                       false,
                       true,
                       DeviceTag());
        ext3DimX = ext4DimX = addLen;
      }
      else if (cDExtendedWouldBe == cDExtendedDimX - 1)
      {
        this->Extend2D(coeffIn,
                       inDimX,
                       inDimY,
                       inStartX + cADimX,
                       inStartY,
                       cDDimX,
                       cDDimY,
                       ext3,
                       ext4,
                       addLen,
                       cDLeft,
                       cDRight,
                       false,
                       true,
                       true,
                       DeviceTag());
        ext3DimX = addLen;
        ext4DimX = addLen + 1;
      }
      else
      {
        vtkm::cont::ErrorInternal("cDTemp Length not match!");
      }
    }
  }

  // decides the correct extension modes for cA and cD separately,
  // and fill the extensions (2D matrices)
  template <typename ArrayInType, typename ArrayOutType, typename DeviceTag>
  void IDWTHelper2DTopDown(const ArrayInType& coeffIn,
                           vtkm::Id inDimX,
                           vtkm::Id inDimY,
                           vtkm::Id inStartX,
                           vtkm::Id inStartY,
                           vtkm::Id inPretendDimX,
                           vtkm::Id inPretendDimY,
                           vtkm::Id cADimY,
                           vtkm::Id cDDimY,
                           ArrayOutType& ext1,
                           ArrayOutType& ext2, // output
                           ArrayOutType& ext3,
                           ArrayOutType& ext4, // output
                           vtkm::Id& ext1DimY,
                           vtkm::Id& ext2DimY, // output
                           vtkm::Id& ext3DimY,
                           vtkm::Id& ext4DimY, // output
                           vtkm::Id filterLen,
                           DWTMode mode,
                           DeviceTag)
  {
    VTKM_ASSERT(inPretendDimY == (cADimY + cDDimY));

    // determine extension modes
    DWTMode cATopMode, cADownMode, cDTopMode, cDDownMode;
    cATopMode = cADownMode = cDTopMode = cDDownMode = mode;
    if (mode == SYMH)
    {
      cDTopMode = ASYMH;
      if (inPretendDimY % 2 != 0)
      {
        cADownMode = SYMW;
        cDDownMode = ASYMW;
      }
      else
      {
        cDDownMode = ASYMH;
      }
    }
    else // mode == SYMW
    {
      cDTopMode = SYMH;
      if (inPretendDimY % 2 != 0)
      {
        cADownMode = SYMW;
        cDDownMode = SYMH;
      }
      else
      {
        cADownMode = SYMH;
      }
    }
    // determine length after extension
    vtkm::Id cAExtendedDimY, cDExtendedDimY;
    vtkm::Id cDPadLen = 0;
    vtkm::Id addLen = filterLen / 4; // addLen == 0 for Haar kernel
    if ((cADimY > cDDimY) && (mode == SYMH))
      cDPadLen = cADimY;
    cAExtendedDimY = cADimY + 2 * addLen;
    cDExtendedDimY = cAExtendedDimY;

    // extend cA
    vtkm::Id cADimX = inPretendDimX;
    this->Extend2D(coeffIn,
                   inDimX,
                   inDimY,
                   inStartX,
                   inStartY,
                   cADimX,
                   cADimY,
                   ext1,
                   ext2,
                   addLen,
                   cATopMode,
                   cADownMode,
                   false,
                   false,
                   false,
                   DeviceTag());
    ext1DimY = ext2DimY = addLen;

    // extend cD
    vtkm::Id cDDimX = inPretendDimX;
    if (cDPadLen > 0)
    {
      this->Extend2D(coeffIn,
                     inDimX,
                     inDimY,
                     inStartX,
                     inStartY + cADimY,
                     cDDimX,
                     cDDimY,
                     ext3,
                     ext4,
                     addLen,
                     cDTopMode,
                     cDDownMode,
                     true,
                     false,
                     false,
                     DeviceTag());
      ext3DimY = addLen;
      ext4DimY = addLen + 1;
    }
    else
    {
      vtkm::Id cDExtendedWouldBe = cDDimY + 2 * addLen;
      if (cDExtendedWouldBe == cDExtendedDimY)
      {
        this->Extend2D(coeffIn,
                       inDimX,
                       inDimY,
                       inStartX,
                       inStartY + cADimY,
                       cDDimX,
                       cDDimY,
                       ext3,
                       ext4,
                       addLen,
                       cDTopMode,
                       cDDownMode,
                       false,
                       false,
                       false,
                       DeviceTag());
        ext3DimY = ext4DimY = addLen;
      }
      else if (cDExtendedWouldBe == cDExtendedDimY - 1)
      {
        this->Extend2D(coeffIn,
                       inDimX,
                       inDimY,
                       inStartX,
                       inStartY + cADimY,
                       cDDimX,
                       cDDimY,
                       ext3,
                       ext4,
                       addLen,
                       cDTopMode,
                       cDDownMode,
                       false,
                       true,
                       false,
                       DeviceTag());
        ext3DimY = addLen;
        ext4DimY = addLen + 1;
      }
      else
      {
        vtkm::cont::ErrorInternal("cDTemp Length not match!");
      }
    }
  }

  // decides the correct extension modes for cA and cD separately,
  // and fill the extensions (3D cubes)
  template <typename ArrayInType, typename ArrayOutType, typename DeviceTag>
  void IDWTHelper3DLeftRight(const ArrayInType& coeffIn,
                             vtkm::Id inDimX,
                             vtkm::Id inDimY,
                             vtkm::Id inDimZ,
                             vtkm::Id inStartX,
                             vtkm::Id inStartY,
                             vtkm::Id inStartZ,
                             vtkm::Id inPretendDimX,
                             vtkm::Id inPretendDimY,
                             vtkm::Id inPretendDimZ,
                             vtkm::Id cADimX,
                             vtkm::Id cDDimX,
                             ArrayOutType& ext1,
                             ArrayOutType& ext2, // output
                             ArrayOutType& ext3,
                             ArrayOutType& ext4, // output
                             vtkm::Id& ext1DimX,
                             vtkm::Id& ext2DimX, // output
                             vtkm::Id& ext3DimX,
                             vtkm::Id& ext4DimX, // output
                             vtkm::Id filterLen,
                             DWTMode mode,
                             DeviceTag)
  {
    VTKM_ASSERT(inPretendDimX == (cADimX + cDDimX));

    // determine extension modes
    DWTMode cALeftMode, cARightMode, cDLeftMode, cDRightMode;
    cALeftMode = cARightMode = cDLeftMode = cDRightMode = mode;
    if (mode == SYMH)
    {
      cDLeftMode = ASYMH;
      if (inPretendDimX % 2 != 0)
      {
        cARightMode = SYMW;
        cDRightMode = ASYMW;
      }
      else
      {
        cDRightMode = ASYMH;
      }
    }
    else // mode == SYMW
    {
      cDLeftMode = SYMH;
      if (inPretendDimX % 2 != 0)
      {
        cARightMode = SYMW;
        cDRightMode = SYMH;
      }
      else
      {
        cARightMode = SYMH;
      }
    }

    // determine length after extension
    vtkm::Id cAExtendedDimX, cDExtendedDimX;
    vtkm::Id cDPadLen = 0;
    vtkm::Id addLen = filterLen / 4; // addLen == 0 for Haar kernel
    if ((cADimX > cDDimX) && (mode == SYMH))
    {
      cDPadLen = cADimX;
    }
    cAExtendedDimX = cADimX + 2 * addLen;
    cDExtendedDimX = cAExtendedDimX;

    // extend cA
    vtkm::Id cADimY = inPretendDimY;
    vtkm::Id cADimZ = inPretendDimZ;
    this->Extend3DLeftRight(coeffIn,
                            inDimX,
                            inDimY,
                            inDimZ,
                            inStartX,
                            inStartY,
                            inStartZ,
                            cADimX,
                            cADimY,
                            cADimZ,
                            ext1,
                            ext2,
                            addLen,
                            cALeftMode,
                            cARightMode,
                            false,
                            false,
                            DeviceTag());
    ext1DimX = ext2DimX = addLen;

    // extend cD
    vtkm::Id cDDimY = inPretendDimY;
    vtkm::Id cDDimZ = inPretendDimZ;
    bool pretendSigPaddedZero, padZeroAtExt2;
    if (cDPadLen > 0)
    {
      ext3DimX = addLen;
      ext4DimX = addLen + 1;
      pretendSigPaddedZero = true;
      padZeroAtExt2 = false;
    }
    else
    {
      vtkm::Id cDExtendedWouldBe = cDDimX + 2 * addLen;
      if (cDExtendedWouldBe == cDExtendedDimX)
      {
        ext3DimX = ext4DimX = addLen;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = false;
      }
      else if (cDExtendedWouldBe == cDExtendedDimX - 1)
      {
        ext3DimX = addLen;
        ext4DimX = addLen + 1;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = true;
      }
      else
      {
        pretendSigPaddedZero = padZeroAtExt2 = false; // so the compiler doesn't complain
        vtkm::cont::ErrorInternal("cDTemp Length not match!");
      }
    }
    this->Extend3DLeftRight(coeffIn,
                            inDimX,
                            inDimY,
                            inDimZ,
                            inStartX + cADimX,
                            inStartY,
                            inStartZ,
                            cDDimX,
                            cDDimY,
                            cDDimZ,
                            ext3,
                            ext4,
                            addLen,
                            cDLeftMode,
                            cDRightMode,
                            pretendSigPaddedZero,
                            padZeroAtExt2,
                            DeviceTag());
  }

  template <typename ArrayInType, typename ArrayOutType, typename DeviceTag>
  void IDWTHelper3DTopDown(const ArrayInType& coeffIn,
                           vtkm::Id inDimX,
                           vtkm::Id inDimY,
                           vtkm::Id inDimZ,
                           vtkm::Id inStartX,
                           vtkm::Id inStartY,
                           vtkm::Id inStartZ,
                           vtkm::Id inPretendDimX,
                           vtkm::Id inPretendDimY,
                           vtkm::Id inPretendDimZ,
                           vtkm::Id cADimY,
                           vtkm::Id cDDimY,
                           ArrayOutType& ext1,
                           ArrayOutType& ext2, // output
                           ArrayOutType& ext3,
                           ArrayOutType& ext4, // output
                           vtkm::Id& ext1DimY,
                           vtkm::Id& ext2DimY, // output
                           vtkm::Id& ext3DimY,
                           vtkm::Id& ext4DimY, // output
                           vtkm::Id filterLen,
                           DWTMode mode,
                           DeviceTag)
  {
    VTKM_ASSERT(inPretendDimY == (cADimY + cDDimY));

    // determine extension modes
    DWTMode cATopMode, cADownMode, cDTopMode, cDDownMode;
    cATopMode = cADownMode = cDTopMode = cDDownMode = mode;
    if (mode == SYMH)
    {
      cDTopMode = ASYMH;
      if (inPretendDimY % 2 != 0)
      {
        cADownMode = SYMW;
        cDDownMode = ASYMW;
      }
      else
      {
        cDDownMode = ASYMH;
      }
    }
    else // mode == SYMW
    {
      cDTopMode = SYMH;
      if (inPretendDimY % 2 != 0)
      {
        cADownMode = SYMW;
        cDDownMode = SYMH;
      }
      else
      {
        cADownMode = SYMH;
      }
    }

    // determine length after extension
    vtkm::Id cAExtendedDimY, cDExtendedDimY;
    vtkm::Id cDPadLen = 0;
    vtkm::Id addLen = filterLen / 4; // addLen == 0 for Haar kernel
    if ((cADimY > cDDimY) && (mode == SYMH))
    {
      cDPadLen = cADimY;
    }
    cAExtendedDimY = cADimY + 2 * addLen;
    cDExtendedDimY = cAExtendedDimY;

    // extend cA
    vtkm::Id cADimX = inPretendDimX;
    vtkm::Id cADimZ = inPretendDimZ;
    this->Extend3DTopDown(coeffIn,
                          inDimX,
                          inDimY,
                          inDimZ,
                          inStartX,
                          inStartY,
                          inStartZ,
                          cADimX,
                          cADimY,
                          cADimZ,
                          ext1,
                          ext2,
                          addLen,
                          cATopMode,
                          cADownMode,
                          false,
                          false,
                          DeviceTag());
    ext1DimY = ext2DimY = addLen;

    // extend cD
    vtkm::Id cDDimX = inPretendDimX;
    vtkm::Id cDDimZ = inPretendDimZ;
    bool pretendSigPaddedZero, padZeroAtExt2;
    if (cDPadLen > 0)
    {
      ext3DimY = addLen;
      ext4DimY = addLen + 1;
      pretendSigPaddedZero = true;
      padZeroAtExt2 = false;
    }
    else
    {
      vtkm::Id cDExtendedWouldBe = cDDimY + 2 * addLen;
      if (cDExtendedWouldBe == cDExtendedDimY)
      {
        ext3DimY = ext4DimY = addLen;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = false;
      }
      else if (cDExtendedWouldBe == cDExtendedDimY - 1)
      {
        ext3DimY = addLen;
        ext4DimY = addLen + 1;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = true;
      }
      else
      {
        pretendSigPaddedZero = padZeroAtExt2 = false; // so the compiler doesn't complain
        vtkm::cont::ErrorInternal("cDTemp Length not match!");
      }
    }
    this->Extend3DTopDown(coeffIn,
                          inDimX,
                          inDimY,
                          inDimZ,
                          inStartX,
                          inStartY + cADimY,
                          inStartZ,
                          cDDimX,
                          cDDimY,
                          cDDimZ,
                          ext3,
                          ext4,
                          addLen,
                          cDTopMode,
                          cDDownMode,
                          pretendSigPaddedZero,
                          padZeroAtExt2,
                          DeviceTag());
  }

  template <typename ArrayInType, typename ArrayOutType, typename DeviceTag>
  void IDWTHelper3DFrontBack(const ArrayInType& coeffIn,
                             vtkm::Id inDimX,
                             vtkm::Id inDimY,
                             vtkm::Id inDimZ,
                             vtkm::Id inStartX,
                             vtkm::Id inStartY,
                             vtkm::Id inStartZ,
                             vtkm::Id inPretendDimX,
                             vtkm::Id inPretendDimY,
                             vtkm::Id inPretendDimZ,
                             vtkm::Id cADimZ,
                             vtkm::Id cDDimZ,
                             ArrayOutType& ext1,
                             ArrayOutType& ext2, // output
                             ArrayOutType& ext3,
                             ArrayOutType& ext4, // output
                             vtkm::Id& ext1DimZ,
                             vtkm::Id& ext2DimZ, // output
                             vtkm::Id& ext3DimZ,
                             vtkm::Id& ext4DimZ, // output
                             vtkm::Id filterLen,
                             DWTMode mode,
                             DeviceTag)
  {
    VTKM_ASSERT(inPretendDimZ == (cADimZ + cDDimZ));

    // determine extension modes
    DWTMode cAFrontMode, cABackMode, cDFrontMode, cDBackMode;
    cAFrontMode = cABackMode = cDFrontMode = cDBackMode = mode;
    if (mode == SYMH)
    {
      cDFrontMode = ASYMH;
      if (inPretendDimZ % 2 != 0)
      {
        cABackMode = SYMW;
        cDBackMode = ASYMW;
      }
      else
      {
        cDBackMode = ASYMH;
      }
    }
    else // mode == SYMW
    {
      cDFrontMode = SYMH;
      if (inPretendDimZ % 2 != 0)
      {
        cABackMode = SYMW;
        cDBackMode = SYMH;
      }
      else
      {
        cABackMode = SYMH;
      }
    }

    // determine length after extension
    vtkm::Id cAExtendedDimZ, cDExtendedDimZ;
    vtkm::Id cDPadLen = 0;
    vtkm::Id addLen = filterLen / 4; // addLen == 0 for Haar kernel
    if ((cADimZ > cDDimZ) && (mode == SYMH))
    {
      cDPadLen = cADimZ;
    }
    cAExtendedDimZ = cADimZ + 2 * addLen;
    cDExtendedDimZ = cAExtendedDimZ;

    // extend cA
    vtkm::Id cADimX = inPretendDimX;
    vtkm::Id cADimY = inPretendDimY;
    this->Extend3DFrontBack(coeffIn,
                            inDimX,
                            inDimY,
                            inDimZ,
                            inStartX,
                            inStartY,
                            inStartZ,
                            cADimX,
                            cADimY,
                            cADimZ,
                            ext1,
                            ext2,
                            addLen,
                            cAFrontMode,
                            cABackMode,
                            false,
                            false,
                            DeviceTag());
    ext1DimZ = ext2DimZ = addLen;

    // extend cD
    vtkm::Id cDDimX = inPretendDimX;
    vtkm::Id cDDimY = inPretendDimY;
    bool pretendSigPaddedZero, padZeroAtExt2;
    if (cDPadLen > 0)
    {
      ext3DimZ = addLen;
      ext4DimZ = addLen + 1;
      pretendSigPaddedZero = true;
      padZeroAtExt2 = false;
    }
    else
    {
      vtkm::Id cDExtendedWouldBe = cDDimZ + 2 * addLen;
      if (cDExtendedWouldBe == cDExtendedDimZ)
      {
        ext3DimZ = ext4DimZ = addLen;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = false;
      }
      else if (cDExtendedWouldBe == cDExtendedDimZ - 1)
      {
        ext3DimZ = addLen;
        ext4DimZ = addLen + 1;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = true;
      }
      else
      {
        pretendSigPaddedZero = padZeroAtExt2 = false; // so the compiler doesn't complain
        vtkm::cont::ErrorInternal("cDTemp Length not match!");
      }
    }
    this->Extend3DFrontBack(coeffIn,
                            inDimX,
                            inDimY,
                            inDimZ,
                            inStartX,
                            inStartY,
                            inStartZ + cADimZ,
                            cDDimX,
                            cDDimY,
                            cDDimZ,
                            ext3,
                            ext4,
                            addLen,
                            cDFrontMode,
                            cDBackMode,
                            pretendSigPaddedZero,
                            padZeroAtExt2,
                            DeviceTag());
  }
};

} // namespace wavelets
} // namespace worklet
} // namespace vtkm

#endif
