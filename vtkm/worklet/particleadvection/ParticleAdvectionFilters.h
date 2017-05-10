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

#ifndef vtk_m_worklet_particleadvection_ParticleAdvectionFilters_h
#define vtk_m_worklet_particleadvection_ParticleAdvectionFilters_h

#include <vtkm/Types.h>
#include <vtkm/exec/ExecutionObjectBase.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm {
namespace worklet {
namespace particleadvection {

template <typename IntegratorType,
          typename FieldType,
          typename DeviceAdapterTag>
class ParticleAdvectWorklet : public vtkm::worklet::WorkletMapField
{
public:
    typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
    typedef typename FieldHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst FieldPortalConstType;
    
    typedef void ControlSignature(FieldIn<IdType> idx,
                                  ExecObject ic);
    typedef void ExecutionSignature(_1, _2);
    typedef _1 InputDomain;
        
    template<typename IntegralCurveType>
    VTKM_EXEC
    void operator()(const vtkm::Id &idx,
                    IntegralCurveType &ic) const
    {
        vtkm::Vec<FieldType, 3> p = ic.GetPos(idx);
        vtkm::Vec<FieldType, 3> p2;
        //vtkm::Vec<FieldType, 3> p0 = p;

        while (!ic.Done(idx))
        {
            if (integrator.Step(p, field, p2))
            {
                ic.TakeStep(idx, p2);
                p = p2;
            }
            else
            {
                ic.SetStatusOutOfBounds(idx);
            }
        }

        //std::cerr<<idx<<" DONE"<<std::endl;
        //p2 = ic.GetPos(idx);
        //std::cerr<<"PIC: "<<idx<<" "<<p0<<" --> "<<p2<<" #steps= "<<ic.GetStep(idx)<<std::endl;
    }
    
    ParticleAdvectWorklet(const IntegratorType &it,
                          const FieldPortalConstType &f) :
        integrator(it), field(f)
    {
    }
        
    IntegratorType integrator;
    FieldPortalConstType field;
};   

    

template <typename IntegratorType,
          typename FieldType,
          typename DeviceAdapterTag>
class ParticleAdvectionFilter
{
public:
    typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
    typedef typename FieldHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst FieldPortalConstType;
    typedef vtkm::worklet::particleadvection::ParticleAdvectWorklet<IntegratorType,
                                                                    FieldType,
                                                                    DeviceAdapterTag> ParticleAdvectWorkletType;
    
    ParticleAdvectionFilter(const IntegratorType &it,
                            const std::vector<vtkm::Vec<FieldType,3> > &pts,
                            const vtkm::cont::DataSet &_ds,
                            const vtkm::Id &nSteps) :
        integrator(it), seeds(pts), maxSteps(nSteps), ds(_ds)
    {
        vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3> > fieldArray;
        ds.GetField(0).GetData().CopyTo(fieldArray);
        field = fieldArray.PrepareForInput(DeviceAdapterTag());
    }

    ~ParticleAdvectionFilter(){}

    
    void run(bool dumpOutput=false)
    {
        typedef typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType> ParticleWorkletDispatchType;
        typedef vtkm::worklet::particleadvection::Particles<FieldType, DeviceAdapterTag> ParticleType;

        vtkm::Id numSeeds = seeds.size();
        vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > posArray = vtkm::cont::make_ArrayHandle(&seeds[0], numSeeds);
        std::vector<vtkm::Id> steps(numSeeds, 0), status(numSeeds,ParticleStatus::OK);
        vtkm::cont::ArrayHandle<vtkm::Id> stepArray = vtkm::cont::make_ArrayHandle(&steps[0], numSeeds);
        vtkm::cont::ArrayHandle<vtkm::Id> statusArray = vtkm::cont::make_ArrayHandle(&status[0], numSeeds);        
        vtkm::cont::ArrayHandleIndex idxArray(numSeeds);        

        ParticleType particles(posArray, stepArray, statusArray, maxSteps);

        ParticleAdvectWorkletType particleWorklet(integrator, field);        
        ParticleWorkletDispatchType particleWorkletDispatch(particleWorklet);
        
        particleWorkletDispatch.Invoke(idxArray, particles);

        if (dumpOutput)
        {
            for (vtkm::Id i = 0; i < numSeeds; i++)
            {
                vtkm::Vec<FieldType,3> p = particles.GetPos(i);
                std::cout<<p[0]<<" "<<p[1]<<" "<<p[2]<<std::endl;
            }
        }
    }

private:
    FieldPortalConstType field;
    vtkm::Id maxSteps;
    IntegratorType integrator;
    std::vector<vtkm::Vec<FieldType,3> > seeds;
    vtkm::cont::DataSet ds;
};


template <typename IntegratorType,
          typename FieldType,
          typename DeviceAdapterTag>
class StreamlineFilter
{
public:
    typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
    typedef typename FieldHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst FieldPortalConstType;
    typedef vtkm::worklet::particleadvection::ParticleAdvectWorklet<IntegratorType,
                                                                    FieldType,
                                                                    DeviceAdapterTag> ParticleAdvectWorkletType;
    
