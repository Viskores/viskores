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

#ifndef vtkm_worklet_cosmotools_cosmotools_centerfinder_h
#define vtkm_worklet_cosmotools_cosmotools_centerfinder_h

#include <vtkm/worklet/cosmotools/CosmoTools.h>

#include <vtkm/cont/ArrayGetValues.h>

namespace vtkm
{
namespace worklet
{
namespace cosmotools
{

///////////////////////////////////////////////////////////////////////////////
//
// Center finder for particles in FOF halo using estimations but with exact final answer
// MBP (Most Bound Particle) is particle with the minimum potential energy
//
///////////////////////////////////////////////////////////////////////////////

template <typename T, typename StorageType>
vtkm::Id CosmoTools<T, StorageType>::MBPCenterFinderMxN(T* mxnPotential)
{
  vtkm::cont::ArrayHandle<vtkm::Id> partId;
  vtkm::cont::ArrayHandle<vtkm::Id> binId;

  vtkm::cont::ArrayHandle<vtkm::Id> uniqueBins;
  vtkm::cont::ArrayHandle<vtkm::Id> partPerBin;
  vtkm::cont::ArrayHandle<vtkm::Id> particleOffset;

  vtkm::cont::ArrayHandle<vtkm::Id> binX;
  vtkm::cont::ArrayHandle<vtkm::Id> binY;
  vtkm::cont::ArrayHandle<vtkm::Id> binZ;

  // Bin all particles in the halo into bins of size linking length
  BinParticlesHalo(partId, binId, uniqueBins, partPerBin, particleOffset, binX, binY, binZ);
#ifdef DEBUG_PRINT
  DebugPrint("uniqueBins", uniqueBins);
  DebugPrint("partPerBin", partPerBin);
#endif

  // Compute the estimated potential per bin using 27 contiguous bins
  vtkm::cont::ArrayHandle<T> partPotential;
  MBPCenterFindingByKey(binId, partId, partPotential);

  // Reduce by key to get the estimated minimum potential per bin within 27 neighbors
  vtkm::cont::ArrayHandle<vtkm::Id> tempId;
  vtkm::cont::ArrayHandle<T> minPotential;
  DeviceAlgorithm::ReduceByKey(binId, partPotential, tempId, minPotential, vtkm::Minimum());

  // Reduce by key to get the estimated maximum potential per bin within 27 neighbors
  vtkm::cont::ArrayHandle<T> maxPotential;
  DeviceAlgorithm::ReduceByKey(binId, partPotential, tempId, maxPotential, vtkm::Maximum());
#ifdef DEBUG_PRINT
  DebugPrint("minPotential", minPotential);
  DebugPrint("maxPotential", maxPotential);
#endif

  // Compute potentials estimate for a bin using all other bins
  // Particles in the other bins are located at the closest point to this bin
  vtkm::cont::ArrayHandleIndex uniqueIndex(uniqueBins.GetNumberOfValues());
  vtkm::cont::ArrayHandle<T> bestEstPotential;
  vtkm::cont::ArrayHandle<T> worstEstPotential;

  // Initialize each bin potential with the nxn for that bin
  DeviceAlgorithm::Copy(minPotential, bestEstPotential);
  DeviceAlgorithm::Copy(maxPotential, worstEstPotential);

  // Estimate only across the uniqueBins that contain particles
  ComputePotentialBin<T> computePotentialBin(uniqueBins.GetNumberOfValues(), particleMass, linkLen);
  vtkm::worklet::DispatcherMapField<ComputePotentialBin<T>> computePotentialBinDispatcher(
    computePotentialBin);

  computePotentialBinDispatcher.Invoke(uniqueIndex,        // input
                                       partPerBin,         // input (whole array)
                                       binX,               // input (whole array)
                                       binY,               // input (whole array)
                                       binZ,               // input (whole array)
                                       bestEstPotential,   // input/output
                                       worstEstPotential); // input/output
#ifdef DEBUG_PRINT
  DebugPrint("bestEstPotential", bestEstPotential);
  DebugPrint("worstEstPotential", worstEstPotential);
  std::cout << "Number of bestEstPotential " << bestEstPotential.GetNumberOfValues() << std::endl;
  std::cout << "Number of worstEstPotential " << worstEstPotential.GetNumberOfValues() << std::endl;
#endif

  // Sort everything by the best estimated potential per bin
  vtkm::cont::ArrayHandle<T> tempBest;
  DeviceAlgorithm::Copy(bestEstPotential, tempBest);
  DeviceAlgorithm::SortByKey(tempBest, worstEstPotential);

  // Use the worst estimate for the first selected bin to compare to best of all others
  // Any bin that passes is a candidate for having the MBP
  T cutoffPotential = vtkm::cont::ArrayGetValue(0, worstEstPotential);
  worstEstPotential.ReleaseResources();
  tempBest.ReleaseResources();

  vtkm::cont::ArrayHandle<vtkm::Id> candidate;
  DeviceAlgorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nParticles), candidate);

