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

#include <vtkm/Particle.h>
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

class VTKM_FILTER_EXPORT ParticleMessenger : public vtkm::filter::particleadvection::Messenger
{
  //sendRank, message
  using MsgCommType = std::pair<int, std::vector<int>>;

  //particle + blockIDs.
  using ParticleCommType = std::pair<vtkm::Massless, std::vector<vtkm::Id>>;

  //sendRank, vector of ParticleCommType.
  using ParticleRecvCommType = std::pair<int, std::vector<ParticleCommType>>;

public:
  VTKM_CONT ParticleMessenger(vtkmdiy::mpi::communicator& comm,
                              const vtkm::filter::particleadvection::BoundsMap& bm,
                              int msgSz = 1,
                              int numParticles = 128);
  VTKM_CONT ~ParticleMessenger() {}

  VTKM_CONT void Exchange(const std::vector<vtkm::Massless>& outData,
                          const std::map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
                          vtkm::Id numLocalTerm,
                          std::vector<vtkm::Massless>& inData,
                          std::map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap,
                          vtkm::Id& numTerminateMessages);

protected:
#ifdef VTKM_ENABLE_MPI
  static constexpr int MSG_TERMINATE = 1;

  enum
  {
    MESSAGE_TAG = 0x42000,
    PARTICLE_TAG = 0x42001
  };

  VTKM_CONT void RegisterMessages(int msgSz, int nParticles);

  // Send/Recv Integral curves.
  VTKM_CONT
  template <typename P,
            template <typename, typename> class Container,
            typename Allocator = std::allocator<P>>
  inline void SendParticles(int dst, const Container<P, Allocator>& c);

  VTKM_CONT
  template <typename P,
            template <typename, typename> class Container,
            typename Allocator = std::allocator<P>>
  inline void SendParticles(const std::map<int, Container<P, Allocator>>& m);

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

  VTKM_CONT void SerialExchange(const std::vector<vtkm::Massless>& outData,
                                const std::map<vtkm::Id, std::vector<vtkm::Id>>& outBlockIDsMap,
                                vtkm::Id numLocalTerm,
                                std::vector<vtkm::Massless>& inData,
                                std::map<vtkm::Id, std::vector<vtkm::Id>>& inDataBlockIDsMap) const;

  static int CalcParticleBufferSize(int nParticles, int numBlockIds = 2);
};


#ifdef VTKM_ENABLE_MPI
VTKM_CONT
template <typename P, template <typename, typename> class Container, typename Allocator>
inline void ParticleMessenger::SendParticles(int dst, const Container<P, Allocator>& c)
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
inline void ParticleMessenger::SendParticles(const std::map<int, Container<P, Allocator>>& m)
{
  for (auto mit = m.begin(); mit != m.end(); mit++)
    if (!mit->second.empty())
      this->SendParticles(mit->first, mit->second);
}
#endif


template <>
struct Serialization<vtkm::Massless>
{
  static void write(vtkm::filter::particleadvection::MemStream& memstream,
                    const vtkm::Massless& data)
  {
    vtkm::filter::particleadvection::write(memstream, data.Pos[0]);
    vtkm::filter::particleadvection::write(memstream, data.Pos[1]);
    vtkm::filter::particleadvection::write(memstream, data.Pos[2]);
    vtkm::filter::particleadvection::write(memstream, data.ID);
    vtkm::filter::particleadvection::write(memstream, data.Status);
    vtkm::filter::particleadvection::write(memstream, data.NumSteps);
    vtkm::filter::particleadvection::write(memstream, data.Time);
  }

  static void read(vtkm::filter::particleadvection::MemStream& memstream, vtkm::Massless& data)
  {
    vtkm::filter::particleadvection::read(memstream, data.Pos[0]);
    vtkm::filter::particleadvection::read(memstream, data.Pos[1]);
    vtkm::filter::particleadvection::read(memstream, data.Pos[2]);
    vtkm::filter::particleadvection::read(memstream, data.ID);
    vtkm::filter::particleadvection::read(memstream, data.Status);
    vtkm::filter::particleadvection::read(memstream, data.NumSteps);
    vtkm::filter::particleadvection::read(memstream, data.Time);
  }
};
}
}
} // namespace vtkm::filter::particleadvection


#endif
