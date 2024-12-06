//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_AdvectAlgorithmThreaded_h
#define vtk_m_filter_flow_internal_AdvectAlgorithmThreaded_h

#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/filter/flow/internal/AdvectAlgorithm.h>
#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/internal/DataSetIntegrator.h>

#include <thread>

namespace vtkm
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

  AdvectAlgorithmThreaded(const vtkm::filter::flow::internal::BoundsMap& bm,
                          std::vector<DSIType>& blocks)
    : AdvectAlgorithm<DSIType>(bm, blocks)
    , Done(false)
  {
    //For threaded algorithm, the particles go out of scope in the Work method.
    //When this happens, they are destructed by the time the Manage thread gets them.
    //Set the copy flag so the std::vector is copied into the ArrayHandle
    for (auto& block : this->Blocks)
      block.SetCopySeedFlag(true);
  }

  void Go() override
  {
    this->Terminator.Control(this->HaveWork());
    std::vector<std::thread> workerThreads;
    workerThreads.emplace_back(std::thread(AdvectAlgorithmThreaded::Worker, this));
    this->Manage();

    //This will only work for 1 thread. For > 1, the Blocks will need a mutex.
    VTKM_ASSERT(workerThreads.size() == 1);
    for (auto& t : workerThreads)
      t.join();
  }

protected:
  bool HaveWork() override
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    return this->AdvectAlgorithm<DSIType>::HaveWork() || this->WorkerActivate;
  }

  bool GetActiveParticles(std::vector<ParticleType>& particles, vtkm::Id& blockId) override
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    bool val = this->AdvectAlgorithm<DSIType>::GetActiveParticles(particles, blockId);
    this->WorkerActivate = val;
    return val;
  }

  void UpdateActive(const std::vector<ParticleType>& particles,
                    const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap) override
  {
    if (!particles.empty())
    {
      std::lock_guard<std::mutex> lock(this->Mutex);
      this->AdvectAlgorithm<DSIType>::UpdateActive(particles, idsMap);

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

  static void Worker(AdvectAlgorithmThreaded* algo) { algo->Work(); }

  void WorkerWait()
  {
    std::unique_lock<std::mutex> lock(this->Mutex);
    this->WorkerActivateCondition.wait(lock, [this] { return WorkerActivate || Done; });
  }

  void UpdateWorkerResult(vtkm::Id blockId, DSIHelperInfo<ParticleType>& b)
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    auto& it = this->WorkerResults[blockId];
    it.emplace_back(b);
  }

  void Work()
  {
    while (!this->CheckDone())
    {
      std::vector<ParticleType> v;
      vtkm::Id blockId = -1;
      if (this->GetActiveParticles(v, blockId))
      {
        auto& block = this->GetDataSet(blockId);
        DSIHelperInfo<ParticleType> bb(v, this->BoundsMap, this->ParticleBlockIDsMap);
        block.Advect(bb, this->StepSize);
        this->UpdateWorkerResult(blockId, bb);
      }
      else
        this->WorkerWait();
    }
  }

  void Manage()
  {
    while (!this->Terminator.Done())
    {
      std::unordered_map<vtkm::Id, std::vector<DSIHelperInfo<ParticleType>>> workerResults;
      this->GetWorkerResults(workerResults);

      //bool localWork = !workerResults.empty();
      vtkm::Id numTerm = 0;
      for (auto& it : workerResults)
        for (auto& r : it.second)
          numTerm += this->UpdateResult(r);

      this->ExchangeParticles();
      this->Terminator.Control(this->HaveWork());
    }
    this->SetDone();
  }

  void GetWorkerResults(
    std::unordered_map<vtkm::Id, std::vector<DSIHelperInfo<ParticleType>>>& results)
  {
    results.clear();

    std::lock_guard<std::mutex> lock(this->Mutex);
    if (!this->WorkerResults.empty())
    {
      results = this->WorkerResults;
      this->WorkerResults.clear();
    }
  }

  std::atomic<bool> Done;
  std::mutex Mutex;
  bool WorkerActivate = false;
  std::condition_variable WorkerActivateCondition;
  std::unordered_map<vtkm::Id, std::vector<DSIHelperInfo<ParticleType>>> WorkerResults;
};

}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_AdvectAlgorithmThreaded_h