  SetCandidateParticles<T> setCandidateParticles(cutoffPotential);
  vtkm::worklet::DispatcherMapField<SetCandidateParticles<T>> setCandidateParticlesDispatcher(
    setCandidateParticles);
  setCandidateParticlesDispatcher.Invoke(bestEstPotential, // input
                                         particleOffset,   // input
                                         partPerBin,       // input
                                         candidate);       // output (whole array)

  // Copy the M candidate particles to a new array
  vtkm::cont::ArrayHandle<vtkm::Id> mparticles;
  DeviceAlgorithm::CopyIf(partId, candidate, mparticles);

  // Compute potentials only on the candidate particles
  vtkm::cont::ArrayHandle<T> mpotential;
  ComputePotentialOnCandidates<T> computePotentialOnCandidates(nParticles, particleMass);
  vtkm::worklet::DispatcherMapField<ComputePotentialOnCandidates<T>>
    computePotentialOnCandidatesDispatcher(computePotentialOnCandidates);

  computePotentialOnCandidatesDispatcher.Invoke(mparticles,
                                                xLoc,        // input (whole array)
                                                yLoc,        // input (whole array)
                                                zLoc,        // input (whole array)
                                                mpotential); // output

  // Of the M candidate particles which has the minimum potential
  DeviceAlgorithm::SortByKey(mpotential, mparticles);
#ifdef DEBUG_PRINT
  DebugPrint("mparticles", mparticles);
  DebugPrint("mpotential", mpotential);
#endif

  // Return the found MBP particle and its potential
  vtkm::Id mxnMBP = vtkm::cont::ArrayGetValue(0, mparticles);
  *mxnPotential = vtkm::cont::ArrayGetValue(0, mpotential);

