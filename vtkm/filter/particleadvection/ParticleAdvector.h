//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ParticleAdvector_h
#define vtk_m_filter_ParticleAdvector_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/ParticleArrayCopy.h>
#include <vtkm/filter/particleadvection/DataSetIntegrator.h>
#include <vtkm/filter/particleadvection/ParticleMessenger.h>

#include <map>
#include <thread>
#include <vector>

//TODO:
// fix inheritance?
// streamlines...

namespace vtkm
{
namespace filter
{
namespace particleadvection
{
using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;
template <typename AlgorithmType>
vtkm::cont::PartitionedDataSet RunAlgo(const vtkm::filter::particleadvection::BoundsMap& boundsMap,
                                       const std::vector<DataSetIntegratorType>& dsi,
                                       vtkm::Id numSteps,
                                       vtkm::FloatDefault stepSize,
                                       const vtkm::cont::ArrayHandle<vtkm::Particle>& seeds)
{
  AlgorithmType algo(boundsMap, dsi);

  algo.SetNumberOfSteps(numSteps);
  algo.SetStepSize(stepSize);
  algo.SetSeeds(seeds);
  algo.Go();
  return algo.GetOutput();
}
//
// Base class for particle advector
//

template <typename ResultType>
class VTKM_ALWAYS_EXPORT ParticleAdvectorBase
{
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

public:
  ParticleAdvectorBase(const vtkm::filter::particleadvection::BoundsMap& bm,
                       const std::vector<DataSetIntegratorType>& blocks)
    : Blocks(blocks)
    , BoundsMap(bm)
    , NumberOfSteps(0)
    , NumRanks(this->Comm.size())
    , Rank(this->Comm.rank())
    , StepSize(0)
  {
  }

  //Initialize ParticleAdvectorBase
  void SetStepSize(vtkm::FloatDefault stepSize) { this->StepSize = stepSize; }
  void SetNumberOfSteps(vtkm::Id numSteps) { this->NumberOfSteps = numSteps; }
  virtual void SetSeeds(const vtkm::cont::ArrayHandle<vtkm::Particle>& seeds) = 0;


  //Virtuals for advection and getting output.
  virtual void Go() = 0;
  virtual vtkm::cont::PartitionedDataSet GetOutput() = 0;

protected:
  inline vtkm::Id ComputeTotalNumParticles(vtkm::Id numLocal) const;
  inline const DataSetIntegratorType& GetDataSet(vtkm::Id id); // const;
  inline void UpdateResultParticle(
    vtkm::Particle& p,
    std::vector<vtkm::Particle>& I,
    std::vector<vtkm::Particle>& T,
    std::vector<vtkm::Particle>& A,
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMapI,
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMapA) const;

  virtual void ClearParticles() = 0;
  virtual void SetSeedArray(const std::vector<vtkm::Particle>& particles,
                            const std::vector<std::vector<vtkm::Id>>& blockIds) = 0;
  virtual void Communicate(vtkm::filter::particleadvection::ParticleMessenger& messenger,
                           vtkm::Id numLocalTerminations,
                           vtkm::Id& numTermMessages) = 0;
  virtual bool GetActiveParticles(std::vector<vtkm::Particle>& particles, vtkm::Id& blockId) = 0;
  virtual void UpdateActive(const std::vector<vtkm::Particle>& particles,
                            const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap) = 0;
  virtual void UpdateInactive(
    const std::vector<vtkm::Particle>& particles,
    const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap) = 0;
  virtual void UpdateTerminated(const std::vector<vtkm::Particle>& particles, vtkm::Id blockId) = 0;

  virtual void StoreResult(const ResultType& vtkmNotUsed(res), vtkm::Id vtkmNotUsed(blockId)) {}
  inline vtkm::Id UpdateResult(const ResultType& res, vtkm::Id blockId);

  //Member data
  std::vector<DataSetIntegratorType> Blocks;
  vtkm::filter::particleadvection::BoundsMap BoundsMap;
  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id NumberOfSteps;
  vtkm::Id NumRanks;
  vtkm::Id Rank;
  vtkm::FloatDefault StepSize;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;
};

//
// Base algorithm for particle advection.
// This is a single-threaded algorithm.
//

template <typename ResultType>
class VTKM_ALWAYS_EXPORT PABaseAlgorithm : public ParticleAdvectorBase<ResultType>
{
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

public:
  PABaseAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                  const std::vector<DataSetIntegratorType>& blocks)
    : ParticleAdvectorBase<ResultType>(bm, blocks)
  {
  }

