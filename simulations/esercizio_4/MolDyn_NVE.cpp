/****************************************************************
*****************************************************************
    _/    _/  _/_/_/  _/       Numerical Simulation Laboratory
   _/_/  _/ _/       _/       Physics Department
  _/  _/_/    _/    _/       Universita' degli Studi di Milano
 _/    _/       _/ _/       Prof. D.E. Galli
_/    _/  _/_/_/  _/_/_/_/ email: Davide.Galli@unimi.it
*****************************************************************
*****************************************************************/
#include <stdlib.h>     // srand, rand: to generate random number
#include <iostream>     // cin, cout: Standard Input/Output Streams Library
#include <fstream>      // Stream class to both read and write from/to files.
#include <cmath>        // rint, pow
#include <string>
#include <vector>
#include "MolDyn_NVE.h"

using namespace std;

int main(){
  Input(); // Inizialization
  int nconf = 1;
  for (int iblock = 1; iblock <= nblocks; ++iblock){
    ZeroAverages();
    if(iblock%10 == 0) cout << "Block: " << iblock << endl;

    for(int istep=1; istep <= nstep; ++istep){
      Move(); // Move particles with Verlet algorithm

      // if(istep%(iprint) == 0) {
      //   cout << "Bl, Ts : " << iblock << ", " << istep << endl;
      // }

      if(istep%10 == 0){ // Write actual configuration in XYZ format:
        // ConfXYZ(nconf); // Commented to avoid "filesystem full"!
        nconf += 1;
      }

      Measure(); // properties
      Accumulate(); // accumulate to compute averages

      if((istep==nstep-1)&&(iblock==nblocks)) ConfPenult();
    }
    ComputeAverages(iblock); // compute and output
  }
  ConfFinal(); // Write final configuration to restart
  return 0;
}

void Welcome(){ // prints welcome message
  cout << "Classic Lennard-Jones fluid        " << endl;
  cout << "Molecular dynamics simulation in NVE ensemble  " << endl << endl;
  cout << "Interatomic potential v(r) = 4 * [(1/r)^12 - (1/r)^6]" << endl << endl;
  cout << "The program uses Lennard-Jones units " << endl;

  cout << "Number of particles = " << npart << endl;
  cout << "Density of particles = " << rho << endl;
  cout << "Volume of the simulation box = " << vol << endl;
  cout << "Edge of the simulation box = " << box << endl;
  cout << "The program integrates Newton equations with the Verlet method " << endl;
  cout << "Time step = " << delta << endl;
  cout << "Number of steps = " << nstep << endl;
  cout << "Number of blocks = " << nblocks << endl;
  cout << "Restart = " << restart << endl;
  cout << "Rescale = " << rescale << endl;
  cout << endl;
}

void Input(){ //Prepare all stuff for the simulation
  ifstream ReadInput,ReadConf;
  double ep, ek, pr, et, vir;

  seed = 1;    //Set seed for random numbers
  srand(seed); //Initialize random number generator

  ReadInput.open("input.dat"); //Read input

  ReadInput >> temp;
  ReadInput >> npart;
  ReadInput >> rho;
  ReadInput >> rcut;
  ReadInput >> delta;
  ReadInput >> nstep;
  ReadInput >> nblocks;
  ReadInput >> iprint;
  ReadInput >> restart;
  ReadInput >> rescale;

  vol = (double)npart/rho;
  box = pow(vol,1.0/3.0);

  ReadInput.close();
  Welcome(); // Welcome message

  // Prepare array for measurements
  iv = 0; //Potential energy
  ik = 1; //Kinetic energy
  ie = 2; //Total energy
  it = 3; //Temperature
  n_props = 4; //Number of observables

  // Restart or rescale?
  if (restart==1) {
    Restart();
    if (rescale==1) {
      Rescale();
    }
  } else {
    cout << "Read initial configuration from file config.0 " << endl << endl;
    ReadConf.open("config.0");
    for (int i=0; i<npart; ++i){
      ReadConf >> x[i] >> y[i] >> z[i];
      x[i] = x[i] * box;
      y[i] = y[i] * box;
      z[i] = z[i] * box;
    }
    ReadConf.close();
    Prepare();
  }
}

