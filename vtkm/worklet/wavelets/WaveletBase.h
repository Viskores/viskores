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

#ifndef vtk_m_worklet_wavelets_waveletbase_h
#define vtk_m_worklet_wavelets_waveletbase_h

#include <vtkm/worklet/wavelets/WaveletFilter.h>
#include <vtkm/worklet/wavelets/WaveletTransforms.h>

#include <vtkm/Math.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm
{
namespace worklet
{

namespace wavelets
{

// Functionalities are similar to MatWaveBase in VAPoR.
class WaveletBase
{
public:
  // Constructor
  WaveletBase(WaveletName name)
    : wname(name)
    , filter(name)
  {
    if (wname == CDF9_7 || wname == BIOR4_4 || wname == CDF5_3 || wname == BIOR2_2)
    {
      this->wmode = SYMW; // Default extension mode, see MatWaveBase.cpp
    }
    else if (wname == HAAR || wname == BIOR1_1 || wname == CDF8_4 || wname == BIOR3_3)
    {
      this->wmode = SYMH;
    }
  }

  // Returns length of approximation coefficients from a decomposition pass.
  vtkm::Id GetApproxLength(vtkm::Id sigInLen)
  {
    if (sigInLen % 2 != 0)
    {
      return ((sigInLen + 1) / 2);
    }
    else
    {
      return ((sigInLen) / 2);
    }
  }

  // Returns length of detail coefficients from a decomposition pass
  vtkm::Id GetDetailLength(vtkm::Id sigInLen)
  {
    if (sigInLen % 2 != 0)
    {
      return ((sigInLen - 1) / 2);
    }
    else
    {
      return ((sigInLen) / 2);
    }
  }

  // Returns length of coefficients generated in a decomposition pass
  vtkm::Id GetCoeffLength(vtkm::Id sigInLen)
  {
    return (GetApproxLength(sigInLen) + GetDetailLength(sigInLen));
  }
  vtkm::Id GetCoeffLength2(vtkm::Id sigInX, vtkm::Id sigInY)
  {
    return (GetCoeffLength(sigInX) * GetCoeffLength(sigInY));
  }
  vtkm::Id GetCoeffLength3(vtkm::Id sigInX, vtkm::Id sigInY, vtkm::Id sigInZ)
  {
    return (GetCoeffLength(sigInX) * GetCoeffLength(sigInY) * GetCoeffLength(sigInZ));
  }

  // Returns maximum wavelet decomposition level
  vtkm::Id GetWaveletMaxLevel(vtkm::Id sigInLen)
  {
    vtkm::Id filterLen = this->filter.GetFilterLength();
    vtkm::Id level;
    this->WaveLengthValidate(sigInLen, filterLen, level);
    return level;
  }

  // perform a device copy. The whole 1st array to a certain start location of the 2nd array
  template <typename ArrayType1, typename ArrayType2, typename DeviceTag>
  void DeviceCopyStartX(const ArrayType1& srcArray,
                        ArrayType2& dstArray,
                        vtkm::Id startIdx,
                        DeviceTag)
  {
    using CopyType = vtkm::worklet::wavelets::CopyWorklet;
    CopyType cp(startIdx);
    vtkm::worklet::DispatcherMapField<CopyType, DeviceTag> dispatcher(cp);
    dispatcher.Invoke(srcArray, dstArray);
  }

  // Assign zero value to a certain location of an array
  template <typename ArrayType, typename DeviceTag>
  void DeviceAssignZero(ArrayType& array, vtkm::Id index, DeviceTag)
  {
    using ZeroWorklet = vtkm::worklet::wavelets::AssignZeroWorklet;
    ZeroWorklet worklet(index);
    vtkm::worklet::DispatcherMapField<ZeroWorklet, DeviceTag> dispatcher(worklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a certain row to a matrix
  template <typename ArrayType, typename DeviceTag>
  void DeviceAssignZero2DRow(ArrayType& array,
                             vtkm::Id dimX,
                             vtkm::Id dimY, // input
                             vtkm::Id rowIdx,
                             DeviceTag)
  {
    using AssignZero2DType = vtkm::worklet::wavelets::AssignZero2DWorklet;
    AssignZero2DType zeroWorklet(dimX, dimY, -1, rowIdx);
    vtkm::worklet::DispatcherMapField<AssignZero2DType, DeviceTag> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a certain column to a matrix
  template <typename ArrayType, typename DeviceTag>
  void DeviceAssignZero2DColumn(ArrayType& array,
                                vtkm::Id dimX,
                                vtkm::Id dimY, // input
                                vtkm::Id colIdx,
                                DeviceTag)
  {
    using AssignZero2DType = vtkm::worklet::wavelets::AssignZero2DWorklet;
    AssignZero2DType zeroWorklet(dimX, dimY, colIdx, -1);
    vtkm::worklet::DispatcherMapField<AssignZero2DType, DeviceTag> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a plane that's perpendicular to the X axis (Left-Right direction)
  template <typename ArrayType, typename DeviceTag>
  void DeviceAssignZero3DPlaneX(ArrayType& array, // input array
                                vtkm::Id dimX,
                                vtkm::Id dimY,
                                vtkm::Id dimZ,  // dims of input
                                vtkm::Id zeroX, // X idx to set zero
                                DeviceTag)
  {
    using AssignZero3DType = vtkm::worklet::wavelets::AssignZero3DWorklet;
    AssignZero3DType zeroWorklet(dimX, dimY, dimZ, zeroX, -1, -1);
    vtkm::worklet::DispatcherMapField<AssignZero3DType, DeviceTag> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a plane that's perpendicular to the Y axis (Top-Down direction)
  template <typename ArrayType, typename DeviceTag>
  void DeviceAssignZero3DPlaneY(ArrayType& array, // input array
                                vtkm::Id dimX,
                                vtkm::Id dimY,
                                vtkm::Id dimZ,  // dims of input
                                vtkm::Id zeroY, // Y idx to set zero
                                DeviceTag)
  {
    using AssignZero3DType = vtkm::worklet::wavelets::AssignZero3DWorklet;
    AssignZero3DType zeroWorklet(dimX, dimY, dimZ, -1, zeroY, -1);
    vtkm::worklet::DispatcherMapField<AssignZero3DType, DeviceTag> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a plane that's perpendicular to the Z axis (Front-Back direction)
  template <typename ArrayType, typename DeviceTag>
  void DeviceAssignZero3DPlaneZ(ArrayType& array, // input array
                                vtkm::Id dimX,
                                vtkm::Id dimY,
                                vtkm::Id dimZ,  // dims of input
                                vtkm::Id zeroZ, // Y idx to set zero
                                DeviceTag)
  {
    using AssignZero3DType = vtkm::worklet::wavelets::AssignZero3DWorklet;
    AssignZero3DType zeroWorklet(dimX, dimY, dimZ, -1, -1, zeroZ);
    vtkm::worklet::DispatcherMapField<AssignZero3DType, DeviceTag> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Sort by the absolute value on device
  struct SortLessAbsFunctor
  {
    template <typename T>
    VTKM_EXEC bool operator()(const T& x, const T& y) const
    {
      return vtkm::Abs(x) < vtkm::Abs(y);
    }
  };
  template <typename ArrayType, typename DeviceTag>
  void DeviceSort(ArrayType& array, DeviceTag)
  {
    vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>::Sort(array, SortLessAbsFunctor());
  }

  // Reduce to the sum of all values on device
  template <typename ArrayType, typename DeviceTag>
  typename ArrayType::ValueType DeviceSum(const ArrayType& array, DeviceTag)
  {
    return vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>::Reduce(
      array, static_cast<typename ArrayType::ValueType>(0.0));
  }

  // Helper functors for finding the max and min of an array
  struct minFunctor
  {
    template <typename FieldType>
    VTKM_EXEC FieldType operator()(const FieldType& x, const FieldType& y) const
    {
      return Min(x, y);
    }
  };
  struct maxFunctor
  {
    template <typename FieldType>
    VTKM_EXEC FieldType operator()(const FieldType& x, const FieldType& y) const
    {
      return vtkm::Max(x, y);
    }
  };

  // Device Min and Max functions
  template <typename ArrayType, typename DeviceTag>
  typename ArrayType::ValueType DeviceMax(const ArrayType& array, DeviceTag)
  {
    typename ArrayType::ValueType initVal = array.GetPortalConstControl().Get(0);
    return vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>::Reduce(array, initVal, maxFunctor());
  }
  template <typename ArrayType, typename DeviceTag>
  typename ArrayType::ValueType DeviceMin(const ArrayType& array, DeviceTag)
  {
    typename ArrayType::ValueType initVal = array.GetPortalConstControl().Get(0);
    return vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>::Reduce(array, initVal, minFunctor());
  }

  // Max absolute value of an array
  struct maxAbsFunctor
  {
    template <typename FieldType>
    VTKM_EXEC FieldType operator()(const FieldType& x, const FieldType& y) const
    {
      return vtkm::Max(vtkm::Abs(x), vtkm::Abs(y));
    }
  };
  template <typename ArrayType, typename DeviceTag>
  typename ArrayType::ValueType DeviceMaxAbs(const ArrayType& array, DeviceTag)
  {
    typename ArrayType::ValueType initVal = array.GetPortalConstControl().Get(0);
    return vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>::Reduce(array, initVal, maxAbsFunctor());
  }

  // Calculate variance of an array
  template <typename ArrayType, typename DeviceTag>
  vtkm::Float64 DeviceCalculateVariance(ArrayType& array, DeviceTag)
  {
    vtkm::Float64 mean = static_cast<vtkm::Float64>(this->DeviceSum(array, DeviceTag())) /
      static_cast<vtkm::Float64>(array.GetNumberOfValues());

    vtkm::cont::ArrayHandle<vtkm::Float64> squaredDeviation;

    // Use a worklet
    using SDWorklet = vtkm::worklet::wavelets::SquaredDeviation;
    SDWorklet sdw(mean);
    vtkm::worklet::DispatcherMapField<SDWorklet, DeviceTag> dispatcher(sdw);
    dispatcher.Invoke(array, squaredDeviation);

    vtkm::Float64 sdMean = this->DeviceSum(squaredDeviation, DeviceTag()) /
      static_cast<vtkm::Float64>(squaredDeviation.GetNumberOfValues());

    return sdMean;
  }

  // Copy a small rectangle to a big rectangle
  template <typename SmallArrayType, typename BigArrayType, typename DeviceTag>
  void DeviceRectangleCopyTo(const SmallArrayType& smallRect,
                             vtkm::Id smallX,
                             vtkm::Id smallY,
                             BigArrayType& bigRect,
                             vtkm::Id bigX,
                             vtkm::Id bigY,
                             vtkm::Id startX,
                             vtkm::Id startY,
                             DeviceTag)
  {
    using CopyToWorklet = vtkm::worklet::wavelets::RectangleCopyTo;
    CopyToWorklet cp(smallX, smallY, bigX, bigY, startX, startY);
    vtkm::worklet::DispatcherMapField<CopyToWorklet, DeviceTag> dispatcher(cp);
    dispatcher.Invoke(smallRect, bigRect);
  }

  // Copy a small cube to a big cube
  template <typename SmallArrayType, typename BigArrayType, typename DeviceTag>
  void DeviceCubeCopyTo(const SmallArrayType& smallCube,
                        vtkm::Id smallX,
                        vtkm::Id smallY,
                        vtkm::Id smallZ,
                        BigArrayType& bigCube,
                        vtkm::Id bigX,
                        vtkm::Id bigY,
                        vtkm::Id bigZ,
                        vtkm::Id startX,
                        vtkm::Id startY,
                        vtkm::Id startZ,
                        DeviceTag)
  {
    using CopyToWorklet = vtkm::worklet::wavelets::CubeCopyTo;
    CopyToWorklet cp(smallX, smallY, smallZ, bigX, bigY, bigZ, startX, startY, startZ);
    vtkm::worklet::DispatcherMapField<CopyToWorklet, DeviceTag> dispatcher(cp);
    dispatcher.Invoke(smallCube, bigCube);
  }

  template <typename ArrayType>
  void Print2DArray(const std::string& str, const ArrayType& arr, vtkm::Id dimX)
  {
    std::cerr << str << std::endl;
    for (vtkm::Id i = 0; i < arr.GetNumberOfValues(); i++)
    {
      std::cerr << arr.GetPortalConstControl().Get(i) << "  ";
      if (i % dimX == dimX - 1)
      {
        std::cerr << std::endl;
      }
    }
  }

protected:
  WaveletName wname;
  DWTMode wmode;
  WaveletFilter filter;

  void WaveLengthValidate(vtkm::Id sigInLen, vtkm::Id filterLength, vtkm::Id& level)
  {
    if (sigInLen < filterLength)
    {
      level = 0;
    }
    else
    {
      level = static_cast<vtkm::Id>(
        vtkm::Floor(1.0 + vtkm::Log2(static_cast<vtkm::Float64>(sigInLen) /
                                     static_cast<vtkm::Float64>(filterLength))));
    }
  }

}; // class WaveletBase.

} // namespace wavelets

} // namespace worklet
} // namespace vtkm

#endif