  void SetSeeds(const vtkm::cont::ArrayHandle<vtkm::Particle>& seeds) override
  {
    this->ClearParticles();

    vtkm::Id n = seeds.GetNumberOfValues();
    auto portal = seeds.ReadPortal();

    std::vector<std::vector<vtkm::Id>> blockIDs;
    std::vector<vtkm::Particle> particles;
    for (vtkm::Id i = 0; i < n; i++)
    {
      const vtkm::Particle p = portal.Get(i);
      std::vector<vtkm::Id> ids = this->BoundsMap.FindBlocks(p.Pos);
      if (!ids.empty() && this->BoundsMap.FindRank(ids[0]) == this->Rank)
      {
        particles.push_back(p);
        blockIDs.push_back(ids);
      }
    }

    this->SetSeedArray(particles, blockIDs);
  }

  void Go() override
  {
    vtkm::filter::particleadvection::ParticleMessenger messenger(
      this->Comm, this->BoundsMap, 1, 128);

    vtkm::Id nLocal = static_cast<vtkm::Id>(this->Active.size() + this->Inactive.size());
    vtkm::Id totalNumSeeds = this->ComputeTotalNumParticles(nLocal);

    vtkm::Id N = 0;
    while (N < totalNumSeeds)
    {
      std::vector<vtkm::Particle> v;
      vtkm::Id numTerm = 0, blockId = -1;
      if (GetActiveParticles(v, blockId))
      {
        const auto& block = this->GetDataSet(blockId);

        ResultType r;
        block.Advect(v, this->StepSize, this->NumberOfSteps, r);
        numTerm = this->UpdateResult(r, blockId);
      }

      vtkm::Id numTermMessages = 0;
      this->Communicate(messenger, numTerm, numTermMessages);

      N += (numTerm + numTermMessages);
      if (N > totalNumSeeds)
        throw vtkm::cont::ErrorFilterExecution("Particle count error");
    }
  }

protected:
  void ClearParticles() override
  {
    this->Active.clear();
    this->Inactive.clear();
    this->Terminated.clear();
    this->ParticleBlockIDsMap.clear();
  }

  void SetSeedArray(const std::vector<vtkm::Particle>& particles,
                    const std::vector<std::vector<vtkm::Id>>& blockIds) override
  {
    VTKM_ASSERT(particles.size() == blockIds.size());

    auto pit = particles.begin();
    auto bit = blockIds.begin();
    while (pit != particles.end() && bit != blockIds.end())
    {
      this->ParticleBlockIDsMap[pit->ID] = *bit;
      pit++;
      bit++;
    }

    this->Active.insert(this->Active.end(), particles.begin(), particles.end());
  }

  bool GetActiveParticles(std::vector<vtkm::Particle>& particles, vtkm::Id& blockId) override
  {
    particles.clear();
    blockId = -1;
    if (this->Active.empty())
      return false;

    blockId = this->ParticleBlockIDsMap[this->Active.front().ID][0];
    auto it = this->Active.begin();
    while (it != this->Active.end())
    {
      auto p = *it;
      if (blockId == this->ParticleBlockIDsMap[p.ID][0])
      {
        particles.push_back(p);
        it = this->Active.erase(it);
      }
      else
        it++;
    }

    return !particles.empty();
  }

  void Communicate(vtkm::filter::particleadvection::ParticleMessenger& messenger,
                   vtkm::Id numLocalTerminations,
                   vtkm::Id& numTermMessages) override
  {

    std::vector<vtkm::Particle> incoming;
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> incomingIDs;
    numTermMessages = 0;
    messenger.Exchange(this->Inactive,
                       this->ParticleBlockIDsMap,
                       numLocalTerminations,
                       incoming,
                       incomingIDs,
                       numTermMessages);
    this->Inactive.clear();
    this->UpdateActive(incoming, incomingIDs);
  }

