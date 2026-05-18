//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_filter_flow_internal_AdvectAlgorithmThreaded_h
#define viskores_filter_flow_internal_AdvectAlgorithmThreaded_h

#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/filter/flow/internal/AdvectAlgorithm.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/internal/DataSetIntegrator.h>

#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename DSIType>
class AdvectAlgorithmThreaded : public AdvectAlgorithm<DSIType>
{
public:
  using ParticleType = typename DSIType::PType;
  using ParticleArray = typename AdvectAlgorithm<DSIType>::ParticleArray;
  using ActiveParticleChunk = typename AdvectAlgorithm<DSIType>::ActiveParticleChunk;

  AdvectAlgorithmThreaded(const viskores::filter::flow::internal::BoundsMap& bm,
                          std::vector<DSIType>& blocks)
    : AdvectAlgorithm<DSIType>(bm, blocks)
  {
    // Keep worker seed arrays independent while the manager thread consumes results.
    for (auto& block : this->Blocks)
      block.SetCopySeedFlag(true);
  }

  void Go() override
  {
    std::vector<std::thread> workerThreads;
    workerThreads.emplace_back(std::thread(AdvectAlgorithmThreaded::Worker, this));
    try
    {
      this->Manage();
    }
    catch (...)
    {
      this->SetDone();
      //This will only work for 1 thread. For > 1, the Blocks will need a mutex.
      VISKORES_ASSERT(workerThreads.size() == 1);
      for (auto& t : workerThreads)
        t.join();
      throw;
    }

    //This will only work for 1 thread. For > 1, the Blocks will need a mutex.
    VISKORES_ASSERT(workerThreads.size() == 1);
    for (auto& t : workerThreads)
      t.join();
    this->RethrowWorkerException();
  }

protected:
  bool HaveWork() override
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    return this->CheckHaveWork();
  }

  virtual bool GetDone() override
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
#ifndef VISKORES_ENABLE_MPI
    return this->Done || !this->CheckHaveWork();
#else
    return this->Done || this->Terminator.Done();
#endif
  }

  bool WorkerGetDone()
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    return this->Done;
  }

  bool GetActiveParticles(ActiveParticleChunk& chunk, viskores::Id& blockId) override
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    bool val = this->AdvectAlgorithm<DSIType>::GetActiveParticles(chunk, blockId);
    this->WorkerRunning = val;
    return val;
  }

  void UpdateActive(const std::vector<ParticleType>& particles,
                    const std::vector<viskores::Id>& blockIds,
                    const std::vector<std::vector<viskores::Id>>& candidateBlockIds) override
  {
    if (!particles.empty())
    {
      std::lock_guard<std::mutex> lock(this->Mutex);
      this->AdvectAlgorithm<DSIType>::UpdateActive(particles, blockIds, candidateBlockIds);

      // Let workers know that MPI exchange produced active work.
      this->WorkerActivateCondition.notify_all();
    }
  }

  void SetDone()
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    this->Done = true;
    this->WorkerActivateCondition.notify_all();
  }

  static void Worker(AdvectAlgorithmThreaded* algo) { algo->Work(); }

  void WorkerWait()
  {
    std::unique_lock<std::mutex> lock(this->Mutex);
    this->WorkerActivateCondition.wait(
      lock,
      [this] { return this->Done || !this->Active.empty() || this->WorkerException != nullptr; });
  }

  void UpdateWorkerResult(viskores::Id blockId, DSIHelperInfo<ParticleType>&& b)
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    auto& it = this->WorkerResults[blockId];
    it.emplace_back(std::move(b));
    // The result is now queued for the manager, so termination checks must look
    // at WorkerResults rather than treating the worker as still active.
    this->WorkerRunning = false;
  }

  void Work()
  {
    try
    {
      while (!this->WorkerGetDone())
      {
        ActiveParticleChunk chunk;
        viskores::Id blockId = -1;
        if (this->GetActiveParticles(chunk, blockId))
        {
          auto& block = this->GetDataSet(blockId);
          DSIHelperInfo<ParticleType> bb(
            std::move(chunk.Particles), std::move(chunk.CandidateBlockIDs), this->BoundsMap);
          block.Advect(bb, this->StepSize);
          this->UpdateWorkerResult(blockId, std::move(bb));
        }
        else
          this->WorkerWait();
      }
    }
    catch (...)
    {
      this->SetWorkerException(std::current_exception());
    }
  }

  void Manage()
  {
    while (!this->GetDone())
    {
      std::unordered_map<viskores::Id, std::vector<DSIHelperInfo<ParticleType>>> workerResults;
      this->GetWorkerResults(workerResults);

      viskores::Id numTerm = 0;
      for (auto& it : workerResults)
        for (auto& r : it.second)
          numTerm += this->UpdateResult(r);

      this->ExchangeParticlesThreaded();
      this->RethrowWorkerException();
    }
    this->SetDone();
    this->RethrowWorkerException();
  }

  void GetWorkerResults(
    std::unordered_map<viskores::Id, std::vector<DSIHelperInfo<ParticleType>>>& results)
  {
    results.clear();

    std::lock_guard<std::mutex> lock(this->Mutex);
    if (!this->WorkerResults.empty())
    {
      results = std::move(this->WorkerResults);
      this->WorkerResults.clear();
    }
  }

  void ExchangeParticlesThreaded()
  {
#ifndef VISKORES_ENABLE_MPI
    this->SerialExchangeThreaded();
#else
    if (this->NumRanks == 1)
      this->SerialExchangeThreaded();
    else
      this->ExchangeParticles();
#endif
  }

  void SerialExchangeThreaded()
  {
    bool addedActiveWork = false;
    {
      std::lock_guard<std::mutex> lock(this->Mutex);
      for (const auto& pending : this->Inactive)
      {
        if (pending.Particles.GetNumberOfValues() > 0)
        {
          this->AddActiveParticlesByBlock(
            pending.Particles, pending.BlockIDs, pending.CandidateBlockIDs);
          addedActiveWork = true;
        }
      }
      this->Inactive.clear();
    }

    if (addedActiveWork)
      this->WorkerActivateCondition.notify_all();
  }

  void SetWorkerException(std::exception_ptr exception)
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    this->WorkerException = exception;
    this->WorkerRunning = false;
    this->Done = true;
    this->WorkerActivateCondition.notify_all();
  }

  void RethrowWorkerException()
  {
    std::exception_ptr exception;
    {
      std::lock_guard<std::mutex> lock(this->Mutex);
      exception = this->WorkerException;
    }
    if (exception)
      std::rethrow_exception(exception);
  }

private:
  bool CheckHaveWork()
  {
    // WorkerResults is work owned by the manager. Without this check the
    // manager can exit after the worker queues a result but before it is drained.
    return this->AdvectAlgorithm<DSIType>::HaveWork() || this->WorkerRunning ||
      !this->WorkerResults.empty();
  }

  bool Done = false;
  std::mutex Mutex;
  // True only while the worker owns a chunk and is outside the mutex in Advect.
  // It is not used as a wakeup flag; Active and Done drive the condition wait.
  bool WorkerRunning = false;
  std::exception_ptr WorkerException;
  std::condition_variable WorkerActivateCondition;
  std::unordered_map<viskores::Id, std::vector<DSIHelperInfo<ParticleType>>> WorkerResults;
};

}
}
}
} //viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_AdvectAlgorithmThreaded_h
