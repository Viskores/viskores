//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/WaveletCompressor.h>

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/Testing.h>

#include <iomanip>
#include <vector>

namespace vtkm
{
namespace worklet
{
namespace wavelets
{

class GaussianWorklet2D : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  VTKM_EXEC
  GaussianWorklet2D(vtkm::Id dx,
                    vtkm::Id dy,
                    vtkm::Float64 a,
                    vtkm::Float64 x,
                    vtkm::Float64 y,
                    vtkm::Float64 sx,
                    vtkm::Float64 xy)
    : dimX(dx)
    , amp(a)
    , x0(x)
    , y0(y)
    , sigmaX(sx)
    , sigmaY(xy)
  {
    (void)dy;
    sigmaX2 = 2 * sigmaX * sigmaX;
    sigmaY2 = 2 * sigmaY * sigmaY;
  }

  VTKM_EXEC
  void Sig1Dto2D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y) const
  {
    x = idx % dimX;
    y = idx / dimX;
  }

  VTKM_EXEC
  vtkm::Float64 GetGaussian(vtkm::Float64 x, vtkm::Float64 y) const
  {
    vtkm::Float64 power = (x - x0) * (x - x0) / sigmaX2 + (y - y0) * (y - y0) / sigmaY2;
    return vtkm::Exp(power * -1.0) * amp;
  }

  template <typename T>
  VTKM_EXEC void operator()(T& val, const vtkm::Id& workIdx) const
  {
    vtkm::Id x, y;
    Sig1Dto2D(workIdx, x, y);
    val = GetGaussian(static_cast<vtkm::Float64>(x), static_cast<vtkm::Float64>(y));
  }

private:                              // see wikipedia page
  const vtkm::Id dimX;                // 2D extent
  const vtkm::Float64 amp;            // amplitude
  const vtkm::Float64 x0, y0;         // center
  const vtkm::Float64 sigmaX, sigmaY; // spread
  vtkm::Float64 sigmaX2, sigmaY2;     // 2 * sigma * sigma
};

template <typename T>
class GaussianWorklet3D : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  VTKM_EXEC
  GaussianWorklet3D(vtkm::Id dx, vtkm::Id dy, vtkm::Id dz)
    : dimX(dx)
    , dimY(dy)
    , dimZ(dz)
  {
    amp = (T)20.0;
    sigmaX = (T)dimX / (T)4.0;
    sigmaX2 = sigmaX * sigmaX * (T)2.0;
    sigmaY = (T)dimY / (T)4.0;
    sigmaY2 = sigmaY * sigmaY * (T)2.0;
    sigmaZ = (T)dimZ / (T)4.0;
    sigmaZ2 = sigmaZ * sigmaZ * (T)2.0;
  }

  VTKM_EXEC
  void Sig1Dto3D(vtkm::Id idx, vtkm::Id& x, vtkm::Id& y, vtkm::Id& z) const
  {
    z = idx / (dimX * dimY);
    y = (idx - z * dimX * dimY) / dimX;
    x = idx % dimX;
  }

  VTKM_EXEC
  T GetGaussian(T x, T y, T z) const
  {
    x -= (T)dimX / (T)2.0; // translate to center at (0, 0, 0)
    y -= (T)dimY / (T)2.0;
    z -= (T)dimZ / (T)2.0;
    T power = x * x / sigmaX2 + y * y / sigmaY2 + z * z / sigmaZ2;

    return vtkm::Exp(power * (T)-1.0) * amp;
  }

  VTKM_EXEC
  void operator()(T& val, const vtkm::Id& workIdx) const
  {
    vtkm::Id x, y, z;
    Sig1Dto3D(workIdx, x, y, z);
    val = GetGaussian((T)x, (T)y, (T)z);
  }

private:
  const vtkm::Id dimX, dimY, dimZ; // extent
  T amp;                           // amplitude
  T sigmaX, sigmaY, sigmaZ;        // spread
  T sigmaX2, sigmaY2, sigmaZ2;     // sigma * sigma * 2
};
}
}
}

template <typename ArrayType>
void FillArray2D(ArrayType& array, vtkm::Id dimX, vtkm::Id dimY)
{
  using WorkletType = vtkm::worklet::wavelets::GaussianWorklet2D;
  WorkletType worklet(dimX,
                      dimY,
                      100.0,
                      static_cast<vtkm::Float64>(dimX) / 2.0,  // center
                      static_cast<vtkm::Float64>(dimY) / 2.0,  // center
                      static_cast<vtkm::Float64>(dimX) / 4.0,  // spread
                      static_cast<vtkm::Float64>(dimY) / 4.0); // spread
  vtkm::worklet::DispatcherMapField<WorkletType> dispatcher(worklet);
  dispatcher.Invoke(array);
}
template <typename ArrayType>
void FillArray3D(ArrayType& array, vtkm::Id dimX, vtkm::Id dimY, vtkm::Id dimZ)
{
  using WorkletType = vtkm::worklet::wavelets::GaussianWorklet3D<typename ArrayType::ValueType>;
  WorkletType worklet(dimX, dimY, dimZ);
  vtkm::worklet::DispatcherMapField<WorkletType> dispatcher(worklet);
  dispatcher.Invoke(array);
}