  void UpdateActive(const std::vector<vtkm::Particle>& particles,
                    const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap) override
  {
    VTKM_ASSERT(particles.size() == idsMap.size());
    if (particles.empty())
      return;

    this->Active.insert(this->Active.end(), particles.begin(), particles.end());
    for (const auto& it : idsMap)
      this->ParticleBlockIDsMap[it.first] = it.second;
  }

  void UpdateInactive(const std::vector<vtkm::Particle>& particles,
                      const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap) override
  {
    VTKM_ASSERT(particles.size() == idsMap.size());
    if (particles.empty())
      return;

    this->Inactive.insert(this->Inactive.end(), particles.begin(), particles.end());
    for (const auto& it : idsMap)
      this->ParticleBlockIDsMap[it.first] = it.second;
  }

  void UpdateTerminated(const std::vector<vtkm::Particle>& particles, vtkm::Id blockId) override
  {
    if (particles.empty())
      return;

    for (const auto& t : particles)
      this->ParticleBlockIDsMap.erase(t.ID);
    auto& it = this->Terminated[blockId];
    it.insert(it.end(), particles.begin(), particles.end());
  }

  std::vector<vtkm::Particle> Active;
  std::vector<vtkm::Particle> Inactive;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Particle>> Terminated;
};


template <typename ResultType>
class VTKM_ALWAYS_EXPORT PABaseThreadedAlgorithm : public PABaseAlgorithm<ResultType>
{
public:
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

  PABaseThreadedAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                          const std::vector<DataSetIntegratorType>& blocks)
    : PABaseAlgorithm<ResultType>(bm, blocks)
    , Done(false)
  {
    //For threaded algorithm, the particles go out of scope in the Work method.
    //When this happens, they are destructed by the time the Manage thread gets them.
    for (auto& block : this->Blocks)
      block.SetCopySeedFlag(true);
  }

  void Go() override
  {
    vtkm::Id nLocal = static_cast<vtkm::Id>(this->Active.size() + this->Inactive.size());
    vtkm::Id totalNumSeeds = this->ComputeTotalNumParticles(nLocal);

    std::vector<std::thread> workerThreads;
    workerThreads.push_back(std::thread(PABaseThreadedAlgorithm::Worker, this));
    this->Manage(totalNumSeeds);
    for (auto& t : workerThreads)
      t.join();
  }

protected:
  bool GetActiveParticles(std::vector<vtkm::Particle>& particles, vtkm::Id& blockId) override
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    return this->PABaseAlgorithm<ResultType>::GetActiveParticles(particles, blockId);
  }

  void UpdateActive(const std::vector<vtkm::Particle>& particles,
                    const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap) override
  {
    if (!particles.empty())
    {
      std::lock_guard<std::mutex> lock(this->Mutex);
      this->PABaseAlgorithm<ResultType>::UpdateActive(particles, idsMap);
    }
  }

  bool CheckDone()
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    return this->Done;
  }
  void SetDone()
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    this->Done = true;
  }

  static void Worker(PABaseThreadedAlgorithm* algo) { algo->Work(); }

  void Work()
  {
    while (!this->CheckDone())
    {
      std::vector<vtkm::Particle> v;
      vtkm::Id blockId = -1;
      if (this->GetActiveParticles(v, blockId))
      {
        const auto& block = this->GetDataSet(blockId);

        ResultType r;
        block.Advect(v, this->StepSize, this->NumberOfSteps, r);
        this->UpdateWorkerResult(blockId, r);
      }
    }
  }

  void Manage(vtkm::Id totalNumSeeds)
  {
    vtkm::filter::particleadvection::ParticleMessenger messenger(
      this->Comm, this->BoundsMap, 1, 128);

    vtkm::Id N = 0;
    while (N < totalNumSeeds)
    {
      std::unordered_map<vtkm::Id, std::vector<ResultType>> workerResults;
      this->GetWorkerResults(workerResults);

      vtkm::Id numTerm = 0;
      for (const auto& it : workerResults)
      {
        vtkm::Id blockId = it.first;
        const auto& results = it.second;
        for (const auto& r : results)
          numTerm += this->UpdateResult(r, blockId);
      }

      vtkm::Id numTermMessages = 0;
      this->Communicate(messenger, numTerm, numTermMessages);

      N += (numTerm + numTermMessages);
      if (N > totalNumSeeds)
        throw vtkm::cont::ErrorFilterExecution("Particle count error");
    }

    //Let the workers know that we are done.
    this->SetDone();
  }

  void GetWorkerResults(std::unordered_map<vtkm::Id, std::vector<ResultType>>& results)
  {
    results.clear();

    std::lock_guard<std::mutex> lock(this->Mutex);
    if (!this->WorkerResults.empty())
    {
      results = this->WorkerResults;
      this->WorkerResults.clear();
    }
  }

  void UpdateWorkerResult(vtkm::Id blockId, const ResultType& result)
  {
    std::lock_guard<std::mutex> lock(this->Mutex);

    auto& it = this->WorkerResults[blockId];
    it.push_back(result);
  }

  bool Done;
  std::mutex Mutex;
  std::unordered_map<vtkm::Id, std::vector<ResultType>> WorkerResults;
};