/*========================NEW FUNCTIONS======================================*/

void Accumulate(){
  block_avg[iv] += stima_pot;
  block_avg[ik] += stima_kin;
  block_avg[it] += stima_temp;
  block_avg[ie] += stima_etot;
}

void ZeroAverages(){
  block_avg[iv] = 0;
  block_avg[ik] = 0;
  block_avg[it] = 0;
  block_avg[ie] = 0;
}

void ComputeAverages(int iblock){
  int j = iblock-1;

  ave_pot[j] = block_avg[iv] / nstep;
  av2_pot[j] = ave_pot[j] * ave_pot[j];

  ave_kin[j] = block_avg[ik] / nstep;
  av2_kin[j] = ave_kin[j] * ave_kin[j];

  ave_temp[j] = block_avg[it] / nstep;
  av2_temp[j] = ave_temp[j] * ave_temp[j];

  ave_etot[j] = block_avg[ie] / nstep;
  av2_etot[j] = ave_etot[j] * ave_etot[j];

  if (iblock==nblocks){
    cout << "Computing blocked statistics." << endl;
    BlockedStats("./avg_epot.out", ave_pot, av2_pot, nblocks);
    BlockedStats("./avg_ekin.out", ave_kin, av2_kin, nblocks);
    BlockedStats("./avg_temp.out", ave_temp, av2_temp, nblocks);
    BlockedStats("./avg_etot.out", ave_etot, av2_etot, nblocks);
  }
}

void BlockedStats(string filename, double AV[], double AV2[], int N){
  ofstream outFile;
  outFile.open(filename);

  vector<double> sum(N,0);
  vector<double> sum2(N,0);
  vector<double> err(N,0);

  for (int i=0; i<N; i++){
    for (int j=0; j<(i+1); j++){
      sum[i] += AV[j]; // cumulative sum of averages
      sum2[i] += AV2[j]; // sum of square averages
    }
    sum[i] /= (i+1); // cumulative average
    sum2[i] /= (i+1); // cumulative square average
    err[i] = Error(sum, sum2, i); // uncertainty
    outFile << sum[i] << " " << sum2[i] << " " << err[i] << endl;
  }
  outFile.close();
}

double Error(vector<double> AV, vector<double> AV2, int n){
  if (n == 0){
    return 0;
  } else {
    return sqrt( (AV2[n] - AV[n] * AV[n]) / n);
  }
}

void Restart(void){
  cout << "Restarting from previous configuration. " << endl << endl;
  ifstream ReadConf;
  ReadConf.open("old.final"); // final config
  for (int i=0; i<npart; ++i){
    ReadConf >> x[i] >> y[i] >> z[i];
    x[i] = x[i] * box; // LJ reduced units
    y[i] = y[i] * box;
    z[i] = z[i] * box;
  }
  ReadConf.close();
  ReadConf.open("old.penult"); // penultimate config
  for (int i=0; i<npart; ++i){
   ReadConf >> xold[i] >> yold[i] >> zold[i];
   xold[i] = xold[i] * box; // LJ reduced units
   yold[i] = yold[i] * box;
   zold[i] = zold[i] * box;
  }
  ReadConf.close();
}

