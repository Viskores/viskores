//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


#include <viskores/Math.h>
class ABCfield
{

public:
  void calculateVelocity(double* location, double t, double* velocity)
  {
    double ep = 0.25;
    double period = 1.0;

    double sinval = ep * viskores::Sin(period * t);

    velocity[0] = viskores::Sin(location[2] + sinval) + viskores::Cos(location[1] + sinval);
    velocity[1] = viskores::Sin(location[0] + sinval) + viskores::Cos(location[2] + sinval);
    velocity[2] = viskores::Sin(location[1] + sinval) + viskores::Cos(location[0] + sinval);
  }
};
