//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_AdvectAlgorithmTerminator_h
#define vtk_m_filter_flow_internal_AdvectAlgorithmTerminator_h

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

class AdvectAlgorithmTerminator
{
  bool FirstCall;

public:
#ifdef VTKM_ENABLE_MPI
  AdvectAlgorithmTerminator(vtkmdiy::mpi::communicator& comm)
    : AllDirty(1)
    , Dirty(1)
    , LocalWork(0)
    , MPIComm(vtkmdiy::mpi::mpi_cast(comm.handle()))
    , Rank(comm.rank())
    , State(STATE_0)
#else
  AdvectAlgorithmTerminator(vtkmdiy::mpi::communicator& vtkmNotUsed(comm))
    : HaveWork(false)
#endif
  {
    this->FirstCall = true;
  }

  bool Done() const
  {
#ifdef VTKM_ENABLE_MPI
    return this->State == AdvectAlgorithmTerminatorState::DONE;
#else
    return this->HaveWork;
#endif
  }

  void Control(bool haveLocalWork)
  {
#ifdef VTKM_ENABLE_MPI
    if (this->FirstCall)
    {
      haveLocalWork = true;
      this->FirstCall = false;
    }

    if (haveLocalWork)
      this->Dirty = 1;

    if (this->State == STATE_0 && !haveLocalWork)
    {
      MPI_Ibarrier(this->MPIComm, &this->StateReq);
      this->Dirty = 0;
      this->State = STATE_1;
    }
    else if (this->State == STATE_1)
    {
      MPI_Status status;
      int flag;
      MPI_Test(&this->StateReq, &flag, &status);
      if (flag == 1)
        this->State = STATE_1B;
    }
    else if (this->State == STATE_1B)
    {
      this->LocalDirty = this->Dirty;
      MPI_Iallreduce(
        &this->LocalDirty, &this->AllDirty, 1, MPI_INT, MPI_LOR, this->MPIComm, &this->StateReq);
      this->State = STATE_2;
      this->BarrierCnt++;
    }
    else if (this->State == STATE_2)
    {
      MPI_Status status;
      int flag;
      MPI_Test(&this->StateReq, &flag, &status);
      if (flag == 1)
      {
        if (this->AllDirty == 0) //done
          this->State = DONE;
        else
          this->State = STATE_0; //reset.
        this->IReduceCnt++;
      }
    }
#else
    this->HaveWork = haveLocalWork;
#endif
  }

private:
#ifdef VTKM_ENABLE_MPI
  enum AdvectAlgorithmTerminatorState
  {
    STATE_0,
    STATE_1,
    STATE_1B,
    STATE_2,
    DONE
  };

  int AllDirty;
  int BarrierCnt = 0;
  int IReduceCnt = 0;
  //Dirty: Has this rank seen any work since entering state?
  std::atomic<int> Dirty;
  int LocalDirty;
  std::atomic<int> LocalWork;
  MPI_Comm MPIComm;
  vtkm::Id Rank;
  AdvectAlgorithmTerminatorState State = AdvectAlgorithmTerminatorState::STATE_0;
  MPI_Request StateReq;
#else
  bool HaveWork;
#endif
};

}
}
}
} //vtkm::filter::flow::internal


#endif //vtk_m_filter_flow_internal_AdvectAlgorithmTerminator_h
