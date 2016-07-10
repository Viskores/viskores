#include <iostream>

const double hm4_44[9] = {
  0.037828455507264,
  -0.023849465019557,
  -0.110624404418437,
  0.377402855612831,
  0.852698679008894,
  0.377402855612831,
  -0.110624404418437,
  -0.023849465019557,
  0.037828455507264
};

const double h4[9] = {
  0.0,
  -0.064538882628697,
  -0.040689417609164,
  0.418092273221617,
  0.788485616405583,
  0.418092273221617,
  -0.0406894176091641,
  -0.0645388826286971,
  0.0
};

void forward_xform (
  const double *sigIn, size_t sigInLen,
  const double *low_filter, const double *high_filter,
  int filterLen, double *cA, double *cD, bool oddlow, bool oddhigh) 
{

  size_t xlstart = oddlow ? 1 : 0;
  size_t xl;
  size_t xhstart = oddhigh ? 1 : 0;
  size_t xh;

  for (size_t yi = 0; yi < sigInLen; yi += 2) {
    cA[yi>>1] = cD[yi>>1] = 0.0;

    xl = xlstart;
    xh = xhstart;

    for (int k = filterLen - 1; k >= 0; k--) {
      cA[yi>>1] += low_filter[k]  * sigIn[xl++];
      cD[yi>>1] += high_filter[k] * sigIn[xh++];
    }
    xlstart+=2;
    xhstart+=2;
  }

}

void print_coeffs( const double* cA, const double* cD, size_t num )
{
	for( size_t i = 0; i < num; i++ )
	{
		std::cout << cA[i] << ",  " << cD[i] << std::endl;
	}
}

void create_array( double* buf, size_t num )
{
   for( size_t i = 0; i < num; i++ )
    buf[i] = i + 1;
}

int main( int argc, char* argv[] )
{
  size_t sigLen = 20;
  double buf[ sigLen+8 ];
  create_array( buf, sigLen+8 );

  size_t coeffLen = 10;
  double cA[coeffLen];
  double cD[coeffLen];
	for( size_t i = 0; i < coeffLen; i++ )
		cA[i] = cD[i] = 3.14159265;

  const double* low_filter = hm4_44;
  const double* high_filter = h4;

  bool oddlow = false;
  bool oddhigh = true;
  

  forward_xform( buf, sigLen, low_filter, high_filter, 9, cA, cD, oddlow, oddhigh );

  print_coeffs( cA, cD, coeffLen );	
}
