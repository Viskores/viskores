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
#ifndef VTKM_KERNEL_GAUSSIAN_H
#define VTKM_KERNEL_GAUSSIAN_H

#include "KernelBase.h"

//
// Gaussian kernel.
// Compact support is achived by truncating the kernel beyond the cutoff radius
// This implementation uses a factor of 5 between smoothing length and cutoff
//

namespace vtkm { namespace worklet {
namespace kernels {

template <int Dimensions>
struct Gaussian : public KernelBase< Gaussian<Dimensions> >
{
    //---------------------------------------------------------------------
    // Constructor
    // Calculate coefficients used repeatedly when evaluating the kernel
    // value or gradient
    Gaussian(double smoothingLength)
    : KernelBase< Gaussian<Dimensions> >(smoothingLength)
      {
        Hinverse_   = 1.0/smoothingLength;
        Hinverse2_  = Hinverse_*Hinverse_;
        maxRadius_  = 5.0*smoothingLength;
        maxRadius2_ = maxRadius_*maxRadius_;
        //
        norm_        = 1.0 / pow(M_PI, static_cast<double>(Dimensions) / 2.0);
        scale_W_     = norm_ * power<Dimensions>  (Hinverse_);
        scale_GradW_ = - 2.0 * power<Dimensions+1>(Hinverse_) / norm_;
      }

    //---------------------------------------------------------------------
    // return the multiplier between smoothing length and max cutoff distance
    /*constexpr */ double getDilationFactor() const { return 5.0; }

    //---------------------------------------------------------------------
    // compute w(h) for the given distance
    inline double w(double distance) const
    {
        if (distance<maxDistance()) {
            // compute r/h
            double normedDist = distance * Hinverse_;
            // compute w(h)
            return scale_W_ * exp(-normedDist * normedDist);
        }
        return 0.0;
    }

    //---------------------------------------------------------------------
    // compute w(h) for the given squared distance
    inline double w2(double distance2) const
    {
        if (distance2<maxSquaredDistance()) {
            // compute (r/h)^2
            double normedDist = distance2 * Hinverse2_;
            // compute w(h)
            return scale_W_ * exp(-normedDist);
        }
        return 0.0;
    }

    //---------------------------------------------------------------------
    // compute w(h) for a variable h kernel
    inline double w(double h, double distance) const
    {
        if (distance<maxDistance(h)) {
            double Hinverse = 1.0/h;
            double scale_W = norm_ * power<Dimensions>(Hinverse);
            double Q = distance * Hinverse;

            return scale_W * exp(-Q*Q);
        }
        return 0;
    }

    //---------------------------------------------------------------------
    // compute w(h) for a variable h kernel using distance squared
    inline double w2(double h, double distance2) const
    {
        if (distance2<maxSquaredDistance(h)) {
            double Hinverse = 1.0/h;
            double scale_W = norm_ * power<Dimensions>(Hinverse);
            double Q = distance2 * Hinverse * Hinverse;

            return scale_W * exp(-Q);
        }
        return 0;
    }

    //---------------------------------------------------------------------
    // Calculates the kernel derivative for a distance {x,y,z} vector
    // from the centre
    inline vector_type gradW(double distance, const vector_type& pos) const
    {
        double Q = distance * Hinverse_;
        if (Q != 0.0)
        {
            return scale_GradW_ * exp(-Q * Q) * pos;
        }
        else {
            return vector_type(0.0);
        }
    }

    //---------------------------------------------------------------------
    // Calculates the kernel derivative for a distance {x,y,z} vector
    // from the centre using a variable h
    inline vector_type gradW(double h, double distance, const vector_type& pos) const
    {
        double Hinverse = 1.0/h;
        double scale_GradW = - 2.0 * power<Dimensions+1>(Hinverse)
                  / pow(M_PI, static_cast<double>(Dimensions) / 2.0);
        double Q = distance * Hinverse;

        //!!! check this due to the fitting offset
        if (distance != 0.0)
        {
            return scale_GradW * exp(-Q * Q) * pos;
        }
        else {
            return vector_type(0.0);
        }
    }

    //---------------------------------------------------------------------
    // return the maximum distance at which this kernel is non zero 
    inline double maxDistance() const
    {
        return maxRadius_;
    }

    //---------------------------------------------------------------------
    // return the maximum distance at which this variable h kernel is non zero
    inline double maxDistance(double h) const
    {
        return getDilationFactor()*h;
    }

    //---------------------------------------------------------------------
    // return the maximum distance at which this kernel is non zero 
    inline double maxSquaredDistance() const
    {
        return maxRadius2_;
    }

    //---------------------------------------------------------------------
    // return the maximum distance at which this kernel is non zero 
    inline double maxSquaredDistance(double h) const
    {
        return power<2>(getDilationFactor())*h*h;
    }

private:
    double norm_;
    double Hinverse_;
    double Hinverse2_;
    double maxRadius_;
    double maxRadius2_;
    double scale_W_;
    double scale_GradW_;
};

}}}

#endif