    StreamlineFilter(const IntegratorType &it,
                     const std::vector<vtkm::Vec<FieldType,3> > &pts,
                     const vtkm::cont::DataSet &_ds,
                     const vtkm::Id &nSteps,
                     const vtkm::Id stepsPerRound=-1) :
        integrator(it), seeds(pts), maxSteps(nSteps), ds(_ds), StepsPerRound(stepsPerRound)
    {
        vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3> > fieldArray;
        ds.GetField(0).GetData().CopyTo(fieldArray);
        field = fieldArray.PrepareForInput(DeviceAdapterTag());
    }

    ~StreamlineFilter(){}
    
    void run(bool dumpOutput=false)
    {
        typedef typename vtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType> ParticleWorkletDispatchType;
        typedef vtkm::worklet::particleadvection::StateRecordingParticles<FieldType, DeviceAdapterTag> StreamlineType;
        typedef vtkm::worklet::particleadvection::StateRecordingParticlesRound<FieldType, DeviceAdapterTag> StreamlineRoundType;

        vtkm::Id numSeeds = seeds.size();

        vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > posArray = vtkm::cont::make_ArrayHandle(&seeds[0], numSeeds);
        std::vector<vtkm::Id> steps(numSeeds, 0), status(numSeeds,ParticleStatus::OK);        
        vtkm::cont::ArrayHandle<vtkm::Id> stepArray = vtkm::cont::make_ArrayHandle(&steps[0], numSeeds);
        vtkm::cont::ArrayHandle<vtkm::Id> statusArray = vtkm::cont::make_ArrayHandle(&status[0], numSeeds);
        vtkm::cont::ArrayHandleIndex idxArray(numSeeds);

        ParticleAdvectWorkletType particleWorklet(integrator, field);        
        ParticleWorkletDispatchType particleWorkletDispatch(particleWorklet);

        //Brute force method, or rounds.
        if (StepsPerRound == -1)
        {
            StreamlineType streamlines(posArray, stepArray, statusArray, maxSteps);
            particleWorkletDispatch.Invoke(idxArray, streamlines);
            
            if (dumpOutput)
            {
                for (vtkm::Id i = 0; i < numSeeds; i++)
                {
                    vtkm::Id numSteps = streamlines.GetStep(i);
                    vtkm::Id status = streamlines.GetStatus(i);                    
                    for (vtkm::Id j = 0; j < numSteps; j++)
                    {
                        vtkm::Vec<FieldType,3> p = streamlines.GetHistory(i,j);
                        std::cout<<p[0]<<" "<<p[1]<<" "<<p[2]<<std::endl;
                    }
                }
            }
        }
        else
        {
            vtkm::Id numRounds = maxSteps / StepsPerRound;
            if (numRounds * StepsPerRound < maxSteps)
                numRounds++;
            
            vtkm::Id numSteps = 0, stepOffset = 0; 
            bool done = false;
            int round = 0;
            while (!done)
            {
                numSteps += StepsPerRound;
                if (numSteps >= maxSteps)
                {
                    numSteps = maxSteps;
                    done = true;
                }
                
                StreamlineRoundType streamlines(posArray, stepArray, statusArray,
                                                numSteps, StepsPerRound, stepOffset, maxSteps);
                particleWorkletDispatch.Invoke(idxArray, streamlines);

                auto historyPortal = streamlines.historyArray.GetPortalConstControl();
                if (dumpOutput)
                {
                    for (vtkm::Id i = 0; i < numSeeds; i++)
                    {
                        vtkm::Id numSteps = streamlines.GetStep(i);
                        vtkm::Id status = streamlines.GetStatus(i);
                        for (vtkm::Id j = stepOffset; j < numSteps; j++)
                        {
                            vtkm::Vec<FieldType,3> p = historyPortal.Get(i*StepsPerRound+(j-stepOffset));
                            std::cout<<p[0]<<" "<<p[1]<<" "<<p[2]<<std::endl;
                        }
                    }
                }
                round++;
                stepOffset += StepsPerRound;

            }
        }
    }

private:
    FieldPortalConstType field;
    vtkm::Id maxSteps;
    IntegratorType integrator;
    std::vector<vtkm::Vec<FieldType,3> > seeds;
    vtkm::Id StepsPerRound;

    vtkm::cont::DataSet ds;    
};

}
}
}

#endif // vtk_m_worklet_particleadvection_ParticleAdvectionFilters_h
    
