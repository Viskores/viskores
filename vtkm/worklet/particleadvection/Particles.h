//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_particleadvection_Particles_h
#define vtk_m_worklet_particleadvection_Particles_h

#include <vtkm/Types.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/exec/ExecutionObjectBase.h>

namespace vtkm {
namespace worklet {
namespace particleadvection {

template<typename T,typename DeviceAdapterTag>
class Particles : public vtkm::exec::ExecutionObjectBase
{
private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id>
        ::template ExecutionTypes<DeviceAdapterTag>::Portal IdPortal;    
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T,3> >
        ::template ExecutionTypes<DeviceAdapterTag>::Portal PosPortal;
public:
    VTKM_EXEC_CONT
    Particles() : pos(), steps(), maxSteps(0)
    {
    }
    VTKM_EXEC_CONT    
    Particles(const Particles &ic) :
        pos(ic.pos), steps(ic.steps), maxSteps(ic.maxSteps)
    {
    }

    VTKM_EXEC_CONT        
    Particles(const PosPortal &_pos,
              const IdPortal &_steps,
              const vtkm::Id &_maxSteps) : pos(_pos), steps(_steps), maxSteps(_maxSteps)
    {
    }

    VTKM_EXEC_CONT            
    Particles(vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &posArray,
              vtkm::cont::ArrayHandle<vtkm::Id> &stepsArray,
              const vtkm::Id &_maxSteps) :
        maxSteps(_maxSteps)
    {
        pos = posArray.PrepareForInPlace(DeviceAdapterTag());
        steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());
    }

    VTKM_EXEC_CONT                
    void TakeStep(const vtkm::Id &idx,
                  const vtkm::Vec<T,3> &pt)
    {
        pos.Set(idx, pt);
        steps.Set(idx, steps.Get(idx)+1);
    }

    VTKM_EXEC_CONT                    
    bool Done(const vtkm::Id &idx)
    {
        return steps.Get(idx) == maxSteps;
    }

    VTKM_EXEC_CONT
    vtkm::Vec<T,3> GetPos(const vtkm::Id &idx) const {return pos.Get(idx);}
    VTKM_EXEC_CONT
    vtkm::Id GetStep(const vtkm::Id &idx) const {return steps.Get(idx);}

    VTKM_EXEC_CONT
    void Dump() const
    {
        vtkm::Id N = pos.GetNumberOfValues();
        for (vtkm::Id i = 0; i < N; i++)
            std::cout<<GetPos(i)<<std::endl;
    }

private:
    vtkm::Id maxSteps;
    IdPortal steps;
    PosPortal pos;
};

template<typename T,typename DeviceAdapterTag>
class StateRecordingParticle : public vtkm::exec::ExecutionObjectBase
{
private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id>
        ::template ExecutionTypes<DeviceAdapterTag>::Portal IdPortal;    
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T,3> >
        ::template ExecutionTypes<DeviceAdapterTag>::Portal PosPortal;
public:
    VTKM_EXEC_CONT
    StateRecordingParticle(const StateRecordingParticle &s) :
        pos(s.pos), steps(s.steps), maxSteps(s.maxSteps), 
        history(s.history), histSize(s.histSize), stepOffset(s.stepOffset)
    {
    }
    VTKM_EXEC_CONT
    StateRecordingParticle() : pos(), steps(), maxSteps(0), histSize(-1), stepOffset(0)
    {
    }

    VTKM_EXEC_CONT
    StateRecordingParticle(const PosPortal &_pos,
                           const IdPortal &_steps,
                           const vtkm::Id &_maxSteps) :
        pos(_pos), steps(_steps), maxSteps(_maxSteps)
    {
        std::cout<<"why calling this?????"<<std::endl;
    }