void TestDecomposeReconstruct3D(vtkm::Float64 cratio)
{
  vtkm::Id sigX = 45;
  vtkm::Id sigY = 45;
  vtkm::Id sigZ = 45;
  vtkm::Id sigLen = sigX * sigY * sigZ;

  // make input data array handle
  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray;
  inputArray.Allocate(sigLen);
  FillArray3D(inputArray, sigX, sigY, sigZ);

  vtkm::cont::ArrayHandle<vtkm::Float32> outputArray;

  // Use a WaveletCompressor
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::BIOR4_4;
  vtkm::worklet::WaveletCompressor compressor(wname);

  vtkm::Id XMaxLevel = compressor.GetWaveletMaxLevel(sigX);
  vtkm::Id YMaxLevel = compressor.GetWaveletMaxLevel(sigY);
  vtkm::Id ZMaxLevel = compressor.GetWaveletMaxLevel(sigZ);
  vtkm::Id nLevels = vtkm::Min(vtkm::Min(XMaxLevel, YMaxLevel), ZMaxLevel);

  // Decompose
  compressor.WaveDecompose3D(inputArray, nLevels, sigX, sigY, sigZ, outputArray, false);

  compressor.SquashCoefficients(outputArray, cratio);

  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float32> reconstructArray;
  compressor.WaveReconstruct3D(outputArray, nLevels, sigX, sigY, sigZ, reconstructArray, false);
  outputArray.ReleaseResources();

  //compressor.EvaluateReconstruction(inputArray, reconstructArray);

  auto reconstructPortal = reconstructArray.ReadPortal();
  auto inputPortal = inputArray.ReadPortal();
  for (vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(reconstructPortal.Get(i), inputPortal.Get(i)),
                     "WaveletCompressor 3D failed...");
  }
}

void TestDecomposeReconstruct2D(vtkm::Float64 cratio)
{
  vtkm::Id sigX = 150;
  vtkm::Id sigY = 150;
  vtkm::Id sigLen = sigX * sigY;

  // make input data array handle
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray;
  inputArray.Allocate(sigLen);
  FillArray2D(inputArray, sigX, sigY);

  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;

  // Use a WaveletCompressor
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::CDF9_7;
  vtkm::worklet::WaveletCompressor compressor(wname);

  vtkm::Id XMaxLevel = compressor.GetWaveletMaxLevel(sigX);
  vtkm::Id YMaxLevel = compressor.GetWaveletMaxLevel(sigY);
  vtkm::Id nLevels = vtkm::Min(XMaxLevel, YMaxLevel);
  std::vector<vtkm::Id> L;
  compressor.WaveDecompose2D(inputArray, nLevels, sigX, sigY, outputArray, L);
  compressor.SquashCoefficients(outputArray, cratio);

  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  compressor.WaveReconstruct2D(outputArray, nLevels, sigX, sigY, reconstructArray, L);
  outputArray.ReleaseResources();

  //compressor.EvaluateReconstruction(inputArray, reconstructArray);

  auto reconstructPortal = reconstructArray.ReadPortal();
  auto inputPortal = inputArray.ReadPortal();
  for (vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(reconstructPortal.Get(i), inputPortal.Get(i)),
                     "WaveletCompressor 2D failed...");
  }
}

void TestDecomposeReconstruct1D(vtkm::Float64 cratio)
{
  vtkm::Id sigLen = 1000;

  // make input data array handle
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray;
  inputArray.Allocate(sigLen);
  auto wp = inputArray.WritePortal();
  for (vtkm::Id i = 0; i < sigLen; i++)
  {
    wp.Set(i, 100.0 * vtkm::Sin(static_cast<vtkm::Float64>(i) / 100.0));
  }
  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;

  // Use a WaveletCompressor
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::CDF9_7;
  vtkm::worklet::WaveletCompressor compressor(wname);

  // User maximum decompose levels
  vtkm::Id maxLevel = compressor.GetWaveletMaxLevel(sigLen);
  vtkm::Id nLevels = maxLevel;

  std::vector<vtkm::Id> L;

  // Decompose
  compressor.WaveDecompose(inputArray, nLevels, outputArray, L);

  // Squash small coefficients
  compressor.SquashCoefficients(outputArray, cratio);

  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  compressor.WaveReconstruct(outputArray, nLevels, L, reconstructArray);

  //compressor.EvaluateReconstruction(inputArray, reconstructArray);
  auto reconstructPortal = reconstructArray.ReadPortal();
  auto inputPortal = inputArray.ReadPortal();
  for (vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(reconstructPortal.Get(i), inputPortal.Get(i)),
                     "WaveletCompressor 1D failed...");
  }
}

void TestWaveletCompressor()
{
  vtkm::Float64 cratio = 2.0; // X:1 compression, where X >= 1
  TestDecomposeReconstruct1D(cratio);
  TestDecomposeReconstruct2D(cratio);
  TestDecomposeReconstruct3D(cratio);
}

int UnitTestWaveletCompressor(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestWaveletCompressor, argc, argv);
}
