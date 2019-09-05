//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_examples_multibackend_TaskQueue_h
#define vtk_m_examples_multibackend_TaskQueue_h

#include <vtkm/cont/PartitionedDataSet.h>

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class TaskQueue
{
public:
  TaskQueue() = default;

  void reset()
  {
    {
      std::unique_lock<std::mutex> lock(this->Lock);
      this->ShutdownOnceTasksCompleted = false;
      this->TaskCount = 0;
    }
    this->CV.notify_all();
  }

  void shutdown()
  {
    {
      std::unique_lock<std::mutex> lock(this->Lock);
      this->ShutdownOnceTasksCompleted = true;
    }
    this->CV.notify_all();
  }

  //Say we always have tasks while the producer (IO) hasn't
  //reported it is finished adding tasks. Once it has finished
  //submitting tasks, we run until the queue is empty
  bool hasTasks()
  {
    {
      std::unique_lock<std::mutex> lock(this->Lock);
      if (this->ShutdownOnceTasksCompleted)
      {
        return this->Queue.size() > 0;
      }
      return true;
    }
  }

  //Add a task to the Queue.
  void push(T&& item)
  {
    {
      std::unique_lock<std::mutex> lock(this->Lock);
      this->Queue.push(item);
      this->TaskCount++;
    } //unlock before we notify so we don't deadlock
    this->CV.notify_all();
  }

  //Get a task from the Queue.
  T pop()
  {
    T item;
    {
      //wait for a job to come into the queue
      std::unique_lock<std::mutex> lock(this->Lock);
      this->CV.wait(lock, [this] {
        //if we are shutting down we need to always wake up
        if (this->ShutdownOnceTasksCompleted)
        {
          return true;
        }
        //if we aren't shutting down sleep when we have no work
        return this->Queue.size() > 0;
      });

      //When shutting down we don't check the queue size
      //so make sure we have something to pop
      if (this->Queue.size() > 0)
      {
        //take the job
        item = this->Queue.front();
        this->Queue.pop();
      }
    } //unlock before we notify so we don't deadlock

    this->CV.notify_all();
    return item;
  }

  //Report that you finished processing a task popped from
  //the Queue
  void completedTask()
  {
    {
      std::unique_lock<std::mutex> lock(this->Lock);
      this->TaskCount--;
    } //unlock before we notify so we don't deadlock
    this->CV.notify_all();
  }

  //Wait for all task to be removed from the queue
  //and to be completed
  //For this to , threads after processing the
  //data they got from pop() must call didTask()
  //

  void waitForAllTasksToComplete()
  {
    {
      std::unique_lock<std::mutex> lock(this->Lock);
      this->CV.wait(lock, [this] { return this->TaskCount == 0; });
    }
    this->CV.notify_all();
  }

private:
  std::mutex Lock;
  std::queue<T> Queue;
  std::condition_variable CV;
  int TaskCount = 0;
  bool ShutdownOnceTasksCompleted = false;

  //don't want copies of this
  TaskQueue(const TaskQueue& rhs) = delete;
  TaskQueue& operator=(const TaskQueue& rhs) = delete;
  TaskQueue(TaskQueue&& rhs) = delete;
  TaskQueue& operator=(TaskQueue&& rhs) = delete;
};

#endif
