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

VTKM_CONT
#ifdef VTKM_ENABLE_MPI
Messenger::Messenger(vtkmdiy::mpi::communicator& comm)
  : MPIComm(vtkmdiy::mpi::mpi_cast(comm.handle()))
  , NumRanks(comm.size())
  , Rank(comm.rank())
#else
Messenger::Messenger(vtkmdiy::mpi::communicator& vtkmNotUsed(comm))
#endif
{
}

#ifdef VTKM_ENABLE_MPI
VTKM_CONT
void Messenger::RegisterTag(int tag, int num_recvs, int size)
{
  if (this->MessageTagInfo.find(tag) != this->MessageTagInfo.end() || tag == TAG_ANY)
  {
    std::stringstream msg;
    msg << "Invalid message tag: " << tag << std::endl;
    throw vtkm::cont::ErrorFilterExecution(msg.str());
  }
  this->MessageTagInfo[tag] = std::pair<int, int>(num_recvs, size);
}

int Messenger::CalcMessageBufferSize(int msgSz)
{
  return static_cast<int>(sizeof(int)) // rank
    // std::vector<int> msg;
    // msg.size()
    + static_cast<int>(sizeof(std::size_t))
    // msgSz ints.
    + msgSz * static_cast<int>(sizeof(int));
}

void Messenger::InitializeBuffers()
{
  //Setup receive buffers.
  std::map<int, std::pair<int, int>>::const_iterator it;
  for (it = this->MessageTagInfo.begin(); it != this->MessageTagInfo.end(); it++)
  {
    int tag = it->first, num = it->second.first;
    for (int i = 0; i < num; i++)
      this->PostRecv(tag);
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
    std::vector<RequestTagPair>::const_iterator it;
    for (it = delKeys.begin(); it != delKeys.end(); it++)
    {
      RequestTagPair v = *it;

      unsigned char* buff = this->RecvBuffers[v];
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

void Messenger::PostRecv(int tag, int sz, int src)
{
  sz += sizeof(Messenger::Header);
  unsigned char* buff = new unsigned char[sz];
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
  {
    std::cerr << "Err with MPI_Testsome in PARIC algorithm" << std::endl;
  }
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

bool Messenger::PacketCompare(const unsigned char* a, const unsigned char* b)
{
  Messenger::Header ha, hb;
  memcpy(&ha, a, sizeof(ha));
  memcpy(&hb, b, sizeof(hb));

  return ha.packet < hb.packet;
}

void Messenger::PrepareForSend(int tag, MemStream* buff, std::vector<unsigned char*>& buffList)
{
  auto it = this->MessageTagInfo.find(tag);
  if (it == this->MessageTagInfo.end())
  {
    std::stringstream msg;
    msg << "Message tag not found: " << tag << std::endl;
    throw vtkm::cont::ErrorFilterExecution(msg.str());
  }

  int bytesLeft = buff->GetLen();
  int maxDataLen = it->second.second;
  Messenger::Header header;
  header.tag = tag;
  header.rank = this->Rank;
  header.id = this->MsgID;
  header.numPackets = 1;
  if (buff->GetLen() > (unsigned int)maxDataLen)
    header.numPackets += buff->GetLen() / maxDataLen;

  header.packet = 0;
  header.packetSz = 0;
  header.dataSz = 0;
  this->MsgID++;

  buffList.resize(header.numPackets);
  size_t pos = 0;
  for (int i = 0; i < header.numPackets; i++)
  {
    header.packet = i;
    if (i == (header.numPackets - 1))
      header.dataSz = bytesLeft;
    else
      header.dataSz = maxDataLen;

    header.packetSz = header.dataSz + sizeof(header);
    unsigned char* b = new unsigned char[header.packetSz];

    //Write the header.
    unsigned char* bPtr = b;
    memcpy(bPtr, &header, sizeof(header));
    bPtr += sizeof(header);

    //Write the data.
    memcpy(bPtr, &buff->GetData()[pos], header.dataSz);
    pos += header.dataSz;

    buffList[i] = b;
    bytesLeft -= maxDataLen;
  }
}

void Messenger::SendData(int dst, int tag, MemStream* buff)
{
  std::vector<unsigned char*> bufferList;

  //Add headers, break into multiple buffers if needed.
  PrepareForSend(tag, buff, bufferList);

  Messenger::Header header;
  for (size_t i = 0; i < bufferList.size(); i++)
  {
    memcpy(&header, bufferList[i], sizeof(header));
    MPI_Request req;
    int err = MPI_Isend(bufferList[i], header.packetSz, MPI_BYTE, dst, tag, this->MPIComm, &req);
    if (err != MPI_SUCCESS)
    {
      std::cerr << "Err with MPI_Isend in SendData algorithm" << std::endl;
    }

    //Add it to sendBuffers
    RequestTagPair entry(req, tag);
    this->SendBuffers[entry] = bufferList[i];
  }

  delete buff;
}

bool Messenger::RecvData(int tag, std::vector<MemStream*>& buffers, bool blockAndWait)
{
  std::set<int> setTag;
  setTag.insert(tag);
  std::vector<std::pair<int, MemStream*>> b;
  buffers.resize(0);
  if (RecvData(setTag, b, blockAndWait))
  {
    buffers.resize(b.size());
    for (size_t i = 0; i < b.size(); i++)
      buffers[i] = b[i].second;
    return true;
  }
  return false;
}

bool Messenger::RecvData(std::set<int>& tags,
                         std::vector<std::pair<int, MemStream*>>& buffers,
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

  std::vector<unsigned char*> incomingBuffers(num);
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

void Messenger::ProcessReceivedBuffers(std::vector<unsigned char*>& incomingBuffers,
                                       std::vector<std::pair<int, MemStream*>>& buffers)
{
  for (size_t i = 0; i < incomingBuffers.size(); i++)
  {
    unsigned char* buff = incomingBuffers[i];

    //Grab the header.
    Messenger::Header header;
    memcpy(&header, buff, sizeof(header));

    //Only 1 packet, strip off header and add to list.
    if (header.numPackets == 1)
    {
      MemStream* b = new MemStream(header.dataSz, (buff + sizeof(header)));
      b->Rewind();
      std::pair<int, MemStream*> entry(header.tag, b);
      buffers.push_back(entry);
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
        std::list<unsigned char*> l;
        l.push_back(buff);
        this->RecvPackets[k] = l;
      }
      else
      {
        i2->second.push_back(buff);

        // The last packet came in, merge into one MemStream.
        if (i2->second.size() == (size_t)header.numPackets)
        {
          //Sort the packets into proper order.
          i2->second.sort(Messenger::PacketCompare);

          MemStream* mergedBuff = new MemStream;
          std::list<unsigned char*>::iterator listIt;

          for (listIt = i2->second.begin(); listIt != i2->second.end(); listIt++)
          {
            unsigned char* bi = *listIt;

            Messenger::Header header2;
            memcpy(&header2, bi, sizeof(header2));
            mergedBuff->WriteBinary((bi + sizeof(header2)), header2.dataSz);
            delete[] bi;
          }

          mergedBuff->Rewind();
          std::pair<int, MemStream*> entry(header.tag, mergedBuff);
          buffers.push_back(entry);
          this->RecvPackets.erase(i2);
        }
      }
    }
  }
}
#endif
}
} // namespace vtkm::filter