void Rescale(void){
  double xnew, ynew, znew, fx[m_part], fy[m_part], fz[m_part];
  double sumv2 = 0, fs;
  cout << "Rescale temperature for quick equilibration." << endl << endl;

  for(int i=0; i<npart; ++i){ // Force acting on particle i
    fx[i] = Force(i,0);
    fy[i] = Force(i,1);
    fz[i] = Force(i,2);
  }

  for(int i=0; i<npart; ++i){ // Verlet integration scheme for r(t+dt)

    // 1. Compute r(t+dt) with Verlet algorithm
    xnew = Pbc( 2.0*x[i] - xold[i] + fx[i]*pow(delta,2) );
    ynew = Pbc( 2.0*y[i] - yold[i] + fy[i]*pow(delta,2) );
    znew = Pbc( 2.0*z[i] - zold[i] + fz[i]*pow(delta,2) );

    // 2. calculate velocity at v(t + dt/2)
    vx[i] = Pbc(xnew - x[i])/(delta);
    vy[i] = Pbc(ynew - y[i])/(delta);
    vz[i] = Pbc(znew - z[i])/(delta);

    // Calculate average square velocity
    sumv2 += vx[i]*vx[i] + vy[i]*vy[i] + vz[i]*vz[i];

    // Keep
    x[i] = xnew;
    y[i] = ynew;
    z[i] = znew;
  }

  sumv2 /= (double) npart;
  fs = sqrt(3*temp / sumv2); // T2/T1

  cout << "Desired temp: " << temp << endl;
  cout << "Computed temp: " << sumv2/3. << endl;
  cout << "Rescaling factor: " << fs << endl;

  for (int i = 0; i < npart; ++i){
    // 3. Rescale velocities according to desired temperature
    vx[i] *= fs;
    vy[i] *= fs;
    vz[i] *= fs;
    // 4. Estimate an OLD spatial config, r(t-dt)
    xold[i] = Pbc(x[i] - vx[i] * delta);
    yold[i] = Pbc(y[i] - vy[i] * delta);
    zold[i] = Pbc(z[i] - vz[i] * delta);
  }
}

void Prepare(void){
  cout << "Prepare random velocities with center of mass velocity equal to zero " << endl << endl;

  double sumv[3] = {0.0, 0.0, 0.0};
  for (int i=0; i<npart; ++i){
    vx[i] = rand()/double(RAND_MAX) - 0.5;
    vy[i] = rand()/double(RAND_MAX) - 0.5;
    vz[i] = rand()/double(RAND_MAX) - 0.5;
    sumv[0] += vx[i];
    sumv[1] += vy[i];
    sumv[2] += vz[i];
  }

  for (int idim=0; idim<3; ++idim) sumv[idim] /= (double)npart;
  double sumv2 = 0.0, fs;
  for (int i=0; i<npart; ++i){
    vx[i] = vx[i] - sumv[0];
    vy[i] = vy[i] - sumv[1];
    vz[i] = vz[i] - sumv[2];

    sumv2 += vx[i]*vx[i] + vy[i]*vy[i] + vz[i]*vz[i];
  }
  sumv2 /= (double)npart;

  fs = sqrt(3 * temp / sumv2);   // fs = velocity scale factor
  for (int i=0; i<npart; ++i){
    vx[i] *= fs;
    vy[i] *= fs;
    vz[i] *= fs;

    xold[i] = Pbc(x[i] - vx[i] * delta);
    yold[i] = Pbc(y[i] - vy[i] * delta);
    zold[i] = Pbc(z[i] - vz[i] * delta);
  }
  return;
}

void ConfPenult(void){ //Write final configuration
  ofstream WriteConf;

  // cout << "Print penultimate configuration to file old.penult " << endl;
  WriteConf.open("old.penult");

  for (int i=0; i<npart; ++i){
    WriteConf << x[i]/box << "   " <<  y[i]/box << "   " << z[i]/box << endl;
  }
  WriteConf.close();
  return;
}

/*========================OLD FUNCTIONS======================================*/

void Move(void){ //Move particles with Verlet algorithm
  double xnew, ynew, znew, fx[m_part], fy[m_part], fz[m_part];

  for(int i=0; i<npart; ++i){ //Force acting on particle i
    fx[i] = Force(i,0);
    fy[i] = Force(i,1);
    fz[i] = Force(i,2);
  }

  for(int i=0; i<npart; ++i){ //Verlet integration scheme

    xnew = Pbc( 2.0 * x[i] - xold[i] + fx[i] * pow(delta,2) );
    ynew = Pbc( 2.0 * y[i] - yold[i] + fy[i] * pow(delta,2) );
    znew = Pbc( 2.0 * z[i] - zold[i] + fz[i] * pow(delta,2) );

    vx[i] = Pbc(xnew - xold[i])/(2.0 * delta);
    vy[i] = Pbc(ynew - yold[i])/(2.0 * delta);
    vz[i] = Pbc(znew - zold[i])/(2.0 * delta);

    xold[i] = x[i];
    yold[i] = y[i];
    zold[i] = z[i];

    x[i] = xnew;
    y[i] = ynew;
    z[i] = znew;
  }
  return;
}