    VTKM_EXEC_CONT
    StateRecordingParticle(vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &posArray,
                           vtkm::cont::ArrayHandle<vtkm::Id> &stepsArray,                                
                           const vtkm::Id &_maxSteps) :
        maxSteps(_maxSteps), histSize(_maxSteps), stepOffset(0)
    {
        pos = posArray.PrepareForInPlace(DeviceAdapterTag());
        steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());

        numPos = posArray.GetNumberOfValues();
        std::cerr<<"Alloc history: "<<histSize<<" sOffset "<<stepOffset<<std::endl;
        history = historyArray.PrepareForOutput(numPos*histSize, DeviceAdapterTag());
    }

    VTKM_EXEC_CONT
    StateRecordingParticle(vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &posArray,
                           vtkm::cont::ArrayHandle<vtkm::Id> &stepsArray,                                
                           const vtkm::Id &_maxSteps,
                           vtkm::Id &_histSize,
                           vtkm::Id &_stepOffset) :
        maxSteps(_maxSteps), histSize(_histSize), stepOffset(_stepOffset)
    {
        pos = posArray.PrepareForInPlace(DeviceAdapterTag());
        steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());

        numPos = posArray.GetNumberOfValues();
        std::cerr<<"Alloc history: "<<histSize<<" sOffset "<<stepOffset<<std::endl;
        history = historyArray.PrepareForOutput(numPos*histSize, DeviceAdapterTag());
    }

    VTKM_EXEC_CONT
    void TakeStep(const vtkm::Id &idx,
                  const vtkm::Vec<T,3> &pt)
    {
        vtkm::Id loc = idx*histSize + (steps.Get(idx)-stepOffset);
        //std::cerr<<"TakeStep("<<idx<<", "<<pt<<"); loc= "<<loc<<" "<<numPos*maxSteps<<std::endl;
        if (loc > histSize*numPos)
        {
            std::cout<<"PROBLEM: "<<idx<<" loc= "<<idx*histSize<<" + "<<steps.Get(idx)<<"-"<<stepOffset<<std::endl;
            std::cout<<"       "<<loc<<" "<<histSize*numPos<<std::endl;
        }
        history.Set(loc, pt);
        steps.Set(idx, steps.Get(idx)+1);
        //Only needed for rounds algorithm.
        pos.Set(idx, pt);
    }

    VTKM_EXEC_CONT
    bool Done(const vtkm::Id &idx)
    {
        //vtkm::Id s = steps.Get(idx);
        //std::cout<<idx<<" steps= "<<s<<std::endl;
        return steps.Get(idx) >= maxSteps;
    }

    VTKM_EXEC_CONT
    vtkm::Vec<T,3> GetPos(const vtkm::Id &idx) const {return pos.Get(idx);}
    VTKM_EXEC_CONT
    vtkm::Id GetStep(const vtkm::Id &idx) const {return steps.Get(idx);}
    VTKM_EXEC_CONT
    vtkm::Vec<T,3> GetHistory(const vtkm::Id &idx, const vtkm::Id &step) const
    {
        //std::cerr<<stepOffset<<"::"<<idx<<" "<<step<<std::endl;
        return history.Get(idx*histSize+step);
    }    

    VTKM_EXEC_CONT
    vtkm::Id Dump() const
    {
        vtkm::Id totSteps = 0;
        vtkm::Id N = pos.GetNumberOfValues();
        for (vtkm::Id i = 0; i < N; i++)
        {
            vtkm::Id ns = GetStep(i)-stepOffset;
            totSteps += ns;
            for (vtkm::Id j = 0; j < ns; j++)
            {
                vtkm::Vec<T,3> p = GetHistory(i,j);
                std::cout<<p[0]<<" "<<p[1]<<" "<<p[2]<<std::endl;
            }
        }
        return totSteps;
    }

private:
    vtkm::Id maxSteps, numPos, histSize, stepOffset;
    IdPortal steps;
    PosPortal pos, history;
    vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > historyArray;
};    