class VTKM_ALWAYS_EXPORT ParticleAdvectionAlgorithm
  : public PABaseAlgorithm<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>
{
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

public:
  ParticleAdvectionAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                             const std::vector<DataSetIntegratorType>& blocks)
    : PABaseAlgorithm<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>(bm, blocks)
  {
  }

  vtkm::cont::PartitionedDataSet GetOutput() override
  {
    vtkm::cont::PartitionedDataSet output;

    for (const auto& it : this->Terminated)
    {
      if (it.second.empty())
        continue;

      auto particles = vtkm::cont::make_ArrayHandle(it.second, vtkm::CopyFlag::Off);
      vtkm::cont::ArrayHandle<vtkm::Vec3f> pos;
      vtkm::cont::ParticleArrayCopy(particles, pos);

      vtkm::cont::DataSet ds;
      vtkm::cont::CoordinateSystem outCoords("coordinates", pos);
      ds.AddCoordinateSystem(outCoords);

      //Create vertex cell set
      vtkm::Id numPoints = pos.GetNumberOfValues();
      vtkm::cont::CellSetSingleType<> cells;
      vtkm::cont::ArrayHandleIndex conn(numPoints);
      vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

      vtkm::cont::ArrayCopy(conn, connectivity);
      cells.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, connectivity);
      ds.SetCellSet(cells);

      output.AppendPartition(ds);
    }

    return output;
  }

protected:
};

class VTKM_ALWAYS_EXPORT ParticleAdvectionThreadedAlgorithm
  : public PABaseThreadedAlgorithm<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>
{
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

public:
  ParticleAdvectionThreadedAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                                     const std::vector<DataSetIntegratorType>& blocks)
    : PABaseThreadedAlgorithm<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>>(bm, blocks)
  {
  }

  vtkm::cont::PartitionedDataSet GetOutput() override
  {
    vtkm::cont::PartitionedDataSet output;

    for (const auto& it : this->Terminated)
    {
      if (it.second.empty())
        continue;

      auto particles = vtkm::cont::make_ArrayHandle(it.second, vtkm::CopyFlag::Off);
      vtkm::cont::ArrayHandle<vtkm::Vec3f> pos;
      vtkm::cont::ParticleArrayCopy(particles, pos);

      vtkm::cont::DataSet ds;
      vtkm::cont::CoordinateSystem outCoords("coordinates", pos);
      ds.AddCoordinateSystem(outCoords);

      //Create vertex cell set
      vtkm::Id numPoints = pos.GetNumberOfValues();
      vtkm::cont::CellSetSingleType<> cells;
      vtkm::cont::ArrayHandleIndex conn(numPoints);
      vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

      vtkm::cont::ArrayCopy(conn, connectivity);
      cells.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, connectivity);
      ds.SetCellSet(cells);

      output.AppendPartition(ds);
    }

    return output;
  }

protected:
};