double Force(int ip, int idir){ //Compute forces as -Grad_ip V(r)
  double f=0.0;
  double dvec[3], dr;

  for (int i=0; i<npart; ++i){
    if(i != ip){
      dvec[0] = Pbc( x[ip] - x[i] );  // distance ip-i in pbc
      dvec[1] = Pbc( y[ip] - y[i] );
      dvec[2] = Pbc( z[ip] - z[i] );

      dr = dvec[0]*dvec[0] + dvec[1]*dvec[1] + dvec[2]*dvec[2];
      dr = sqrt(dr);

      if(dr < rcut){
        f += dvec[idir] * (48.0/pow(dr,14) - 24.0/pow(dr,8)); // -Grad_ip V(r)
      }
    }
  }

  return f;
}

void Measure(){ //Properties measurement
  int bin;
  double v, t, vij;
  double dx, dy, dz, dr;
  ofstream Epot, Ekin, Etot, Temp;

  Epot.open("output_epot.dat",ios::app);
  Ekin.open("output_ekin.dat",ios::app);
  Temp.open("output_temp.dat",ios::app);
  Etot.open("output_etot.dat",ios::app);

  v = 0.0; //reset observables
  t = 0.0;

  // cycle over pairs of particles
  for (int i=0; i<npart-1; ++i){
    for (int j=i+1; j<npart; ++j){

     dx = Pbc( xold[i] - xold[j] ); // here I use old configurations [old = r(t)]
     dy = Pbc( yold[i] - yold[j] ); // to be compatible with EKin which uses v(t)
     dz = Pbc( zold[i] - zold[j] ); // => EPot should be computed with r(t)

     dr = dx*dx + dy*dy + dz*dz;
     dr = sqrt(dr);

     if(dr < rcut){
       vij = 4.0/pow(dr,12) - 4.0/pow(dr,6);

    // Potential energy
       v += vij;
     }
    }
  }

  // Kinetic energy
  for (int i=0; i<npart; ++i) t += 0.5 * (vx[i]*vx[i] + vy[i]*vy[i] + vz[i]*vz[i]);

    stima_pot = v/(double)npart; //Potential energy per particle
    stima_kin = t/(double)npart; //Kinetic energy per particle
    stima_temp = (2.0 / 3.0) * t/(double)npart; //Temperature
    stima_etot = (t+v)/(double)npart; //Total energy per particle

    Epot << stima_pot  << endl;
    Ekin << stima_kin  << endl;
    Temp << stima_temp << endl;
    Etot << stima_etot << endl;

    Epot.close();
    Ekin.close();
    Temp.close();
    Etot.close();

    return;
}

void ConfFinal(void){ //Write final configuration
  ofstream WriteConf;

  cout << "Print final configuration to file old.final " << endl << endl;
  WriteConf.open("old.final");

  for (int i=0; i<npart; ++i){
    WriteConf << x[i]/box << "   " <<  y[i]/box << "   " << z[i]/box << endl;
  }
  WriteConf.close();
  return;
}

void ConfXYZ(int nconf){ //Write configuration in .xyz format
  ofstream WriteXYZ;

  WriteXYZ.open("frames/config_" + to_string(nconf) + ".xyz");
  WriteXYZ << npart << endl;
  WriteXYZ << "This is only a comment!" << endl;
  for (int i=0; i<npart; ++i){
    WriteXYZ << "LJ  " << Pbc(x[i]) << "   " <<  Pbc(y[i]) << "   " << Pbc(z[i]) << endl;
  }
  WriteXYZ.close();
}

double Pbc(double r){
  // Algorithm for periodic boundary conditions with side L=box
  return r - box * rint(r/box);
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