template<typename T,typename DeviceAdapterTag>
class StateRecordingParticleTwoPass : public vtkm::exec::ExecutionObjectBase
{
private:
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id>
        ::template ExecutionTypes<DeviceAdapterTag>::Portal IdPortal;    
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T,3> >
        ::template ExecutionTypes<DeviceAdapterTag>::Portal PosPortal;
public:
    VTKM_EXEC_CONT
    StateRecordingParticleTwoPass(const StateRecordingParticleTwoPass &s) :
        pos(s.pos), steps(s.steps), maxSteps(s.maxSteps), history(s.history), historyOffset(s.historyOffset)
    {
    }
    VTKM_EXEC_CONT
    StateRecordingParticleTwoPass() : pos(), steps(), maxSteps(0), historyOffset()
    {
    }

    VTKM_EXEC_CONT
    StateRecordingParticleTwoPass(const PosPortal &_pos,
                                  const IdPortal &_steps,
                                  const IdPortal &_historyOffset,
                                  const vtkm::Id &_maxSteps) :
        pos(_pos), steps(_steps), historyOffset(_historyOffset), maxSteps(_maxSteps)
    {
    }

    VTKM_EXEC_CONT    
    StateRecordingParticleTwoPass(vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &posArray,
                                  vtkm::cont::ArrayHandle<vtkm::Id> &stepsArray,                          
                                  vtkm::cont::ArrayHandle<vtkm::Id> &historyOffsetArray,
                                  const vtkm::Id &totalNumSteps,
                                  const vtkm::Id &_maxSteps) :
        maxSteps(_maxSteps)
    {
        pos = posArray.PrepareForInPlace(DeviceAdapterTag());
        steps = stepsArray.PrepareForInPlace(DeviceAdapterTag());
        historyOffset = historyOffsetArray.PrepareForInPlace(DeviceAdapterTag());

        history = historyArray.PrepareForOutput(totalNumSteps, DeviceAdapterTag());
    }

    VTKM_EXEC_CONT
    void TakeStep(const vtkm::Id &idx,
                  const vtkm::Vec<T,3> &pt)
    {
        vtkm::Id loc = historyOffset.Get(idx) + steps.Get(idx);
        //std::cerr<<"TakeStep("<<idx<<", "<<pt<<"); loc= "<<historyOffset.Get(idx)<<" + "<<steps.Get(idx)<<std::endl;
        history.Set(loc, pt);
        steps.Set(idx, steps.Get(idx)+1);
    }

    VTKM_EXEC_CONT
    bool Done(const vtkm::Id &idx)
    {
        //vtkm::Id s = steps.Get(idx);
        //std::cout<<idx<<" steps= "<<s<<std::endl;
        //std::cerr<<steps.Get(idx)<<"  >= "<<maxSteps<<std::endl;

        return steps.Get(idx) >= maxSteps;
    }

    VTKM_EXEC_CONT
    vtkm::Vec<T,3> GetPos(const vtkm::Id &idx) const {return pos.Get(idx);}
    VTKM_EXEC_CONT
    vtkm::Id GetStep(const vtkm::Id &idx) const {return steps.Get(idx);}
    VTKM_EXEC_CONT
    vtkm::Vec<T,3> GetHistory(const vtkm::Id &idx, const vtkm::Id &step) const
    {
        return history.Get(historyOffset.Get(idx)+step);
    }    

    VTKM_EXEC_CONT
    void Dump() const
    {
        vtkm::Id N = pos.GetNumberOfValues();
        for (vtkm::Id i = 0; i < N; i++)
        {
            vtkm::Id ns = GetStep(i);
            for (vtkm::Id j = 0; j < ns; j++)
                std::cout<<GetHistory(i,j)<<std::endl;
        }
    }

private:
    vtkm::Id maxSteps;
    IdPortal steps, historyOffset;
    PosPortal pos, history;
    vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > historyArray;
};    
    

}
}
}


#endif // vtk_m_worklet_particleadvection_Particles_h
