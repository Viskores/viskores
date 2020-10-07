//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <iostream>
#include <sstream>
#include <string.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/particleadvection/Messenger.h>

#ifdef VTKM_ENABLE_MPI
#include <vtkm/thirdparty/diy/mpi-cast.h>
#endif

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

VTKM_CONT
#ifdef VTKM_ENABLE_MPI
Messenger::Messenger(vtkmdiy::mpi::communicator& comm)
  : MPIComm(vtkmdiy::mpi::mpi_cast(comm.handle()))
  , MsgID(0)
  , NumRanks(comm.size())
  , Rank(comm.rank())
#else
Messenger::Messenger(vtkmdiy::mpi::communicator& vtkmNotUsed(comm))
#endif
{
}

#ifdef VTKM_ENABLE_MPI
VTKM_CONT
void Messenger::RegisterTag(int tag, std::size_t num_recvs, std::size_t size)
{
  if (this->MessageTagInfo.find(tag) != this->MessageTagInfo.end() || tag == TAG_ANY)
  {
    std::stringstream msg;
    msg << "Invalid message tag: " << tag << std::endl;
    throw vtkm::cont::ErrorFilterExecution(msg.str());
  }
  this->MessageTagInfo[tag] = std::pair<std::size_t, std::size_t>(num_recvs, size);
}

std::size_t Messenger::CalcMessageBufferSize(std::size_t msgSz)
{
  return sizeof(int) // rank
    // std::vector<int> msg;
    // msg.size()
    + sizeof(std::size_t)
    // msgSz ints.
    + msgSz * sizeof(int);
}

void Messenger::InitializeBuffers()
{
  //Setup receive buffers.
  for (const auto& it : this->MessageTagInfo)
  {
    int tag = it.first;
    std::size_t num = it.second.first;
    std::size_t sz = it.second.second;
    for (std::size_t i = 0; i < num; i++)
      this->PostRecv(tag, sz);
  }
}

void Messenger::CleanupRequests(int tag)
{
  std::vector<RequestTagPair> delKeys;
  for (auto&& i : this->RecvBuffers)
  {
    if (tag == TAG_ANY || tag == i.first.second)
      delKeys.push_back(i.first);
  }

  if (!delKeys.empty())
  {
    for (const auto& it : delKeys)
    {
      RequestTagPair v = it;

      char* buff = this->RecvBuffers[v];
      MPI_Cancel(&(v.first));
      delete[] buff;
      this->RecvBuffers.erase(v);
    }
  }
}

void Messenger::PostRecv(int tag)
{
  auto it = this->MessageTagInfo.find(tag);
  if (it != this->MessageTagInfo.end())
    this->PostRecv(tag, it->second.second);
}

void Messenger::PostRecv(int tag, std::size_t sz, int src)
{
  sz += sizeof(Messenger::Header);
  char* buff = new char[sz];
  memset(buff, 0, sz);

  MPI_Request req;
  if (src == -1)
    MPI_Irecv(buff, sz, MPI_BYTE, MPI_ANY_SOURCE, tag, this->MPIComm, &req);
  else
    MPI_Irecv(buff, sz, MPI_BYTE, src, tag, this->MPIComm, &req);

  RequestTagPair entry(req, tag);
  this->RecvBuffers[entry] = buff;
}

void Messenger::CheckPendingSendRequests()
{
  std::vector<MPI_Request> req, copy;
  std::vector<int> tags;

  for (auto it = this->SendBuffers.begin(); it != this->SendBuffers.end(); it++)
  {
    req.push_back(it->first.first);
    copy.push_back(it->first.first);
    tags.push_back(it->first.second);
  }

  if (req.empty())
    return;

  //See if any sends are done.
  int num = 0, *indices = new int[req.size()];
  MPI_Status* status = new MPI_Status[req.size()];
  int err = MPI_Testsome(req.size(), &req[0], &num, indices, status);
  if (err != MPI_SUCCESS)
    throw vtkm::cont::ErrorFilterExecution(
      "Error iwth MPI_Testsome in Messenger::CheckPendingSendRequests");

  for (int i = 0; i < num; i++)
  {
    MPI_Request r = copy[indices[i]];
    int tag = tags[indices[i]];

    RequestTagPair k(r, tag);
    auto entry = this->SendBuffers.find(k);
    if (entry != this->SendBuffers.end())
    {
      delete[] entry->second;
      this->SendBuffers.erase(entry);
    }
  }

  delete[] indices;
  delete[] status;
}

bool Messenger::PacketCompare(const char* a, const char* b)
{
  Messenger::Header ha, hb;
  memcpy(&ha, a, sizeof(ha));
  memcpy(&hb, b, sizeof(hb));

  return ha.packet < hb.packet;
}

void Messenger::PrepareForSend(int tag,
                               const vtkmdiy::MemoryBuffer& buff,
                               std::vector<char*>& buffList)
{
  auto it = this->MessageTagInfo.find(tag);
  if (it == this->MessageTagInfo.end())
  {
    std::stringstream msg;
    msg << "Message tag not found: " << tag << std::endl;
    throw vtkm::cont::ErrorFilterExecution(msg.str());
  }

  std::size_t bytesLeft = buff.size();
  std::size_t maxDataLen = it->second.second;
  Messenger::Header header;
  header.tag = tag;
  header.rank = this->Rank;
  header.id = this->GetMsgID();
  header.numPackets = 1;
  if (buff.size() > maxDataLen)
    header.numPackets += buff.size() / maxDataLen;

  header.packet = 0;
  header.packetSz = 0;
  header.dataSz = 0;

  buffList.resize(header.numPackets);
  std::size_t pos = 0;
  for (std::size_t i = 0; i < header.numPackets; i++)
  {
    header.packet = i;
    if (i == (header.numPackets - 1))
      header.dataSz = bytesLeft;
    else
      header.dataSz = maxDataLen;

    header.packetSz = header.dataSz + sizeof(header);
    char* b = new char[header.packetSz];

    //Write the header.
    char* bPtr = b;
    memcpy(bPtr, &header, sizeof(header));
    bPtr += sizeof(header);

    //Write the data.
    memcpy(bPtr, &buff.buffer[pos], header.dataSz);
    pos += header.dataSz;

    buffList[i] = b;
    bytesLeft -= maxDataLen;
  }
}