  return mxnMBP;
}

///////////////////////////////////////////////////////////////////////////////
//
// Bin particles in one halo for quick MBP finding
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
void CosmoTools<T, StorageType>::BinParticlesHalo(vtkm::cont::ArrayHandle<vtkm::Id>& partId,
                                                  vtkm::cont::ArrayHandle<vtkm::Id>& binId,
                                                  vtkm::cont::ArrayHandle<vtkm::Id>& uniqueBins,
                                                  vtkm::cont::ArrayHandle<vtkm::Id>& partPerBin,
                                                  vtkm::cont::ArrayHandle<vtkm::Id>& particleOffset,
                                                  vtkm::cont::ArrayHandle<vtkm::Id>& binX,
                                                  vtkm::cont::ArrayHandle<vtkm::Id>& binY,
                                                  vtkm::cont::ArrayHandle<vtkm::Id>& binZ)
{
  // Compute number of bins and ranges for each bin
  vtkm::Vec<T, 2> xRange(vtkm::cont::ArrayGetValue(0, xLoc));
  vtkm::Vec<T, 2> yRange(vtkm::cont::ArrayGetValue(0, yLoc));
  vtkm::Vec<T, 2> zRange(vtkm::cont::ArrayGetValue(0, zLoc));
  xRange = DeviceAlgorithm::Reduce(xLoc, xRange, vtkm::MinAndMax<T>());
  T minX = xRange[0];
  T maxX = xRange[1];
  yRange = DeviceAlgorithm::Reduce(yLoc, yRange, vtkm::MinAndMax<T>());
  T minY = yRange[0];
  T maxY = yRange[1];
  zRange = DeviceAlgorithm::Reduce(zLoc, zRange, vtkm::MinAndMax<T>());
  T minZ = zRange[0];
  T maxZ = zRange[1];

  numBinsX = static_cast<vtkm::Id>(vtkm::Floor((maxX - minX) / linkLen));
  numBinsY = static_cast<vtkm::Id>(vtkm::Floor((maxY - minY) / linkLen));
  numBinsZ = static_cast<vtkm::Id>(vtkm::Floor((maxZ - minZ) / linkLen));

  vtkm::Id maxBins = 1048576;
  numBinsX = std::min(maxBins, numBinsX);
  numBinsY = std::min(maxBins, numBinsY);
  numBinsZ = std::min(maxBins, numBinsZ);

  vtkm::Id minBins = 1;
  numBinsX = std::max(minBins, numBinsX);
  numBinsY = std::max(minBins, numBinsY);
  numBinsZ = std::max(minBins, numBinsZ);

#ifdef DEBUG_PRINT
  std::cout << std::endl
            << "** BinParticlesHalo (" << numBinsX << ", " << numBinsY << ", " << numBinsZ << ") ("
            << minX << ", " << minY << ", " << minZ << ") (" << maxX << ", " << maxY << ", " << maxZ
            << ")" << std::endl;
#endif

  // Compute which bin each particle is in
  ComputeBins<T> computeBins(minX,
                             maxX, // Physical range on domain
                             minY,
                             maxY,
                             minZ,
                             maxZ,
                             numBinsX,
                             numBinsY,
                             numBinsZ); // Size of superimposed mesh
  vtkm::worklet::DispatcherMapField<ComputeBins<T>> computeBinsDispatcher(computeBins);
  computeBinsDispatcher.Invoke(xLoc,   // input
                               yLoc,   // input
                               zLoc,   // input
                               binId); // output

  vtkm::cont::ArrayHandleIndex indexArray(nParticles);
  DeviceAlgorithm::Copy(indexArray, partId);

#ifdef DEBUG_PRINT
  DebugPrint("xLoc", xLoc);
  DebugPrint("yLoc", yLoc);
  DebugPrint("zLoc", zLoc);
  DebugPrint("partId", partId);
  DebugPrint("binId", binId);
#endif

  // Sort the particles by bin
  DeviceAlgorithm::SortByKey(binId, partId);

  // Count the number of particles per bin
  vtkm::cont::ArrayHandleConstant<vtkm::Id> constArray(1, nParticles);
  DeviceAlgorithm::ReduceByKey(binId, constArray, uniqueBins, partPerBin, vtkm::Add());
#ifdef DEBUG_PRINT
  DebugPrint("sorted binId", binId);
  DebugPrint("sorted partId", partId);
  DebugPrint("uniqueBins", uniqueBins);
  DebugPrint("partPerBin", partPerBin);
#endif

  // Calculate the bin indices
  ComputeBinIndices<T> computeBinIndices(numBinsX, numBinsY, numBinsZ);
  vtkm::worklet::DispatcherMapField<ComputeBinIndices<T>> computeBinIndicesDispatcher(
    computeBinIndices);

  computeBinIndicesDispatcher.Invoke(uniqueBins, // input
                                     binX,       // input
                                     binY,       // input
                                     binZ);      // input

  DeviceAlgorithm::ScanExclusive(partPerBin, particleOffset);
}

///////////////////////////////////////////////////////////////////////////////
//
// Center finder for all particles given location, particle id and key id
// Assumed that key and particles are already sorted
// MBP (Most Bound Particle) is particle with the minimum potential energy
// Method uses ScanInclusiveByKey() and ArrayHandleReverse
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
void CosmoTools<T, StorageType>::MBPCenterFindingByKey(vtkm::cont::ArrayHandle<vtkm::Id>& keyId,
                                                       vtkm::cont::ArrayHandle<vtkm::Id>& partId,
                                                       vtkm::cont::ArrayHandle<T>& minPotential)
{
  // Compute starting and ending indices of each key (bin or halo)
  vtkm::cont::ArrayHandleIndex indexArray(nParticles);
  vtkm::cont::ArrayHandle<T> potential;

  vtkm::cont::ArrayHandleReverse<vtkm::cont::ArrayHandle<vtkm::Id>> keyReverse(keyId);
  vtkm::cont::ArrayHandleReverse<vtkm::cont::ArrayHandle<T>> minPotReverse(minPotential);

  // Compute indices of all left neighbor bins per bin not per particle
  vtkm::cont::ArrayHandle<vtkm::Id> leftNeighbor;
  vtkm::cont::ArrayHandle<vtkm::Id> rightNeighbor;
  leftNeighbor.Allocate(NUM_NEIGHBORS * nParticles);
  rightNeighbor.Allocate(NUM_NEIGHBORS * nParticles);

  vtkm::cont::ArrayHandleIndex countArray(nParticles);
  ComputeNeighborBins computeNeighborBins(numBinsX, numBinsY, numBinsZ, NUM_NEIGHBORS);
  vtkm::worklet::DispatcherMapField<ComputeNeighborBins> computeNeighborBinsDispatcher(
    computeNeighborBins);
  computeNeighborBinsDispatcher.Invoke(countArray, keyId, leftNeighbor);

  // Compute indices of all right neighbor bins
  ComputeBinRange computeBinRange(numBinsX);
  vtkm::worklet::DispatcherMapField<ComputeBinRange> computeBinRangeDispatcher(computeBinRange);
  computeBinRangeDispatcher.Invoke(leftNeighbor, rightNeighbor);

  // Convert bin range to particle range within the bins
  DeviceAlgorithm::LowerBounds(keyId, leftNeighbor, leftNeighbor);
  DeviceAlgorithm::UpperBounds(keyId, rightNeighbor, rightNeighbor);
#ifdef DEBUG_PRINT
  DebugPrint("leftNeighbor", leftNeighbor);
  DebugPrint("rightNeighbor", rightNeighbor);
#endif

  // Initialize halo id of each particle to itself
  // Compute potentials on particles in 27 neighbors to find minimum
  ComputePotentialNeighbors<T> computePotentialNeighbors(
    numBinsX, numBinsY, numBinsZ, NUM_NEIGHBORS, particleMass);
  vtkm::worklet::DispatcherMapField<ComputePotentialNeighbors<T>>
    computePotentialNeighborsDispatcher(computePotentialNeighbors);

  computePotentialNeighborsDispatcher.Invoke(indexArray,
                                             keyId,         // input (whole array)
                                             partId,        // input (whole array)
                                             xLoc,          // input (whole array)
                                             yLoc,          // input (whole array)
                                             zLoc,          // input (whole array)
                                             leftNeighbor,  // input (whole array)
                                             rightNeighbor, // input (whole array)
                                             potential);    // output

  // Find minimum potential for all particles in a halo
  DeviceAlgorithm::ScanInclusiveByKey(keyId, potential, minPotential, vtkm::Minimum());
  DeviceAlgorithm::ScanInclusiveByKey(keyReverse, minPotReverse, minPotReverse, vtkm::Minimum());
#ifdef DEBUG_PRINT
  DebugPrint("potential", potential);
  DebugPrint("minPotential", minPotential);
#endif

  // Find the particle id matching the minimum potential
  vtkm::cont::ArrayHandle<vtkm::Id> centerId;
  EqualsMinimumPotential<T> equalsMinimumPotential;
  vtkm::worklet::DispatcherMapField<EqualsMinimumPotential<T>> equalsMinimumPotentialDispatcher(
    equalsMinimumPotential);

  equalsMinimumPotentialDispatcher.Invoke(partId, potential, minPotential, centerId);
}

///////////////////////////////////////////////////////////////////////////////
//
// Center finder for particles in a single halo given location and particle id
// MBP (Most Bound Particle) is particle with the minimum potential energy
// Method uses ScanInclusiveByKey() and ArrayHandleReverse
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
vtkm::Id CosmoTools<T, StorageType>::MBPCenterFinderNxN(T* nxnPotential)
{
  vtkm::cont::ArrayHandle<T> potential;
  vtkm::cont::ArrayHandle<T> minPotential;

  vtkm::cont::ArrayHandleReverse<vtkm::cont::ArrayHandle<T>> minPotReverse(minPotential);

  vtkm::cont::ArrayHandleIndex particleIndex(nParticles);

  // Compute potentials (Worklet)
  ComputePotentialNxN<T> computePotentialHalo(nParticles, particleMass);
  vtkm::worklet::DispatcherMapField<ComputePotentialNxN<T>> computePotentialHaloDispatcher(
    computePotentialHalo);

  computePotentialHaloDispatcher.Invoke(particleIndex, // input
                                        xLoc,          // input (whole array)
                                        yLoc,          // input (whole array)
                                        zLoc,          // input (whole array)
                                        potential);    // output

  // Find minimum potential for all particles in a halo
  DeviceAlgorithm::ScanInclusive(potential, minPotential, vtkm::Minimum());
  DeviceAlgorithm::ScanInclusive(minPotReverse, minPotReverse, vtkm::Minimum());

  // Find the particle id matching the minimum potential
  vtkm::cont::ArrayHandle<vtkm::Id> centerId;
  EqualsMinimumPotential<T> equalsMinimumPotential;
  vtkm::worklet::DispatcherMapField<EqualsMinimumPotential<T>> equalsMinimumPotentialDispatcher(
    equalsMinimumPotential);

  equalsMinimumPotentialDispatcher.Invoke(particleIndex, potential, minPotential, centerId);

  // Fill out entire array with center index
  vtkm::cont::ArrayHandleReverse<vtkm::cont::ArrayHandle<vtkm::Id>> centerIdReverse(centerId);
  DeviceAlgorithm::ScanInclusive(centerId, centerId, vtkm::Maximum());
  DeviceAlgorithm::ScanInclusive(centerIdReverse, centerIdReverse, vtkm::Maximum());

  vtkm::Id nxnMBP = vtkm::cont::ArrayGetValue(0, centerId);
  *nxnPotential = vtkm::cont::ArrayGetValue(nxnMBP, potential);

  return nxnMBP;
}
}
}
}
#endif