class VTKM_ALWAYS_EXPORT StreamlineAlgorithm
  : public PABaseAlgorithm<vtkm::worklet::StreamlineResult<vtkm::Particle>>
{
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

public:
  StreamlineAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                      const std::vector<DataSetIntegratorType>& blocks)
    : PABaseAlgorithm<vtkm::worklet::StreamlineResult<vtkm::Particle>>(bm, blocks)
  {
  }

  //DAVE dave
  vtkm::cont::PartitionedDataSet GetOutput() override
  {
    vtkm::cont::PartitionedDataSet output;

    for (const auto& it : this->Results)
    {
      std::size_t nResults = it.second.size();
      if (nResults == 0)
        continue;

      vtkm::cont::DataSet ds;
      //Easy case with one result.
      if (nResults == 1)
      {
        ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", it.second[0].Positions));
        ds.SetCellSet(it.second[0].PolyLines);
      }
      else
      {
        //Append all the results into one data set.
        vtkm::cont::ArrayHandle<vtkm::Vec3f> appendPts;
        std::vector<vtkm::Id> posOffsets(nResults);

        const auto& res0 = it.second[0];
        vtkm::Id totalNumCells = res0.PolyLines.GetNumberOfCells();
        vtkm::Id totalNumPts = res0.Positions.GetNumberOfValues();

        posOffsets[0] = 0;
        for (std::size_t i = 1; i < nResults; i++)
        {
          const auto& res = it.second[i];
          posOffsets[i] = totalNumPts;
          totalNumPts += res.Positions.GetNumberOfValues();
          totalNumCells += res.PolyLines.GetNumberOfCells();
        }

        //Append all the points together.
        appendPts.Allocate(totalNumPts);
        for (std::size_t i = 0; i < nResults; i++)
        {
          const auto& res = it.second[i];
          // copy all values into appendPts starting at offset.
          vtkm::cont::Algorithm::CopySubRange(
            res.Positions, 0, res.Positions.GetNumberOfValues(), appendPts, posOffsets[i]);
        }
        vtkm::cont::CoordinateSystem outputCoords =
          vtkm::cont::CoordinateSystem("coordinates", appendPts);
        ds.AddCoordinateSystem(outputCoords);

        //Create polylines.
        std::vector<vtkm::Id> numPtsPerCell(static_cast<std::size_t>(totalNumCells));
        std::size_t off = 0;
        for (std::size_t i = 0; i < nResults; i++)
        {
          const auto& res = it.second[i];
          vtkm::Id nCells = res.PolyLines.GetNumberOfCells();
          for (vtkm::Id j = 0; j < nCells; j++)
            numPtsPerCell[off++] = static_cast<vtkm::Id>(res.PolyLines.GetNumberOfPointsInCell(j));
        }

        auto numPointsPerCellArray =
          vtkm::cont::make_ArrayHandle(numPtsPerCell, vtkm::CopyFlag::Off);

        vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
        vtkm::Id connectivityLen =
          vtkm::cont::Algorithm::ScanExclusive(numPointsPerCellArray, cellIndex);
        vtkm::cont::ArrayHandleIndex connCount(connectivityLen);
        vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
        vtkm::cont::ArrayCopy(connCount, connectivity);

        vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
        auto polyLineShape = vtkm::cont::make_ArrayHandleConstant<vtkm::UInt8>(
          vtkm::CELL_SHAPE_POLY_LINE, totalNumCells);
        vtkm::cont::ArrayCopy(polyLineShape, cellTypes);
        auto offsets = vtkm::cont::ConvertNumIndicesToOffsets(numPointsPerCellArray);

        vtkm::cont::CellSetExplicit<> polyLines;
        polyLines.Fill(totalNumPts, cellTypes, connectivity, offsets);
        ds.SetCellSet(polyLines);
      }
      output.AppendPartition(ds);
    }

    return output;
  }

protected:
  virtual void StoreResult(const vtkm::worklet::StreamlineResult<vtkm::Particle>& res,
                           vtkm::Id blockId) override
  {
    this->Results[blockId].push_back(res);
  }

  std::map<vtkm::Id, std::vector<vtkm::worklet::StreamlineResult<vtkm::Particle>>> Results;
};

