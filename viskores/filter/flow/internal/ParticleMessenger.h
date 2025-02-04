//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_filter_flow_internal_ParticleMessenger_h
#define viskores_filter_flow_internal_ParticleMessenger_h

#include <viskores/Particle.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/internal/Messenger.h>
#include <viskores/filter/flow/viskores_filter_flow_export.h>

#include <list>
#include <map>
#include <set>
#include <vector>

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename ParticleType>
class VISKORES_FILTER_FLOW_EXPORT ParticleMessenger : public viskores::filter::flow::internal::Messenger
{
  //sendRank, message
  using MsgCommType = std::pair<int, std::vector<int>>;

  //particle + blockIDs.
  using ParticleCommType = std::pair<ParticleType, std::vector<viskores::Id>>;

  //sendRank, vector of ParticleCommType.
  using ParticleRecvCommType = std::pair<int, std::vector<ParticleCommType>>;

public:
  VISKORES_CONT ParticleMessenger(viskoresdiy::mpi::communicator& comm,
                              bool useAsyncComm,
                              const viskores::filter::flow::internal::BoundsMap& bm,
                              int msgSz = 1,
                              int numParticles = 128,
                              int numBlockIds = 2);
  VISKORES_CONT ~ParticleMessenger() {}

  VISKORES_CONT void Exchange(const std::vector<ParticleType>& outData,
                          const std::vector<viskores::Id>& outRanks,
                          const std::unordered_map<viskores::Id, std::vector<viskores::Id>>& outBlockIDsMap,
                          viskores::Id numLocalTerm,
                          std::vector<ParticleType>& inData,
                          std::unordered_map<viskores::Id, std::vector<viskores::Id>>& inDataBlockIDsMap,
                          viskores::Id& numTerminateMessages,
                          bool blockAndWait = false);

protected:
#ifdef VISKORES_ENABLE_MPI
  static constexpr int MSG_TERMINATE = 1;

  enum { MESSAGE_TAG = 0x42000, PARTICLE_TAG = 0x42001 };

  VISKORES_CONT void RegisterMessages(int msgSz, int nParticles, int numBlockIds);

  // Send/Recv particles
  VISKORES_CONT
  template <typename P,
            template <typename, typename>
            class Container,
            typename Allocator = std::allocator<P>>
  inline void SendParticles(int dst, const Container<P, Allocator>& c);

  VISKORES_CONT
  template <typename P,
            template <typename, typename>
            class Container,
            typename Allocator = std::allocator<P>>
  inline void SendParticles(const std::unordered_map<int, Container<P, Allocator>>& m);

  // Send/Recv messages.
  VISKORES_CONT void SendMsg(int dst, const std::vector<int>& msg);
  VISKORES_CONT void SendAllMsg(const std::vector<int>& msg);
  VISKORES_CONT bool RecvMsg(std::vector<MsgCommType>& msgs) { return RecvAny(&msgs, NULL, false); }

  // Send/Recv datasets.
  VISKORES_CONT bool RecvAny(std::vector<MsgCommType>* msgs,
                         std::vector<ParticleRecvCommType>* recvParticles,
                         bool blockAndWait);
  const viskores::filter::flow::internal::BoundsMap& BoundsMap;

#endif

  VISKORES_CONT void SerialExchange(
    const std::vector<ParticleType>& outData,
    const std::vector<viskores::Id>& outRanks,
    const std::unordered_map<viskores::Id, std::vector<viskores::Id>>& outBlockIDsMap,
    viskores::Id numLocalTerm,
    std::vector<ParticleType>& inData,
    std::unordered_map<viskores::Id, std::vector<viskores::Id>>& inDataBlockIDsMap,
    bool blockAndWait) const;

  static std::size_t CalcParticleBufferSize(std::size_t nParticles, std::size_t numBlockIds = 2);
};

//methods

VISKORES_CONT
template <typename ParticleType>
ParticleMessenger<ParticleType>::ParticleMessenger(
  viskoresdiy::mpi::communicator& comm,
  bool useAsyncComm,
  const viskores::filter::flow::internal::BoundsMap& boundsMap,
  int msgSz,
  int numParticles,
  int numBlockIds)
  : Messenger(comm, useAsyncComm)
#ifdef VISKORES_ENABLE_MPI
  , BoundsMap(boundsMap)
#endif
{
#ifdef VISKORES_ENABLE_MPI
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
  viskoresdiy::MemoryBuffer buff;
  ParticleType p;
  viskoresdiy::save(buff, p);

  //Make sure the buffer size is correct.
  //If this fires, then the size of the class has changed.
  VISKORES_ASSERT(pSize == buff.size());
#endif

  return
    // rank
    sizeof(int)
    //std::vector<ParticleType> p;
    //p.size()
    + sizeof(std::size_t)
    //nParticles of ParticleType
    + nParticles * pSize
    // std::vector<viskores::Id> blockIDs for each particle.
    // blockIDs.size() for each particle
    + nParticles * sizeof(std::size_t)
    // nBlockIDs of viskores::Id for each particle.
    + nParticles * nBlockIds * sizeof(viskores::Id);
}

