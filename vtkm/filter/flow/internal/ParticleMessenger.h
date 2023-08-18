//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_ParticleMessenger_h
#define vtk_m_filter_flow_internal_ParticleMessenger_h

#include <vtkm/Particle.h>
#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/internal/Messenger.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

#include <list>
#include <map>
#include <set>
#include <vector>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename ParticleType>
class VTKM_FILTER_FLOW_EXPORT ParticleMessenger : public vtkm::filter::flow::internal::Messenger
{
  //sendRank, message
  using MsgCommType = std::pair<int, std::vector<int>>;

  //particle + blockIDs.
  using ParticleCommType = std::pair<ParticleType, std::vector<vtkm::Id>>;

  //sendRank, vector of ParticleCommType.
  using ParticleRecvCommType = std::pair<int, std::vector<ParticleCommType>>;

public:
  VTKM_CONT ParticleMessenger(vtkmdiy::mpi::communicator& comm,
                              bool useAsyncComm,
                              const vtkm::filter::flow::internal::BoundsMap& bm,
                              int msgSz = 1,
                              int numParticles = 128,
                              int numBlockIds = 2);
  VTKM_CONT ~ParticleMessenger() {}

  VTKM_CONT void Exchange(const std::vector<ParticleType>& outData,
                          const std::vector<vtkm::Id>& outRanks,
                          const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
                          vtkm::Id numLocalTerm,
                          std::vector<ParticleType>& inData,
                          std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
                          vtkm::Id& numTerminateMessages,
                          bool blockAndWait = false);

protected:
#ifdef VTKM_ENABLE_MPI
  static constexpr int MSG_TERMINATE = 1;

  enum { MESSAGE_TAG = 0x42000, PARTICLE_TAG = 0x42001 };

  VTKM_CONT void RegisterMessages(int msgSz, int nParticles, int numBlockIds);

  // Send/Recv particles
  VTKM_CONT
  template <typename P,
            template <typename, typename>
            class Container,
            typename Allocator = std::allocator<P>>
  inline void SendParticles(int dst, const Container<P, Allocator>& c);

  VTKM_CONT
  template <typename P,
            template <typename, typename>
            class Container,
            typename Allocator = std::allocator<P>>
  inline void SendParticles(const std::unordered_map<int, Container<P, Allocator>>& m);

  // Send/Recv messages.
  VTKM_CONT void SendMsg(int dst, const std::vector<int>& msg);
  VTKM_CONT void SendAllMsg(const std::vector<int>& msg);
  VTKM_CONT bool RecvMsg(std::vector<MsgCommType>& msgs) { return RecvAny(&msgs, NULL, false); }

  // Send/Recv datasets.
  VTKM_CONT bool RecvAny(std::vector<MsgCommType>* msgs,
                         std::vector<ParticleRecvCommType>* recvParticles,
                         bool blockAndWait);
  const vtkm::filter::flow::internal::BoundsMap& BoundsMap;

#endif

  VTKM_CONT void SerialExchange(
    const std::vector<ParticleType>& outData,
    const std::vector<vtkm::Id>& outRanks,
    const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
    vtkm::Id numLocalTerm,
    std::vector<ParticleType>& inData,
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
    bool blockAndWait) const;

  static std::size_t CalcParticleBufferSize(std::size_t nParticles, std::size_t numBlockIds = 2);
};

//methods

VTKM_CONT
template <typename ParticleType>
ParticleMessenger<ParticleType>::ParticleMessenger(
  vtkmdiy::mpi::communicator& comm,
  bool useAsyncComm,
  const vtkm::filter::flow::internal::BoundsMap& boundsMap,
  int msgSz,
  int numParticles,
  int numBlockIds)
  : Messenger(comm, useAsyncComm)
#ifdef VTKM_ENABLE_MPI
  , BoundsMap(boundsMap)
#endif
{
#ifdef VTKM_ENABLE_MPI
  this->RegisterMessages(msgSz, numParticles, numBlockIds);
#else
  (void)(boundsMap);
  (void)(msgSz);
  (void)(numParticles);
  (void)(numBlockIds);
#endif
}

