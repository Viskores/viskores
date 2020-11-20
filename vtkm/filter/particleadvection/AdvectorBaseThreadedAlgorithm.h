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

template <typename ResultType>
class VTKM_ALWAYS_EXPORT AdvectorBaseThreadedAlgorithm : public AdvectorBaseAlgorithm<ResultType>
{
public:
  using DataSetIntegratorType = vtkm::filter::particleadvection::DataSetIntegrator;

  AdvectorBaseThreadedAlgorithm(const vtkm::filter::particleadvection::BoundsMap& bm,
                                const std::vector<DataSetIntegratorType>& blocks)
    : AdvectorBaseAlgorithm<ResultType>(bm, blocks)
    , Done(false)
    , WorkerIdle(true)
  {
    //For threaded algorithm, the particles go out of scope in the Work method.
    //When this happens, they are destructed by the time the Manage thread gets them.
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
    return this->AdvectorBaseAlgorithm<ResultType>::GetActiveParticles(particles, blockId);
  }

  void UpdateActive(const std::vector<vtkm::Particle>& particles,
                    const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap) override
  {
    if (!particles.empty())
    {
      std::lock_guard<std::mutex> lock(this->Mutex);
      this->AdvectorBaseAlgorithm<ResultType>::UpdateActive(particles, idsMap);
      //      this->WorkAvailableCondition.notify_all();
    }
  }

  bool CheckDone() const { return this->Done; }
  void SetDone() { this->Done = true; }

  static void Worker(AdvectorBaseThreadedAlgorithm* algo) { algo->Work(); }

  void WorkerWait()
  {
    this->WorkerIdle = true;

    //    std::cout<<"Worker wait..."<<std::endl;
    //    std::unique_lock<std::mutex> lock(this->WorkAvailMutex);
    //    this->WorkAvailableCondition.wait(lock);
  }

  void Work()
  {
    while (!this->CheckDone())
    {
      std::vector<vtkm::Particle> v;
      vtkm::Id blockId = -1;
      if (this->GetActiveParticles(v, blockId))
      {
        this->WorkerIdle = false;
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
      //      if (this->Rank == 0) std::cout<<" M: "<<this->TotalNumTerminatedParticles<<" "<<this->TotalNumParticles<<std::endl;

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
      //this->WorkAvailableCondition.notify_all();

      this->TotalNumTerminatedParticles += (numTerm + numTermMessages);
      if (this->TotalNumTerminatedParticles > this->TotalNumParticles)
        throw vtkm::cont::ErrorFilterExecution("Particle count error");

      //      if (numTerm + numTermMessages > 0)
      //        this->WorkAvailableCondition.notify_all();
      //      this->WorkAvailableCondition.notify_all();
    }

    //Let the workers know that we are done.
    std::cout << this->Rank << " DONE" << std::endl;
    this->SetDone();
    //    this->WorkAvailableCondition.notify_all();
  }

  bool GetBlockAndWait(const vtkm::Id& numLocalTerm) override
  {
    return false;
    /*
    std::lock_guard<std::mutex> lock(this->Mutex);
    bool val = this->AdvectorBaseAlgorithm<ResultType>::GetBlockAndWait(numLocalTerm);
    if (val && this->WorkerIdle)
        val = true;
    else
        val = false;
    if (this->Rank == 0) std::cout<<" M: GBW: val= "<<val<<" ((wi= "<<this->WorkerIdle<<" Asz= "<<this->Active.size()<<std::endl;

    return val;
      */
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

  std::mutex WorkAvailMutex;
  std::condition_variable WorkAvailableCondition;
  std::atomic<bool> Done;
  std::mutex Mutex;
  std::unordered_map<vtkm::Id, std::vector<ResultType>> WorkerResults;
  std::atomic<bool> WorkerIdle;
};

}
}
} // namespace vtkm::filter::particleadvection

#endif
