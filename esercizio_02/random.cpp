/****************************************************************
*****************************************************************
    _/    _/  _/_/_/  _/       Numerical Simulation Laboratory
   _/_/  _/ _/       _/       Physics Department
  _/  _/_/    _/    _/       Universita' degli Studi di Milano
 _/    _/       _/ _/       Prof. D.E. Galli
_/    _/  _/_/_/  _/_/_/_/ email: Davide.Galli@unimi.it
*****************************************************************
*****************************************************************/

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include "random.h"

using namespace std;

Random :: Random(){}

Random :: ~Random(){}

void Random :: SaveSeed(){
   ofstream WriteSeed;
   WriteSeed.open("seed.out");
   if (WriteSeed.is_open()){
      WriteSeed << l1 << " " << l2 << " " << l3 << " " << l4 << endl;;
   } else cerr << "PROBLEM: Unable to open random.out" << endl;
  WriteSeed.close();
  return;
}

int Random :: Six_Sided_Die(){
  double r;
  int die;
  r = Rannyu(); // r ~ U[0,1)
  if (r < 1./6){
    die = 1;
  } else if (r<2./6 && r>=1./6){
    die = 2;
  } else if (r<3./6 && r>=2./6){
    die = 3;
  } else if (r<4./6 && r>=3./6){
    die = 4;
  } else if (r<5./6 && r>=4./6){
    die = 5;
  } else {
    die = 6;
  }
  return die;
}

double Random :: ImportanceSample(){
  return 1 - sqrt(1-Rannyu());
}

double Random :: UnImportanceSample(){
  return -sqrt(Rannyu());
}


double Random :: RanTheta3d(){
  return acos ( 1 - 2*Rannyu() );
}

double Random :: RanTheta(){
  double x,y,r,theta;
  while (true){
    // random radius
    x = Rannyu(-1,1);
    y = Rannyu(-1,1);
    r = x*x + y*y;
    // if radius is in unit circle, calculate theta
    if (r<=1){
      theta = 2 * acos (x / sqrt (r) );
      return theta;
    } else {
        x = Rannyu(-1,1);
        y = Rannyu(-1,1);
        r = x*x + y*y;
    }
  }
}


double Random :: sum_uniform(int M, double min, double max){
   double sum = 0;
   for (int j = 0; j < M; j++){
      sum += Rannyu(min,max);
   }
   sum /= M;
   return sum;
}

double Random :: sum_exponential(int M, double lambda){
   double sum = 0;
   for (int j = 0; j < M; j++){
      sum += Exponential(1);
   }
   sum /= M;
   return sum;
}

double Random :: sum_lorentzian(int M, double mean, double gamma){
   double sum = 0;
   for (int j = 0; j < M; j++){
      sum += Lorentzian(mean, gamma);
   }
   sum /= M;
   return sum;
}

double Random :: Lorentzian(double mean, double gamma) {
  double r;
  r = gamma * tan(M_PI * (Rannyu() - 0.5) ) + mean;
  return r;
}

double Random :: Exponential(double lambda) {
  double r;
  r = - log(1-Rannyu()) / lambda;
  return r;
}

double Random :: Gauss(double mean, double sigma) {
   double s=Rannyu();
   double t=Rannyu();
   double x=sqrt(-2.*log(1.-s))*cos(2.*M_PI*t);
   return mean + x * sigma;
}

double Random :: Rannyu(double min, double max){
   return min+(max-min)*Rannyu();
}

double Random :: Rannyu(void){
  const double twom12=0.000244140625;
  int i1,i2,i3,i4;
  double r;

  i1 = l1*m4 + l2*m3 + l3*m2 + l4*m1 + n1;
  i2 = l2*m4 + l3*m3 + l4*m2 + n2;
  i3 = l3*m4 + l4*m3 + n3;
  i4 = l4*m4 + n4;
  l4 = i4%4096;
  i3 = i3 + i4/4096;
  l3 = i3%4096;
  i2 = i2 + i3/4096;
  l2 = i2%4096;
  l1 = (i1 + i2/4096)%4096;
  r=twom12*(l1+twom12*(l2+twom12*(l3+twom12*(l4))));

  return r;
}

void Random :: SetRandom(int * s, int p1, int p2){
  m1 = 502;
  m2 = 1521;
  m3 = 4071;
  m4 = 2107;
  l1 = s[0]%4096;
  l2 = s[1]%4096;
  l3 = s[2]%4096;
  l4 = s[3]%4096;
  l4 = 2*(l4/2)+1;
  n1 = 0;
  n2 = 0;
  n3 = p1;
  n4 = p2;

  return;
}

/****************************************************************
*****************************************************************
    _/    _/  _/_/_/  _/       Numerical Simulation Laboratory
   _/_/  _/ _/       _/       Physics Department
  _/  _/_/    _/    _/       Universita' degli Studi di Milano
 _/    _/       _/ _/       Prof. D.E. Galli
_/    _/  _/_/_/  _/_/_/_/ email: Davide.Galli@unimi.it
*****************************************************************
*****************************************************************/
