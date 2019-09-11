//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

#ifndef vtkm_worklet_cosmotools_cosmotools_h
#define vtkm_worklet_cosmotools_cosmotools_h

#include <vtkm/worklet/cosmotools/ComputeBinIndices.h>
#include <vtkm/worklet/cosmotools/ComputeBinRange.h>
#include <vtkm/worklet/cosmotools/ComputeBins.h>
#include <vtkm/worklet/cosmotools/ComputeNeighborBins.h>
#include <vtkm/worklet/cosmotools/GraftParticles.h>
#include <vtkm/worklet/cosmotools/IsStar.h>
#include <vtkm/worklet/cosmotools/MarkActiveNeighbors.h>
#include <vtkm/worklet/cosmotools/PointerJump.h>
#include <vtkm/worklet/cosmotools/ValidHalo.h>

#include <vtkm/worklet/cosmotools/ComputePotential.h>
#include <vtkm/worklet/cosmotools/ComputePotentialBin.h>
#include <vtkm/worklet/cosmotools/ComputePotentialMxN.h>
#include <vtkm/worklet/cosmotools/ComputePotentialNeighbors.h>
#include <vtkm/worklet/cosmotools/ComputePotentialNxN.h>
#include <vtkm/worklet/cosmotools/ComputePotentialOnCandidates.h>
#include <vtkm/worklet/cosmotools/EqualsMinimumPotential.h>
#include <vtkm/worklet/cosmotools/SetCandidateParticles.h>

#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleReverse.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/ScatterCounting.h>

#include <vtkm/BinaryPredicates.h>
#include <vtkm/Math.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

//#define DEBUG_PRINT 1

namespace
{

///////////////////////////////////////////////////////////////////////////////
//
// Debug prints
//
///////////////////////////////////////////////////////////////////////////////
template <typename U>
void DebugPrint(const char* msg, vtkm::cont::ArrayHandle<U>& array)
{
  vtkm::Id count = 20;
  count = std::min(count, array.GetNumberOfValues());
  std::cout << std::setw(15) << msg << ": ";
  for (vtkm::Id i = 0; i < count; i++)
    std::cout << std::setprecision(3) << std::setw(5) << array.GetPortalConstControl().Get(i)
              << " ";
  std::cout << std::endl;
}

template <typename U>
void DebugPrint(const char* msg, vtkm::cont::ArrayHandleReverse<vtkm::cont::ArrayHandle<U>>& array)
{
  vtkm::Id count = 20;
  count = std::min(count, array.GetNumberOfValues());
  std::cout << std::setw(15) << msg << ": ";
  for (vtkm::Id i = 0; i < count; i++)
    std::cout << std::setw(5) << array.GetPortalConstControl().Get(i) << " ";
  std::cout << std::endl;
}
}

namespace vtkm
{
namespace worklet
{
namespace cosmotools
{


///////////////////////////////////////////////////////////////////////////////
//
// Scatter the result of a reduced array
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct ScatterWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inIndices, FieldOut outIndices);
  using ExecutionSignature = void(_1, _2);
  using ScatterType = vtkm::worklet::ScatterCounting;

  VTKM_EXEC
  void operator()(T inputIndex, T& outputIndex) const { outputIndex = inputIndex; }
};

///////////////////////////////////////////////////////////////////////////////
//
// Scale or offset values of an array
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct ScaleBiasFunctor
{
  T Scale;
  T Bias;

  VTKM_CONT
  ScaleBiasFunctor(T scale = T(1), T bias = T(0))
    : Scale(scale)
    , Bias(bias)
  {
  }

  VTKM_EXEC_CONT
  T operator()(T value) const { return (Scale * value + Bias); }
};

template <typename T, typename StorageType>
class CosmoTools
{
public:
  using DeviceAlgorithm = vtkm::cont::Algorithm;
  const vtkm::Id NUM_NEIGHBORS = 9;

  // geometry of domain
  const vtkm::Id nParticles;
  const T particleMass;
  const vtkm::Id minPartPerHalo;
  const T linkLen;
  vtkm::Id numBinsX;
  vtkm::Id numBinsY;
  vtkm::Id numBinsZ;

  // particle locations within domain
  using LocationType = typename vtkm::cont::ArrayHandle<T, StorageType>;
  LocationType& xLoc;
  LocationType& yLoc;
  LocationType& zLoc;

