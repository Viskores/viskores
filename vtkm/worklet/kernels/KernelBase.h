#ifndef VTKM_KERNELBASE_HPP
#define VTKM_KERNELBASE_HPP

#include <vtkm/Math.h>
#include <vtkm/Types.h>

namespace vtkm { namespace worklet {
namespace kernels {

// Vector class used in the kernels
typedef vtkm::Vec<vtkm::Float64, 3> vector_type;
// Pi compatibility
#ifndef M_PI
  #define M_PI vtkm::Pi() 
#endif

// templated utility to generate expansions at compile time for x^N
template <int N>
double power(double x) { return x * power<N-1>(x); }

template <>
double power<0>(double) { return 1; }

//---------------------------------------------------------------------
// Base class for Kernels
// We use CRTP to avoid virtual function calls.
template <typename Kernel>
struct KernelBase
{
    //---------------------------------------------------------------------
    // Constructor
    // Calculate coefficients used repeatedly when evaluating the kernel
    // value or gradient
    // The smoothing length is usually denoted as 'h' in SPH literature
    KernelBase(double smoothingLength)     
      : smoothingLength_(smoothingLength) {};

    //---------------------------------------------------------------------
    // The functions below are placeholders which should be provided by
    // concrete implementations of this class.
    // The KernelBase versions will not be called when algorithms are
    // templated over a concrete implementation.
    //---------------------------------------------------------------------

    //---------------------------------------------------------------------
    // compute w(h) for the given distance
    inline double w(double distance) { 
      return static_cast<Kernel*>(this)->w(distance);
    }

    //---------------------------------------------------------------------
    // compute w(h) for the given squared distance
    // this version takes the distance squared as a convenience/optimization
    // but not all implementations will benefit from it
    inline double w2(double distance2) {
      return static_cast<Kernel*>(this)->w2(distance2);
    }

    //---------------------------------------------------------------------
    // compute w(h) for a variable h kernel
    // this is less efficient than the fixed radius version as coefficients
    // must be calculatd on the fly, but it is required when all particles
    // have different smoothing lengths
    inline double w(double h, double distance) { 
      return static_cast<Kernel*>(this)->w(h, distance);
    }

    //---------------------------------------------------------------------
    // compute w(h) for a variable h kernel using distance squared
    // this version takes the distance squared as a convenience/optimization
    inline double w2(double h, double distance2) {
      return static_cast<Kernel*>(this)->w2(h, distance2);
    }

    //---------------------------------------------------------------------
    // Calculates the kernel derivative for a distance {x,y,z} vector
    // from the centre.
    inline vector_type gradW(double distance, const vector_type& pos) {
      return static_cast<Kernel*>(this)->gradW(distance, pos);
    }

    // Calculates the kernel derivative at the given distance using a variable h value 
    // this is less efficient than the fixed radius version as coefficients
    // must be calculatd on the fly
    inline vector_type gradW(double h, double distance, const vector_type& pos) {
      return static_cast<Kernel*>(this)->gradW(h, distance, pos);
    }

    // return the multiplier between smoothing length and max cutoff distance 
    inline double getDilationFactor() const { 
      return static_cast<Kernel*>(this)->getDilationFactor;
    }

    // return the maximum cutoff distance over which the kernel acts,
    // beyond this distance the kernel value is zero
    inline double maxDistance() {
      return static_cast<Kernel*>(this)->maxDistance();
    }

    // return the maximum cutoff distance over which the kernel acts,
    // beyond this distance the kernel value is zero
    inline double maxDistanceSquared() {
      return static_cast<Kernel*>(this)->maxDistanceSquared();
    }

  protected:

    const double smoothingLength_;
};

}}}
#endif