class VTKM_ALWAYS_EXPORT StreamlineThreadedAlgorithm
  : public PABaseThreadedAlgorithm<vtkm::worklet::StreamlineResult<vtkm::Particle>>
{
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

public:
  StreamlineThreadedAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                              const std::vector<DataSetIntegratorType>& blocks)
    : PABaseThreadedAlgorithm<vtkm::worklet::StreamlineResult<vtkm::Particle>>(bm, blocks)
  {
  }

  vtkm::cont::PartitionedDataSet GetOutput() override
  {
    vtkm::cont::PartitionedDataSet output;

    for (const auto& it : this->Results)
    {
      std::size_t nResults = it.second.size();
      if (nResults == 0)
        continue;

      vtkm::cont::DataSet ds;
      //Easy case with one result.
      if (nResults == 1)
      {
        ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", it.second[0].Positions));
        ds.SetCellSet(it.second[0].PolyLines);
      }
      else
      {
        //Append all the results into one data set.
        vtkm::cont::ArrayHandle<vtkm::Vec3f> appendPts;
        std::vector<vtkm::Id> posOffsets(nResults);

        const auto& res0 = it.second[0];
        vtkm::Id totalNumCells = res0.PolyLines.GetNumberOfCells();
        vtkm::Id totalNumPts = res0.Positions.GetNumberOfValues();

        posOffsets[0] = 0;
        for (std::size_t i = 1; i < nResults; i++)
        {
          const auto& res = it.second[i];
          posOffsets[i] = totalNumPts;
          totalNumPts += res.Positions.GetNumberOfValues();
          totalNumCells += res.PolyLines.GetNumberOfCells();
        }

        //Append all the points together.
        appendPts.Allocate(totalNumPts);
        for (std::size_t i = 0; i < nResults; i++)
        {
          const auto& res = it.second[i];
          // copy all values into appendPts starting at offset.
          vtkm::cont::Algorithm::CopySubRange(
            res.Positions, 0, res.Positions.GetNumberOfValues(), appendPts, posOffsets[i]);
        }
        vtkm::cont::CoordinateSystem outputCoords =
          vtkm::cont::CoordinateSystem("coordinates", appendPts);
        ds.AddCoordinateSystem(outputCoords);

        //Create polylines.
        std::vector<vtkm::Id> numPtsPerCell(static_cast<std::size_t>(totalNumCells));
        std::size_t off = 0;
        for (std::size_t i = 0; i < nResults; i++)
        {
          const auto& res = it.second[i];
          vtkm::Id nCells = res.PolyLines.GetNumberOfCells();
          for (vtkm::Id j = 0; j < nCells; j++)
            numPtsPerCell[off++] = static_cast<vtkm::Id>(res.PolyLines.GetNumberOfPointsInCell(j));
        }

        auto numPointsPerCellArray =
          vtkm::cont::make_ArrayHandle(numPtsPerCell, vtkm::CopyFlag::Off);

        vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
        vtkm::Id connectivityLen =
          vtkm::cont::Algorithm::ScanExclusive(numPointsPerCellArray, cellIndex);
        vtkm::cont::ArrayHandleIndex connCount(connectivityLen);
        vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
        vtkm::cont::ArrayCopy(connCount, connectivity);

        vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
        auto polyLineShape = vtkm::cont::make_ArrayHandleConstant<vtkm::UInt8>(
          vtkm::CELL_SHAPE_POLY_LINE, totalNumCells);
        vtkm::cont::ArrayCopy(polyLineShape, cellTypes);
        auto offsets = vtkm::cont::ConvertNumIndicesToOffsets(numPointsPerCellArray);

        vtkm::cont::CellSetExplicit<> polyLines;
        polyLines.Fill(totalNumPts, cellTypes, connectivity, offsets);
        ds.SetCellSet(polyLines);
      }

      output.AppendPartition(ds);
    }

    return output;
  }

protected:
  virtual void StoreResult(const vtkm::worklet::StreamlineResult<vtkm::Particle>& res,
                           vtkm::Id blockId) override
  {
    this->Results[blockId].push_back(res);
  }

  std::map<vtkm::Id, std::vector<vtkm::worklet::StreamlineResult<vtkm::Particle>>> Results;
};


}
}
} // namespace vtkm::filter::particleadvection


#ifndef vtk_m_filter_ParticleAdvector_hxx
#include <vtkm/filter/particleadvection/ParticleAdvector.hxx>
#endif

#endif