VISKORES_CONT
template <typename ParticleType>
void ParticleMessenger<ParticleType>::SerialExchange(
  const std::vector<ParticleType>& outData,
  const std::vector<viskores::Id>& viskoresNotUsed(outRanks),
  const std::unordered_map<viskores::Id, std::vector<viskores::Id>>& outBlockIDsMap,
  viskores::Id viskoresNotUsed(numLocalTerm),
  std::vector<ParticleType>& inData,
  std::unordered_map<viskores::Id, std::vector<viskores::Id>>& inDataBlockIDsMap,
  bool viskoresNotUsed(blockAndWait)) const
{
  for (auto& p : outData)
  {
    const auto& bids = outBlockIDsMap.find(p.GetID())->second;
    inData.emplace_back(p);
    inDataBlockIDsMap[p.GetID()] = bids;
  }
}

VISKORES_CONT
template <typename ParticleType>
void ParticleMessenger<ParticleType>::Exchange(
  const std::vector<ParticleType>& outData,
  const std::vector<viskores::Id>& outRanks,
  const std::unordered_map<viskores::Id, std::vector<viskores::Id>>& outBlockIDsMap,
  viskores::Id numLocalTerm,
  std::vector<ParticleType>& inData,
  std::unordered_map<viskores::Id, std::vector<viskores::Id>>& inDataBlockIDsMap,
  viskores::Id& numTerminateMessages,
  bool blockAndWait)
{
  VISKORES_ASSERT(outData.size() == outRanks.size());

  numTerminateMessages = 0;
  inDataBlockIDsMap.clear();

  if (this->GetNumRanks() == 1)
    return this->SerialExchange(
      outData, outRanks, outBlockIDsMap, numLocalTerm, inData, inDataBlockIDsMap, blockAndWait);

#ifdef VISKORES_ENABLE_MPI

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
        numTerminateMessages += static_cast<viskores::Id>(m.second[1]);
    }
  }
#endif
}


#ifdef VISKORES_ENABLE_MPI

VISKORES_CONT
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

VISKORES_CONT
template <typename ParticleType>
void ParticleMessenger<ParticleType>::SendMsg(int dst, const std::vector<int>& msg)
{
  viskoresdiy::MemoryBuffer buff;

  //Write data.
  viskoresdiy::save(buff, this->GetRank());
  viskoresdiy::save(buff, msg);
  this->SendData(dst, ParticleMessenger::MESSAGE_TAG, buff);
}

VISKORES_CONT
template <typename ParticleType>
void ParticleMessenger<ParticleType>::SendAllMsg(const std::vector<int>& msg)
{
  for (int i = 0; i < this->GetNumRanks(); i++)
    if (i != this->GetRank())
      this->SendMsg(i, msg);
}

VISKORES_CONT
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

  std::vector<std::pair<int, viskoresdiy::MemoryBuffer>> buffers;
  if (!this->RecvData(tags, buffers, blockAndWait))
    return false;

  for (auto& buff : buffers)
  {
    if (buff.first == ParticleMessenger::MESSAGE_TAG)
    {
      int sendRank;
      std::vector<int> m;
      viskoresdiy::load(buff.second, sendRank);
      viskoresdiy::load(buff.second, m);
      msgs->emplace_back(std::make_pair(sendRank, m));
    }
    else if (buff.first == ParticleMessenger::PARTICLE_TAG)
    {
      int sendRank;
      std::vector<ParticleCommType> particles;

      viskoresdiy::load(buff.second, sendRank);
      viskoresdiy::load(buff.second, particles);
      recvParticles->emplace_back(std::make_pair(sendRank, particles));
    }
  }

  return true;
}

VISKORES_CONT
template <typename ParticleType>
template <typename P, template <typename, typename> class Container, typename Allocator>
inline void ParticleMessenger<ParticleType>::SendParticles(int dst,
                                                           const Container<P, Allocator>& c)
{
  if (dst == this->GetRank())
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Error, "Error. Sending a particle to yourself.");
    return;
  }
  if (c.empty())
    return;

  viskoresdiy::MemoryBuffer bb;
  viskoresdiy::save(bb, this->GetRank());
  viskoresdiy::save(bb, c);
  this->SendData(dst, ParticleMessenger::PARTICLE_TAG, bb);
}

VISKORES_CONT
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
} // viskores::filter::flow::internal

#endif // viskores_filter_flow_internal_ParticleMessenger_h
