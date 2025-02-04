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

// This is based on:
// D. Morozov, et al., "IExchange: Asynchronous Communication and Termination Detection for Iterative Algorithms,"
// 2021 IEEE 11th Symposium on Large Data Analysis and Visualization (LDAV), New Orleans, LA, USA, 2021, pp. 12-21,
// doi: 10.1109/LDAV53230.2021.00009.
//
// The challenge for async termination is to determine when all work is complete and no messages remain in flight.
// The algorithm uses a number of states to determine when this occurs.
// State 0: a process is working.
// State 1: Process is done and waiting
// State 2: All done and checking for cancelation
// State 3: Done
//
// State 0:  ----- if no work ----> State 1: (locally done. call ibarrier).
//                                      |
//                                      |  ibarrier done
//                                      |  dirty = "have new work since entering State 1"
//                                      |  call iallreduce(dirty)
//                                      |
//                                  State 2: (all done, checking for cancel)
//                                      |
//                                      | if dirty == 1 : GOTO State 0.
//                                      | else: goto State 3 (DONE)
//
// A process begins in State 0 and remains until it has no more work to do.
// Process calls ibarrier and enters State 1.  When the ibarrier is satisfied, this means that all processes are in State 1.
// When all processes are in State 1, each process sets a dirty flag to true if any work has arrived since entering State 1.
// Each procces call iallreduce(dirty) and enter State 2.
// In State 2, if the iallreduce returns true, there is new work, so return to State 0.
// If the iallreduce returns false, then all work is complete and we can terminate.
//
class AdvectAlgorithmTerminator
{
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
    return !this->HaveWork;
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
      {
        this->LocalDirty = this->Dirty;
        MPI_Iallreduce(
          &this->LocalDirty, &this->AllDirty, 1, MPI_INT, MPI_LOR, this->MPIComm, &this->StateReq);
        this->State = STATE_2;
      }
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
      }
    }
#else
    this->HaveWork = haveLocalWork;
#endif
  }

private:
  bool FirstCall;

#ifdef VTKM_ENABLE_MPI
  enum AdvectAlgorithmTerminatorState
  {
    STATE_0,
    STATE_1,
    STATE_2,
    DONE
  };

  int AllDirty;
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
