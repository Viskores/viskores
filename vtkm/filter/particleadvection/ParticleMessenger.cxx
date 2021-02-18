//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/particleadvection/ParticleMessenger.h>

#include <iostream>
#include <string.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/Serialization.h>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

VTKM_CONT
ParticleMessenger::ParticleMessenger(vtkmdiy::mpi::communicator& comm,
                                     const vtkm::filter::particleadvection::BoundsMap& boundsMap,
                                     int msgSz,
                                     int numParticles,
                                     int numBlockIds)
  : Messenger(comm)
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

std::size_t ParticleMessenger::CalcParticleBufferSize(std::size_t nParticles, std::size_t nBlockIds)
{
  constexpr std::size_t pSize = sizeof(vtkm::Vec3f) // Pos
    + sizeof(vtkm::Id)                              // ID
    + sizeof(vtkm::Id)                              // NumSteps
    + sizeof(vtkm::UInt8)                           // Status
    + sizeof(vtkm::FloatDefault);                   // Time

#ifndef NDEBUG
  vtkmdiy::MemoryBuffer buff;
  vtkm::Particle p;
  vtkmdiy::save(buff, p);

  //If this assert fires, vtkm::Particle changed
  //and pSize should be updated.
  VTKM_ASSERT(pSize == buff.size());
#endif

  return
    // rank
    sizeof(int)
    //std::vector<vtkm::Particle> p;
    //p.size()
    + sizeof(std::size_t)
    //nParticles of vtkm::Particle
    + nParticles * pSize
    // std::vector<vtkm::Id> blockIDs for each particle.
    // blockIDs.size() for each particle
    + nParticles * sizeof(std::size_t)
    // nBlockIDs of vtkm::Id for each particle.
    + nParticles * nBlockIds * sizeof(vtkm::Id);
}

VTKM_CONT
void ParticleMessenger::SerialExchange(
  const std::vector<vtkm::Particle>& outData,
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
  vtkm::Id vtkmNotUsed(numLocalTerm),
  std::vector<vtkm::Particle>& inData,
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
  bool vtkmNotUsed(blockAndWait)) const
{
  for (auto& p : outData)
  {
    const auto& bids = outBlockIDsMap.find(p.ID)->second;
    inData.push_back(p);
    inDataBlockIDsMap[p.ID] = bids;
  }
}

VTKM_CONT
void ParticleMessenger::Exchange(
  const std::vector<vtkm::Particle>& outData,
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
  vtkm::Id numLocalTerm,
  std::vector<vtkm::Particle>& inData,
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
  vtkm::Id& numTerminateMessages,
  bool blockAndWait)
{
  numTerminateMessages = 0;
  inDataBlockIDsMap.clear();

  if (this->GetNumRanks() == 1)
    return this->SerialExchange(
      outData, outBlockIDsMap, numLocalTerm, inData, inDataBlockIDsMap, blockAndWait);

#ifdef VTKM_ENABLE_MPI

  //dstRank, vector of (particles,blockIDs)
  std::unordered_map<int, std::vector<ParticleCommType>> sendData;

  for (const auto& p : outData)
  {
    const auto& bids = outBlockIDsMap.find(p.ID)->second;
    int dstRank = this->BoundsMap.FindRank(bids[0]);
    sendData[dstRank].push_back(std::make_pair(p, bids));
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
        inData.push_back(p);
        inDataBlockIDsMap[p.ID] = bids;
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
void ParticleMessenger::RegisterMessages(int msgSz, int nParticles, int numBlockIds)
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
void ParticleMessenger::SendMsg(int dst, const std::vector<int>& msg)
{
  vtkmdiy::MemoryBuffer buff;

  //Write data.
  vtkmdiy::save(buff, this->GetRank());
  vtkmdiy::save(buff, msg);
  this->SendData(dst, ParticleMessenger::MESSAGE_TAG, buff);
}

VTKM_CONT
void ParticleMessenger::SendAllMsg(const std::vector<int>& msg)
{
  for (int i = 0; i < this->GetNumRanks(); i++)
    if (i != this->GetRank())
      this->SendMsg(i, msg);
}

VTKM_CONT
bool ParticleMessenger::RecvAny(std::vector<MsgCommType>* msgs,
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
      msgs->push_back(std::make_pair(sendRank, m));
    }
    else if (buff.first == ParticleMessenger::PARTICLE_TAG)
    {
      int sendRank;
      std::vector<ParticleCommType> particles;

      vtkmdiy::load(buff.second, sendRank);
      vtkmdiy::load(buff.second, particles);
      recvParticles->push_back(std::make_pair(sendRank, particles));
    }
  }

  return true;
}

#endif
}
}
} // vtkm::filter::particleadvection
