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
  vtkm::Id sigX = 99;
  vtkm::Id sigY = 99;
  vtkm::Id sigZ = 99;
  vtkm::Id sigLen = sigX * sigY * sigZ;
  std::cout << "Testing 3D wavelet compressor on a (99x99x99) cube..." << std::endl;

  // make input data array handle
  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray;
  inputArray.Allocate(sigLen);
  FillArray3D(inputArray, sigX, sigY, sigZ);

  vtkm::cont::ArrayHandle<vtkm::Float32> outputArray;

  // Use a WaveletCompressor
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::BIOR4_4;
  if (wname == vtkm::worklet::wavelets::BIOR1_1)
    std::cout << "Using wavelet kernel   = Bior1.1 (HAAR)" << std::endl;
  else if (wname == vtkm::worklet::wavelets::BIOR2_2)
    std::cout << "Using wavelet kernel   = Bior2.2 (CDF 5/3)" << std::endl;
  else if (wname == vtkm::worklet::wavelets::BIOR3_3)
    std::cout << "Using wavelet kernel   = Bior3.3 (CDF 8/4)" << std::endl;
  else if (wname == vtkm::worklet::wavelets::BIOR4_4)
    std::cout << "Using wavelet kernel   = Bior4.4 (CDF 9/7)" << std::endl;
  vtkm::worklet::WaveletCompressor compressor(wname);

  vtkm::Id XMaxLevel = compressor.GetWaveletMaxLevel(sigX);
  vtkm::Id YMaxLevel = compressor.GetWaveletMaxLevel(sigY);
  vtkm::Id ZMaxLevel = compressor.GetWaveletMaxLevel(sigZ);
  vtkm::Id nLevels = vtkm::Min(vtkm::Min(XMaxLevel, YMaxLevel), ZMaxLevel);
  std::cout << "Decomposition levels   = " << nLevels << std::endl;
  vtkm::Float64 computationTime = 0.0;
  vtkm::Float64 elapsedTime1, elapsedTime2, elapsedTime3;

  // Decompose

  vtkm::cont::Timer timer;
  timer.Start();
  computationTime =
    compressor.WaveDecompose3D(inputArray, nLevels, sigX, sigY, sigZ, outputArray, false);
  elapsedTime1 = timer.GetElapsedTime();
  std::cout << "Decompose time         = " << elapsedTime1 << std::endl;
  std::cout << "  ->computation time   = " << computationTime << std::endl;

  // Squash small coefficients
  timer.Start();
  compressor.SquashCoefficients(outputArray, cratio);
  elapsedTime2 = timer.GetElapsedTime();
  std::cout << "Squash time            = " << elapsedTime2 << std::endl;

  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float32> reconstructArray;
  timer.Start();
  computationTime =
    compressor.WaveReconstruct3D(outputArray, nLevels, sigX, sigY, sigZ, reconstructArray, false);
  elapsedTime3 = timer.GetElapsedTime();
  std::cout << "Reconstruction time    = " << elapsedTime3 << std::endl;
  std::cout << "  ->computation time   = " << computationTime << std::endl;
  std::cout << "Total time             = " << (elapsedTime1 + elapsedTime2 + elapsedTime3)
            << std::endl;

  outputArray.ReleaseResources();

  compressor.EvaluateReconstruction(inputArray, reconstructArray);

  timer.Start();
  for (vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(reconstructArray.GetPortalConstControl().Get(i),
                                inputArray.GetPortalConstControl().Get(i)),
                     "WaveletCompressor 3D failed...");
  }
  elapsedTime1 = timer.GetElapsedTime();
  std::cout << "Verification time      = " << elapsedTime1 << std::endl;
}