template <typename ParticleType>
std::size_t ParticleMessenger<ParticleType>::CalcParticleBufferSize(std::size_t nParticles,
                                                                    std::size_t nBlockIds)
{
  ParticleType pTmp;
  std::size_t pSize = ParticleType::Sizeof();

#ifndef NDEBUG
  vtkmdiy::MemoryBuffer buff;
  ParticleType p;
  vtkmdiy::save(buff, p);

  //Make sure the buffer size is correct.
  //If this fires, then the size of the class has changed.
  VTKM_ASSERT(pSize == buff.size());
#endif

  return
    // rank
    sizeof(int)
    //std::vector<ParticleType> p;
    //p.size()
    + sizeof(std::size_t)
    //nParticles of ParticleType
    + nParticles * pSize
    // std::vector<vtkm::Id> blockIDs for each particle.
    // blockIDs.size() for each particle
    + nParticles * sizeof(std::size_t)
    // nBlockIDs of vtkm::Id for each particle.
    + nParticles * nBlockIds * sizeof(vtkm::Id);
}

VTKM_CONT
template <typename ParticleType>
void ParticleMessenger<ParticleType>::SerialExchange(
  const std::vector<ParticleType>& outData,
  const std::vector<vtkm::Id>& vtkmNotUsed(outRanks),
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
  vtkm::Id vtkmNotUsed(numLocalTerm),
  std::vector<ParticleType>& inData,
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
  bool vtkmNotUsed(blockAndWait)) const
{
  for (auto& p : outData)
  {
    const auto& bids = outBlockIDsMap.find(p.GetID())->second;
    inData.emplace_back(p);
    inDataBlockIDsMap[p.GetID()] = bids;
  }
}

VTKM_CONT
template <typename ParticleType>
void ParticleMessenger<ParticleType>::Exchange(
  const std::vector<ParticleType>& outData,
  const std::vector<vtkm::Id>& outRanks,
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
  vtkm::Id numLocalTerm,
  std::vector<ParticleType>& inData,
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
  vtkm::Id& numTerminateMessages,
  bool blockAndWait)
{
  VTKM_ASSERT(outData.size() == outRanks.size());

  numTerminateMessages = 0;
  inDataBlockIDsMap.clear();

  if (this->GetNumRanks() == 1)
    return this->SerialExchange(
      outData, outRanks, outBlockIDsMap, numLocalTerm, inData, inDataBlockIDsMap, blockAndWait);

#ifdef VTKM_ENABLE_MPI

  //dstRank, vector of (particles,blockIDs)
  std::unordered_map<int, std::vector<ParticleCommType>> sendData;

  std::size_t numP = outData.size();
  for (std::size_t i = 0; i < numP; i++)
  {
    const auto& bids = outBlockIDsMap.find(outData[i].GetID())->second;
    sendData[outRanks[i]].emplace_back(std::make_pair(outData[i], bids));
  }

  //Do all the sends first.
  if (numLocalTerm > 0)
    this->SendAllMsg({ MSG_TERMINATE, static_cast<int>(numLocalTerm) });
  this->SendParticles(sendData);
  this->CheckPendingSendRequests();

  //Check if we have anything coming in.
  std::vector<ParticleRecvCommType> particleData;
  std::vector<MsgCommType> msgData;
  if (RecvAny(&msgData, &particleData, blockAndWait))
  {
    for (const auto& it : particleData)
      for (const auto& v : it.second)
      {
        const auto& p = v.first;
        const auto& bids = v.second;
        inData.emplace_back(p);
        inDataBlockIDsMap[p.GetID()] = bids;
      }

    for (const auto& m : msgData)
    {
      if (m.second[0] == MSG_TERMINATE)
        numTerminateMessages += static_cast<vtkm::Id>(m.second[1]);
    }
  }
#endif
}


#ifdef VTKM_ENABLE_MPI

