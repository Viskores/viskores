//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/particleadvection/MemStream.h>
#include <vtkm/filter/particleadvection/ParticleMessenger.h>

#include <iostream>
#include <string.h>
#include <vtkm/cont/Logging.h>

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
                                     int numParticles)
  : Messenger(comm)
#ifdef VTKM_ENABLE_MPI
  , BoundsMap(boundsMap)
#endif
{
#ifdef VTKM_ENABLE_MPI
  this->RegisterMessages(msgSz, numParticles);
#else
  (void)(boundsMap);
  (void)(msgSz);
  (void)(numParticles);
#endif
}

int ParticleMessenger::CalcParticleBufferSize(int nParticles, int nBlockIds)
{
  return
    // rank
    static_cast<int>(sizeof(int))
    //std::vector<vtkm::Massless> p;
    //p.size()
    + static_cast<int>(sizeof(std::size_t))
    //nParticles of vtkm::Massless
    + nParticles * static_cast<int>(sizeof(vtkm::Massless))
    // std::vector<vtkm::Id> blockIDs for each particle.
    // blockIDs.size() for each particle
    + nParticles * static_cast<int>(sizeof(std::size_t))
    // nBlockIDs of vtkm::Id for each particle.
    + nParticles * nBlockIds * static_cast<int>(sizeof(vtkm::Id));
}

VTKM_CONT
void ParticleMessenger::SerialExchange(
  const std::vector<vtkm::Massless>& outData,
  const std::map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
  vtkm::Id vtkmNotUsed(numLocalTerm),
  std::vector<vtkm::Massless>& inData,
  std::map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap) const
{
  for (auto& p : outData)
  {
    const auto& bids = outBlockIDsMap.find(p.ID)->second;
    inData.push_back(p);
    inDataBlockIDsMap[p.ID] = bids;
  }
}

VTKM_CONT
void ParticleMessenger::Exchange(const std::vector<vtkm::Massless>& outData,
                                 const std::map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
                                 vtkm::Id numLocalTerm,
                                 std::vector<vtkm::Massless>& inData,
                                 std::map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
                                 vtkm::Id& numTerminateMessages)
{
  numTerminateMessages = 0;
  inDataBlockIDsMap.clear();

  if (this->NumRanks == 1)
    return this->SerialExchange(outData, outBlockIDsMap, numLocalTerm, inData, inDataBlockIDsMap);

#ifdef VTKM_ENABLE_MPI

  //dstRank, vector of (particles,blockIDs)
  std::map<int, std::vector<ParticleCommType>> sendData;

  for (auto& p : outData)
  {
    const auto& bids = outBlockIDsMap.find(p.ID)->second;
    int dstRank = this->BoundsMap.FindRank(bids[0]);
    sendData[dstRank].push_back(std::make_pair(p, bids));
  }

  //Check if we have anything coming in.
  std::vector<ParticleRecvCommType> particleData;
  std::vector<MsgCommType> msgData;
  if (RecvAny(&msgData, &particleData, false))
  {
    for (auto& it : particleData)
      for (const auto& v : it.second)
      {
        const auto& p = v.first;
        const auto& bids = v.second;
        inData.push_back(p);
        inDataBlockIDsMap[p.ID] = bids;
      }

    for (auto& m : msgData)
    {
      if (m.second[0] == MSG_TERMINATE)
        numTerminateMessages += static_cast<vtkm::Id>(m.second[1]);
    }
  }

  //Do all the sending...
  if (numLocalTerm > 0)
  {
    std::vector<int> msg = { MSG_TERMINATE, static_cast<int>(numLocalTerm) };
    SendAllMsg(msg);
  }

  this->SendParticles(sendData);
  this->CheckPendingSendRequests();
#endif
}

#ifdef VTKM_ENABLE_MPI

VTKM_CONT
void ParticleMessenger::RegisterMessages(int msgSz, int nParticles)
{
  //Determine buffer size for msg and particle tags.
  int messageBuffSz = CalcMessageBufferSize(msgSz + 1);
  int particleBuffSz = CalcParticleBufferSize(nParticles);

  int numRecvs = std::min(64, this->NumRanks - 1);

  this->RegisterTag(ParticleMessenger::MESSAGE_TAG, numRecvs, messageBuffSz);
  this->RegisterTag(ParticleMessenger::PARTICLE_TAG, numRecvs, particleBuffSz);

  this->InitializeBuffers();
}

VTKM_CONT
void ParticleMessenger::SendMsg(int dst, const std::vector<int>& msg)
{
  MemStream* buff = new MemStream();

  //Write data.
  vtkm::filter::particleadvection::write(*buff, this->Rank);
  vtkm::filter::particleadvection::write(*buff, msg);
  this->SendData(dst, ParticleMessenger::MESSAGE_TAG, buff);
}

VTKM_CONT
void ParticleMessenger::SendAllMsg(const std::vector<int>& msg)
{
  for (int i = 0; i < this->NumRanks; i++)
    if (i != this->Rank)
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

  std::vector<std::pair<int, MemStream*>> buffers;
  if (!this->RecvData(tags, buffers, blockAndWait))
    return false;

  for (size_t i = 0; i < buffers.size(); i++)
  {
    if (buffers[i].first == ParticleMessenger::MESSAGE_TAG)
    {
      int sendRank;
      std::vector<int> m;
      vtkm::filter::particleadvection::read(*buffers[i].second, sendRank);
      vtkm::filter::particleadvection::read(*buffers[i].second, m);

      msgs->push_back(std::make_pair(sendRank, m));
    }
    else if (buffers[i].first == ParticleMessenger::PARTICLE_TAG)
    {
      int sendRank;
      std::size_t num;
      vtkm::filter::particleadvection::read(*buffers[i].second, sendRank);
      vtkm::filter::particleadvection::read(*buffers[i].second, num);
      if (num > 0)
      {
        std::vector<ParticleCommType> particles(num);
        for (std::size_t j = 0; j < num; j++)
        {
          vtkm::filter::particleadvection::read(*(buffers[i].second), particles[j].first);
          vtkm::filter::particleadvection::read(*(buffers[i].second), particles[j].second);
        }
        recvParticles->push_back(std::make_pair(sendRank, particles));
      }
    }

    delete buffers[i].second;
  }

  return true;
}

VTKM_CONT
template <typename P, template <typename, typename> class Container, typename Allocator>
void ParticleMessenger::SendParticles(int dst, const Container<P, Allocator>& c)
{
  if (dst == this->Rank)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error, "Error. Sending a particle to yourself.");
    return;
  }
  if (c.empty())
    return;

  vtkm::filter::particleadvection::MemStream* buff =
    new vtkm::filter::particleadvection::MemStream();
  vtkm::filter::particleadvection::write(*buff, this->Rank);
  vtkm::filter::particleadvection::write(*buff, c);
  this->SendData(dst, ParticleMessenger::PARTICLE_TAG, buff);
}

VTKM_CONT
template <typename P, template <typename, typename> class Container, typename Allocator>
void ParticleMessenger::SendParticles(const std::map<int, Container<P, Allocator>>& m)
{
  for (auto mit = m.begin(); mit != m.end(); mit++)
    if (!mit->second.empty())
      this->SendParticles(mit->first, mit->second);
}

#endif
}
}
} // vtkm::filter::particleadvection