  // cosmo tools constructor for all particles
  CosmoTools(const vtkm::Id NParticles,                  // Number of particles
             const T mass,                               // Particle mass for potential
             const vtkm::Id pmin,                        // Minimum particles per halo
             const T bb,                                 // Linking length between particles
             vtkm::cont::ArrayHandle<T, StorageType>& X, // Physical location of each particle
             vtkm::cont::ArrayHandle<T, StorageType>& Y,
             vtkm::cont::ArrayHandle<T, StorageType>& Z);

  // cosmo tools constructor for particles in one halo
  CosmoTools(const vtkm::Id NParticles,                  // Number of particles
             const T mass,                               // Particle mass for potential
             vtkm::cont::ArrayHandle<T, StorageType>& X, // Physical location of each particle
             vtkm::cont::ArrayHandle<T, StorageType>& Y,
             vtkm::cont::ArrayHandle<T, StorageType>& Z);
  ~CosmoTools() {}

  // Halo finding and center finding on halos
  void HaloFinder(vtkm::cont::ArrayHandle<vtkm::Id>& resultHaloId,
                  vtkm::cont::ArrayHandle<vtkm::Id>& resultMBP,
                  vtkm::cont::ArrayHandle<T>& resultPot);
  void BinParticlesAll(vtkm::cont::ArrayHandle<vtkm::Id>& partId,
                       vtkm::cont::ArrayHandle<vtkm::Id>& binId,
                       vtkm::cont::ArrayHandle<vtkm::Id>& leftNeighbor,
                       vtkm::cont::ArrayHandle<vtkm::Id>& rightNeighbor);
  void MBPCenterFindingByHalo(vtkm::cont::ArrayHandle<vtkm::Id>& partId,
                              vtkm::cont::ArrayHandle<vtkm::Id>& haloId,
                              vtkm::cont::ArrayHandle<vtkm::Id>& mbpId,
                              vtkm::cont::ArrayHandle<T>& minPotential);

  // MBP Center finding on single halo using NxN algorithm
  vtkm::Id MBPCenterFinderNxN(T* nxnPotential);

  // MBP Center finding on single halo using MxN estimation
  vtkm::Id MBPCenterFinderMxN(T* mxnPotential);

  void BinParticlesHalo(vtkm::cont::ArrayHandle<vtkm::Id>& partId,
                        vtkm::cont::ArrayHandle<vtkm::Id>& binId,
                        vtkm::cont::ArrayHandle<vtkm::Id>& uniqueBins,
                        vtkm::cont::ArrayHandle<vtkm::Id>& partPerBin,
                        vtkm::cont::ArrayHandle<vtkm::Id>& particleOffset,
                        vtkm::cont::ArrayHandle<vtkm::Id>& binX,
                        vtkm::cont::ArrayHandle<vtkm::Id>& binY,
                        vtkm::cont::ArrayHandle<vtkm::Id>& binZ);
  void MBPCenterFindingByKey(vtkm::cont::ArrayHandle<vtkm::Id>& keyId,
                             vtkm::cont::ArrayHandle<vtkm::Id>& partId,
                             vtkm::cont::ArrayHandle<T>& minPotential);
};

///////////////////////////////////////////////////////////////////////////////
//
// Constructor for all particles in the system
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
CosmoTools<T, StorageType>::CosmoTools(const vtkm::Id NParticles,
                                       const T mass,
                                       const vtkm::Id pmin,
                                       const T bb,
                                       vtkm::cont::ArrayHandle<T, StorageType>& X,
                                       vtkm::cont::ArrayHandle<T, StorageType>& Y,
                                       vtkm::cont::ArrayHandle<T, StorageType>& Z)
  : nParticles(NParticles)
  , particleMass(mass)
  , minPartPerHalo(pmin)
  , linkLen(bb)
  , xLoc(X)
  , yLoc(Y)
  , zLoc(Z)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// Constructor for particles in a single halo
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
CosmoTools<T, StorageType>::CosmoTools(const vtkm::Id NParticles,
                                       const T mass,
                                       vtkm::cont::ArrayHandle<T, StorageType>& X,
                                       vtkm::cont::ArrayHandle<T, StorageType>& Y,
                                       vtkm::cont::ArrayHandle<T, StorageType>& Z)
  : nParticles(NParticles)
  , particleMass(mass)
  , minPartPerHalo(10)
  , linkLen(0.2f)
  , xLoc(X)
  , yLoc(Y)
  , zLoc(Z)
{
}
}
}
}
#endif