VTKM_CONT
template <typename ParticleType>
void ParticleMessenger<ParticleType>::RegisterMessages(int msgSz, int nParticles, int numBlockIds)
{
  //Determine buffer size for msg and particle tags.
  std::size_t messageBuffSz = CalcMessageBufferSize(msgSz + 1);
  std::size_t particleBuffSz = CalcParticleBufferSize(nParticles, numBlockIds);

  int numRecvs = std::min(64, this->GetNumRanks() - 1);

  this->RegisterTag(ParticleMessenger::MESSAGE_TAG, numRecvs, messageBuffSz);
  this->RegisterTag(ParticleMessenger::PARTICLE_TAG, numRecvs, particleBuffSz);

  this->InitializeBuffers();
}

VTKM_CONT
template <typename ParticleType>
void ParticleMessenger<ParticleType>::SendMsg(int dst, const std::vector<int>& msg)
{
  vtkmdiy::MemoryBuffer buff;

  //Write data.
  vtkmdiy::save(buff, this->GetRank());
  vtkmdiy::save(buff, msg);
  this->SendData(dst, ParticleMessenger::MESSAGE_TAG, buff);
}

VTKM_CONT
template <typename ParticleType>
void ParticleMessenger<ParticleType>::SendAllMsg(const std::vector<int>& msg)
{
  for (int i = 0; i < this->GetNumRanks(); i++)
    if (i != this->GetRank())
      this->SendMsg(i, msg);
}

VTKM_CONT
template <typename ParticleType>
bool ParticleMessenger<ParticleType>::RecvAny(std::vector<MsgCommType>* msgs,
                                              std::vector<ParticleRecvCommType>* recvParticles,
                                              bool blockAndWait)
{
  std::set<int> tags;
  if (msgs)
  {
    tags.insert(ParticleMessenger::MESSAGE_TAG);
    msgs->resize(0);
  }
  if (recvParticles)
  {
    tags.insert(ParticleMessenger::PARTICLE_TAG);
    recvParticles->resize(0);
  }

  if (tags.empty())
    return false;

  std::vector<std::pair<int, vtkmdiy::MemoryBuffer>> buffers;
  if (!this->RecvData(tags, buffers, blockAndWait))
    return false;

  for (auto& buff : buffers)
  {
    if (buff.first == ParticleMessenger::MESSAGE_TAG)
    {
      int sendRank;
      std::vector<int> m;
      vtkmdiy::load(buff.second, sendRank);
      vtkmdiy::load(buff.second, m);
      msgs->emplace_back(std::make_pair(sendRank, m));
    }
    else if (buff.first == ParticleMessenger::PARTICLE_TAG)
    {
      int sendRank;
      std::vector<ParticleCommType> particles;

      vtkmdiy::load(buff.second, sendRank);
      vtkmdiy::load(buff.second, particles);
      recvParticles->emplace_back(std::make_pair(sendRank, particles));
    }
  }

  return true;
}

VTKM_CONT
template <typename ParticleType>
template <typename P, template <typename, typename> class Container, typename Allocator>
inline void ParticleMessenger<ParticleType>::SendParticles(int dst,
                                                           const Container<P, Allocator>& c)
{
  if (dst == this->GetRank())
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error, "Error. Sending a particle to yourself.");
    return;
  }
  if (c.empty())
    return;

  vtkmdiy::MemoryBuffer bb;
  vtkmdiy::save(bb, this->GetRank());
  vtkmdiy::save(bb, c);
  this->SendData(dst, ParticleMessenger::PARTICLE_TAG, bb);
}

VTKM_CONT
template <typename ParticleType>
template <typename P, template <typename, typename> class Container, typename Allocator>
inline void ParticleMessenger<ParticleType>::SendParticles(
  const std::unordered_map<int, Container<P, Allocator>>& m)
{
  for (const auto& mit : m)
    if (!mit.second.empty())
      this->SendParticles(mit.first, mit.second);
}
#endif

}
}
}
} // vtkm::filter::flow::internal

#endif // vtk_m_filter_flow_internal_ParticleMessenger_h