void TestDecomposeReconstruct2D(vtkm::Float64 cratio)
{
  std::cout << "Testing 2D wavelet compressor on a (1000x1000) square... " << std::endl;
  vtkm::Id sigX = 1000;
  vtkm::Id sigY = 1000;
  vtkm::Id sigLen = sigX * sigY;

  // make input data array handle
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray;
  inputArray.Allocate(sigLen);
  FillArray2D(inputArray, sigX, sigY);

  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;

  // Use a WaveletCompressor
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::CDF9_7;
  std::cout << "Wavelet kernel         = CDF 9/7" << std::endl;
  vtkm::worklet::WaveletCompressor compressor(wname);

  vtkm::Id XMaxLevel = compressor.GetWaveletMaxLevel(sigX);
  vtkm::Id YMaxLevel = compressor.GetWaveletMaxLevel(sigY);
  vtkm::Id nLevels = vtkm::Min(XMaxLevel, YMaxLevel);
  std::cout << "Decomposition levels   = " << nLevels << std::endl;
  std::vector<vtkm::Id> L;
  vtkm::Float64 computationTime = 0.0;
  vtkm::Float64 elapsedTime1, elapsedTime2, elapsedTime3;

  // Decompose
  vtkm::cont::Timer timer;
  timer.Start();
  computationTime = compressor.WaveDecompose2D(inputArray, nLevels, sigX, sigY, outputArray, L);
  elapsedTime1 = timer.GetElapsedTime();
  std::cout << "Decompose time         = " << elapsedTime1 << std::endl;
  std::cout << "  ->computation time   = " << computationTime << std::endl;

  // Squash small coefficients
  timer.Start();
  compressor.SquashCoefficients(outputArray, cratio);
  elapsedTime2 = timer.GetElapsedTime();
  std::cout << "Squash time            = " << elapsedTime2 << std::endl;

  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  timer.Start();
  computationTime =
    compressor.WaveReconstruct2D(outputArray, nLevels, sigX, sigY, reconstructArray, L);
  elapsedTime3 = timer.GetElapsedTime();
  std::cout << "Reconstruction time    = " << elapsedTime3 << std::endl;
  std::cout << "  ->computation time   = " << computationTime << std::endl;
  std::cout << "Total time             = " << (elapsedTime1 + elapsedTime2 + elapsedTime3)
            << std::endl;

  outputArray.ReleaseResources();

  compressor.EvaluateReconstruction(inputArray, reconstructArray);

  timer.Start();
  for (vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(reconstructArray.GetPortalConstControl().Get(i),
                                inputArray.GetPortalConstControl().Get(i)),
                     "WaveletCompressor 2D failed...");
  }
  elapsedTime1 = timer.GetElapsedTime();
  std::cout << "Verification time      = " << elapsedTime1 << std::endl;
}

void TestDecomposeReconstruct1D(vtkm::Float64 cratio)
{
  std::cout << "Testing 1D wavelet compressor on a 1 million sized array... " << std::endl;
  vtkm::Id sigLen = 1000000;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for (vtkm::Id i = 0; i < sigLen; i++)
  {
    tmpVector.push_back(100.0 * vtkm::Sin(static_cast<vtkm::Float64>(i) / 100.0));
  }
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = vtkm::cont::make_ArrayHandle(tmpVector);

  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;

  // Use a WaveletCompressor
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::CDF9_7;
  std::cout << "Wavelet kernel         = CDF 9/7" << std::endl;
  vtkm::worklet::WaveletCompressor compressor(wname);

  // User maximum decompose levels
  vtkm::Id maxLevel = compressor.GetWaveletMaxLevel(sigLen);
  vtkm::Id nLevels = maxLevel;
  std::cout << "Decomposition levels   = " << nLevels << std::endl;

  std::vector<vtkm::Id> L;

  // Decompose
  vtkm::cont::Timer timer;
  timer.Start();
  compressor.WaveDecompose(inputArray, nLevels, outputArray, L);

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();
  std::cout << "Decompose time         = " << elapsedTime << std::endl;

  // Squash small coefficients
  timer.Start();
  compressor.SquashCoefficients(outputArray, cratio);
  elapsedTime = timer.GetElapsedTime();
  std::cout << "Squash time            = " << elapsedTime << std::endl;

  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  timer.Start();
  compressor.WaveReconstruct(outputArray, nLevels, L, reconstructArray);
  elapsedTime = timer.GetElapsedTime();
  std::cout << "Reconstruction time    = " << elapsedTime << std::endl;

  compressor.EvaluateReconstruction(inputArray, reconstructArray);

  timer.Start();
  for (vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(reconstructArray.GetPortalConstControl().Get(i),
                                inputArray.GetPortalConstControl().Get(i)),
                     "WaveletCompressor 1D failed...");
  }
  elapsedTime = timer.GetElapsedTime();
  std::cout << "Verification time      = " << elapsedTime << std::endl;
}

void TestWaveletCompressor()
{
  vtkm::Float64 cratio = 2.0; // X:1 compression, where X >= 1
  std::cout << "Compression ratio       = " << cratio << ":1 ";
  std::cout
    << "(Reconstruction using higher compression ratios may result in failure in verification)"
    << std::endl;

  TestDecomposeReconstruct1D(cratio);
  std::cout << std::endl;
  TestDecomposeReconstruct2D(cratio);
  std::cout << std::endl;
  TestDecomposeReconstruct3D(cratio);
}

int UnitTestWaveletCompressor(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestWaveletCompressor, argc, argv);
}
