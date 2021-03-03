//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ParticleMessenger_h
#define vtk_m_filter_ParticleMessenger_h

#include <vtkm/filter/vtkm_filter_extra_export.h>

#include <vtkm/Particle.h>
#include <vtkm/cont/Serialization.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/Messenger.h>

#include <list>
#include <map>
#include <set>
#include <vector>

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

class VTKM_FILTER_EXTRA_EXPORT ParticleMessenger : public vtkm::filter::particleadvection::Messenger
{
  //sendRank, message
  using MsgCommType = std::pair<int, std::vector<int>>;

  //particle + blockIDs.
  using ParticleCommType = std::pair<vtkm::Particle, std::vector<vtkm::Id>>;

  //sendRank, vector of ParticleCommType.
  using ParticleRecvCommType = std::pair<int, std::vector<ParticleCommType>>;

public:
  VTKM_CONT ParticleMessenger(vtkmdiy::mpi::communicator& comm,
                              const vtkm::filter::particleadvection::BoundsMap& bm,
                              int msgSz = 1,
                              int numParticles = 128,
                              int numBlockIds = 2);
  VTKM_CONT ~ParticleMessenger() {}

  VTKM_CONT void Exchange(const std::vector<vtkm::Particle>& outData,
                          const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
                          vtkm::Id numLocalTerm,
                          std::vector<vtkm::Particle>& inData,
                          std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
                          vtkm::Id& numTerminateMessages,
                          bool blockAndWait = false);

protected:
#ifdef VTKM_ENABLE_MPI
  static constexpr int MSG_TERMINATE = 1;

  enum
  {
    MESSAGE_TAG = 0x42000,
    PARTICLE_TAG = 0x42001
  };

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
  const vtkm::filter::particleadvection::BoundsMap& BoundsMap;

#endif

  VTKM_CONT void SerialExchange(
    const std::vector<vtkm::Particle>& outData,
    const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
    vtkm::Id numLocalTerm,
    std::vector<vtkm::Particle>& inData,
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
    bool blockAndWait) const;

  static std::size_t CalcParticleBufferSize(std::size_t nParticles, std::size_t numBlockIds = 2);
};


#ifdef VTKM_ENABLE_MPI
VTKM_CONT
template <typename P, template <typename, typename> class Container, typename Allocator>
inline void ParticleMessenger::SendParticles(int dst, const Container<P, Allocator>& c)
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
template <typename P, template <typename, typename> class Container, typename Allocator>
inline void ParticleMessenger::SendParticles(
  const std::unordered_map<int, Container<P, Allocator>>& m)
{
  for (const auto& mit : m)
    if (!mit.second.empty())
      this->SendParticles(mit.first, mit.second);
}
#endif
}
}
} // namespace vtkm::filter::particleadvection


#endif