void Messenger::SendData(int dst, int tag, const vtkmdiy::MemoryBuffer& buff)
{
  std::vector<char*> bufferList;

  //Add headers, break into multiple buffers if needed.
  PrepareForSend(tag, buff, bufferList);

  Messenger::Header header;
  for (std::size_t i = 0; i < bufferList.size(); i++)
  {
    memcpy(&header, bufferList[i], sizeof(header));
    MPI_Request req;
    int err = MPI_Isend(bufferList[i], header.packetSz, MPI_BYTE, dst, tag, this->MPIComm, &req);
    if (err != MPI_SUCCESS)
      throw vtkm::cont::ErrorFilterExecution("Error in MPI_Isend inside Messenger::SendData");

    //Add it to sendBuffers
    RequestTagPair entry(req, tag);
    this->SendBuffers[entry] = bufferList[i];
  }
}

bool Messenger::RecvData(int tag, std::vector<vtkmdiy::MemoryBuffer>& buffers, bool blockAndWait)
{
  std::set<int> setTag;
  setTag.insert(tag);
  std::vector<std::pair<int, vtkmdiy::MemoryBuffer>> b;
  buffers.resize(0);
  if (RecvData(setTag, b, blockAndWait))
  {
    buffers.resize(b.size());
    for (std::size_t i = 0; i < b.size(); i++)
      buffers[i] = std::move(b[i].second);
    return true;
  }
  return false;
}

bool Messenger::RecvData(std::set<int>& tags,
                         std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>& buffers,
                         bool blockAndWait)
{
  buffers.resize(0);

  //Find all recv of type tag.
  std::vector<MPI_Request> req, copy;
  std::vector<int> reqTags;
  for (auto i = this->RecvBuffers.begin(); i != this->RecvBuffers.end(); i++)
  {
    if (tags.find(i->first.second) != tags.end())
    {
      req.push_back(i->first.first);
      copy.push_back(i->first.first);
      reqTags.push_back(i->first.second);
    }
  }

  if (req.empty())
    return false;

  MPI_Status* status = new MPI_Status[req.size()];
  int *indices = new int[req.size()], num = 0;
  if (blockAndWait)
    MPI_Waitsome(req.size(), &req[0], &num, indices, status);
  else
    MPI_Testsome(req.size(), &req[0], &num, indices, status);

  if (num == 0)
  {
    delete[] status;
    delete[] indices;
    return false;
  }

  std::vector<char*> incomingBuffers(num);
  for (int i = 0; i < num; i++)
  {
    RequestTagPair entry(copy[indices[i]], reqTags[indices[i]]);
    auto it = this->RecvBuffers.find(entry);
    if (it == this->RecvBuffers.end())
    {
      delete[] status;
      delete[] indices;
      throw vtkm::cont::ErrorFilterExecution("receive buffer not found");
    }

    incomingBuffers[i] = it->second;
    this->RecvBuffers.erase(it);
  }

  ProcessReceivedBuffers(incomingBuffers, buffers);

  for (int i = 0; i < num; i++)
    PostRecv(reqTags[indices[i]]);

  delete[] status;
  delete[] indices;

  return !buffers.empty();
}

void Messenger::ProcessReceivedBuffers(std::vector<char*>& incomingBuffers,
                                       std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>& buffers)
{
  for (std::size_t i = 0; i < incomingBuffers.size(); i++)
  {
    char* buff = incomingBuffers[i];

    //Grab the header.
    Messenger::Header header;
    memcpy(&header, buff, sizeof(header));

    //Only 1 packet, strip off header and add to list.
    if (header.numPackets == 1)
    {
      std::pair<int, vtkmdiy::MemoryBuffer> entry;
      entry.first = header.tag;
      entry.second.save_binary((char*)(buff + sizeof(header)), header.dataSz);
      entry.second.reset();
      buffers.push_back(std::move(entry));

      delete[] buff;
    }

    //Multi packet....
    else
    {
      RankIdPair k(header.rank, header.id);
      auto i2 = this->RecvPackets.find(k);

      //First packet. Create a new list and add it.
      if (i2 == this->RecvPackets.end())
      {
        std::list<char*> l;
        l.push_back(buff);
        this->RecvPackets[k] = l;
      }
      else
      {
        i2->second.push_back(buff);

        // The last packet came in, merge into one MemStream.
        if (i2->second.size() == header.numPackets)
        {
          //Sort the packets into proper order.
          i2->second.sort(Messenger::PacketCompare);

          std::pair<int, vtkmdiy::MemoryBuffer> entry;
          entry.first = header.tag;
          for (const auto& listIt : i2->second)
          {
            char* bi = listIt;

            Messenger::Header header2;
            memcpy(&header2, bi, sizeof(header2));
            entry.second.save_binary((char*)(bi + sizeof(header2)), header2.dataSz);
            delete[] bi;
          }

          entry.second.reset();
          buffers.push_back(std::move(entry));
          this->RecvPackets.erase(i2);
        }
      }
    }
  }
}

#endif
}
}
} // namespace vtkm::filter::particleadvection
