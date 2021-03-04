//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particleadvection_AdvectorBaseThreadedAlgorithm_h
#define vtk_m_filter_particleadvection_AdvectorBaseThreadedAlgorithm_h

#include <vtkm/filter/particleadvection/AdvectorBaseAlgorithm.h>

#include <thread>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

template <typename DataSetIntegratorType, typename ResultType>
class VTKM_ALWAYS_EXPORT AdvectorBaseThreadedAlgorithm
  : public AdvectorBaseAlgorithm<DataSetIntegratorType, ResultType>
{
public:
  AdvectorBaseThreadedAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                                const std::vector<DataSetIntegratorType>& blocks)
    : AdvectorBaseAlgorithm<DataSetIntegratorType, ResultType>(bm, blocks)
    , Done(false)
    , WorkerActivate(false)
  {
    //For threaded algorithm, the particles go out of scope in the Work method.
    //When this happens, they are destructed by the time the Manage thread gets them.
    //Set the copy flag so the std::vector is copied into the ArrayHandle
    for (auto& block : this->Blocks)
      block.SetCopySeedFlag(true);
  }

  void Go() override
  {
    vtkm::Id nLocal = static_cast<vtkm::Id>(this->Active.size() + this->Inactive.size());
    this->ComputeTotalNumParticles(nLocal);

    std::vector<std::thread> workerThreads;
    workerThreads.push_back(std::thread(AdvectorBaseThreadedAlgorithm::Worker, this));
    this->Manage();
    for (auto& t : workerThreads)
      t.join();
  }

protected:
  bool GetActiveParticles(std::vector<vtkm::Particle>& particles, vtkm::Id& blockId) override
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    bool val = this->AdvectorBaseAlgorithm<DataSetIntegratorType, ResultType>::GetActiveParticles(
      particles, blockId);
    this->WorkerActivate = val;
    return val;
  }

  void UpdateActive(const std::vector<vtkm::Particle>& particles,
                    const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap) override
  {
    if (!particles.empty())
    {
      std::lock_guard<std::mutex> lock(this->Mutex);
      this->AdvectorBaseAlgorithm<DataSetIntegratorType, ResultType>::UpdateActive(particles,
                                                                                   idsMap);

      //Let workers know there is new work
      this->WorkerActivateCondition.notify_all();
      this->WorkerActivate = true;
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
    this->WorkerActivateCondition.notify_all();
  }

  static void Worker(AdvectorBaseThreadedAlgorithm* algo) { algo->Work(); }

  void WorkerWait()
  {
    std::unique_lock<std::mutex> lock(this->Mutex);
    this->WorkerActivateCondition.wait(lock, [this] { return WorkerActivate || Done; });
  }

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
      else
        this->WorkerWait();
    }
  }

  void Manage()
  {
    vtkm::filter::particleadvection::ParticleMessenger messenger(
      this->Comm, this->BoundsMap, 1, 128);

    while (this->TotalNumTerminatedParticles < this->TotalNumParticles)
    {
      std::unordered_map<vtkm::Id, std::vector<ResultType>> workerResults;
      this->GetWorkerResults(workerResults);

      vtkm::Id numTerm = 0;
      for (auto& it : workerResults)
      {
        vtkm::Id blockId = it.first;
        for (auto& r : it.second)
          numTerm += this->UpdateResult(r, blockId);
      }

      vtkm::Id numTermMessages = 0;
      this->Communicate(messenger, numTerm, numTermMessages);

      this->TotalNumTerminatedParticles += (numTerm + numTermMessages);
      if (this->TotalNumTerminatedParticles > this->TotalNumParticles)
        throw vtkm::cont::ErrorFilterExecution("Particle count error");
    }

    //Let the workers know that we are done.
    this->SetDone();
  }

  bool GetBlockAndWait(const vtkm::Id& numLocalTerm) override
  {
    std::lock_guard<std::mutex> lock(this->Mutex);

    return (this->AdvectorBaseAlgorithm<DataSetIntegratorType, ResultType>::GetBlockAndWait(
              numLocalTerm) &&
            !this->WorkerActivate && this->WorkerResults.empty());
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

  std::atomic<bool> Done;
  std::mutex Mutex;
  bool WorkerActivate;
  std::condition_variable WorkerActivateCondition;
  std::unordered_map<vtkm::Id, std::vector<ResultType>> WorkerResults;
};

}
}
} // namespace vtkm::filter::particleadvection

#endif
