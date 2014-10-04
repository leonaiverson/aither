#include "inviscidFlux.h"
#include <cmath> //sqrt

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using std::string;
using std::max;
using std::copysign;

//constructor -- initialize flux from state vector
inviscidFlux::inviscidFlux( const primVars &state, const idealGas &eqnState, const vector3d<double>& areaVec){
  vector3d<double> normArea = areaVec / areaVec.Mag();
  vector3d<double> vel = state.Velocity();

  rhoVel  = state.Rho() * vel.DotProd(normArea);      
  rhoVelU = state.Rho() * vel.DotProd(normArea) * vel.X() + state.P() * normArea.X();     
  rhoVelV = state.Rho() * vel.DotProd(normArea) * vel.Y() + state.P() * normArea.Y();     
  rhoVelW = state.Rho() * vel.DotProd(normArea) * vel.Z() + state.P() * normArea.Z();     
  rhoVelH = state.Rho() * vel.DotProd(normArea) * state.Enthalpy(eqnState);
}

//constructor -- initialize flux from state vector using conservative variables
inviscidFlux::inviscidFlux( const colMatrix &cons, const idealGas &eqnState, const vector3d<double>& areaVec){

  //check to see that colMatrix is correct size
  if (cons.Size() != 5){
    cerr << "ERROR: Error in inviscidFlux::inviscidFlux. Column matrix of conservative variables is not the correct size!" << endl;
    exit(0);
  }

  primVars state;
  state.SetRho(cons.Data(0));
  state.SetU(cons.Data(1)/cons.Data(0));
  state.SetV(cons.Data(2)/cons.Data(0));
  state.SetW(cons.Data(3)/cons.Data(0));
  double energy = cons.Data(4)/cons.Data(0);
  state.SetP(eqnState.GetPressFromEnergy(state.Rho(), energy, state.Velocity().Mag() ));

  vector3d<double> normArea = areaVec / areaVec.Mag();
  vector3d<double> vel = state.Velocity();

  rhoVel  = state.Rho() * vel.DotProd(normArea);      
  rhoVelU = state.Rho() * vel.DotProd(normArea) * vel.X() + state.P() * normArea.X();     
  rhoVelV = state.Rho() * vel.DotProd(normArea) * vel.Y() + state.P() * normArea.Y();     
  rhoVelW = state.Rho() * vel.DotProd(normArea) * vel.Z() + state.P() * normArea.Z();     
  rhoVelH = state.Rho() * vel.DotProd(normArea) * state.Enthalpy(eqnState);
}

void inviscidFlux::SetFlux( const primVars &state, const idealGas &eqnState, const vector3d<double>& areaVec){
  vector3d<double> normArea = areaVec / areaVec.Mag();
  vector3d<double> vel = state.Velocity();

  rhoVel  = state.Rho() * vel.DotProd(normArea);      
  rhoVelU = state.Rho() * vel.DotProd(normArea) * vel.X() + state.P() * normArea.X();     
  rhoVelV = state.Rho() * vel.DotProd(normArea) * vel.Y() + state.P() * normArea.Y();     
  rhoVelW = state.Rho() * vel.DotProd(normArea) * vel.Z() + state.P() * normArea.Z();     
  rhoVelH = state.Rho() * vel.DotProd(normArea) * state.Enthalpy(eqnState);
}


//function to calculate pressure from conserved variables and equation of state
inviscidFlux RoeFlux( const primVars &left, const primVars &right, const idealGas &eqnState, const vector3d<double>& areaVec, double &maxWS){

  //compute Rho averaged quantities
  double denRatio = sqrt(right.Rho()/left.Rho());
  double rhoR = left.Rho() * denRatio;  //Roe averaged density
  double uR = (left.U() + denRatio * right.U()) / (1.0 + denRatio);  //Roe averaged u-velocity
  double vR = (left.V() + denRatio * right.V()) / (1.0 + denRatio);  //Roe averaged v-velocity
  double wR = (left.W() + denRatio * right.W()) / (1.0 + denRatio);  //Roe averaged w-velocity
  double hR = (left.Enthalpy(eqnState) + denRatio * right.Enthalpy(eqnState)) / (1.0 + denRatio);  //Roe averaged total enthalpy
  double aR = sqrt( (eqnState.Gamma() - 1.0) * (hR - 0.5 * (uR*uR + vR*vR + wR*wR)) );  //Roe averaged speed of sound
  //Roe averaged face normal velocity
  vector3d<double> velR(uR,vR,wR);

  //cout << "Roe velocity " << velR << endl;

  vector3d<double> areaNorm = areaVec / areaVec.Mag();
  double velRSum = velR.DotProd(areaNorm);

  //calculate wave strengths
  // double denDiff = right.Rho() - left.Rho();
  // double pressDiff = right.Pressure(eqnState) - left.Pressure(eqnState);

  double normVelDiff = right.Velocity().DotProd(areaNorm) - left.Velocity().DotProd(areaNorm);

  //vector<double> waveStrength(4);
  double waveStrength[4] = {((right.P() - left.P()) - rhoR * aR * normVelDiff) / (2.0 * aR * aR), 
			    (right.Rho() - left.Rho()) - (right.P() - left.P()) / (aR * aR), 
			    ((right.P() - left.P()) + rhoR * aR * normVelDiff) / (2.0 * aR * aR), 
			    rhoR};

  // waveStrength[0] = (pressDiff - rhoR * aR * normVelDiff) / (2.0 * aR * aR);       //left moving acoustic wave strength
  // waveStrength[1] = denDiff - pressDiff / (aR * aR);                         //entropy wave strength
  // waveStrength[2] = (pressDiff + rhoR * aR * normVelDiff) / (2.0 * aR * aR);        //right moving acoustic wave strength
  // waveStrength[3] = rhoR;                                                    // shear waves get combined into one factor


  //calculate absolute value of wave speeds
  //vector<double> waveSpeed(4);
  double waveSpeed[4] = {fabs(velRSum - aR), 
			 fabs(velRSum), 
			 fabs(velRSum + aR), 
			 fabs(velRSum)};

  // waveSpeed[0] = fabs(velRSum - aR);                                          //left moving acoustic wave speed
  // waveSpeed[1] = fabs(velRSum);                                               //entropy wave speed
  // waveSpeed[2] = fabs(velRSum + aR);                                          //right moving acoustic wave speed
  // waveSpeed[3] = fabs(velRSum);                                               //shear wave speed

  //calculate entropy fix
  double entropyFix = 0.1;                                                            // default setting for entropy fix to kick in

  if ( waveSpeed[0] < entropyFix ){
    waveSpeed[0] = 0.5 * (waveSpeed[0] * waveSpeed[0] / entropyFix + entropyFix);
  }
  if ( waveSpeed[2] < entropyFix ){
    waveSpeed[2] = 0.5 * (waveSpeed[2] * waveSpeed[2] / entropyFix + entropyFix);
  }

  maxWS = fabs(velRSum) + aR;

  //calculate eigenvectors
  // vector<double> lAcousticEigV(5);
  // lAcousticEigV[0] = 1.0;
  // lAcousticEigV[1] = uR - aR * areaNorm.X();
  // lAcousticEigV[2] = vR - aR * areaNorm.Y();
  // lAcousticEigV[3] = wR - aR * areaNorm.Z();
  // lAcousticEigV[4] = hR - aR * velRSum;

  double lAcousticEigV[5] = {1.0, 
			     uR - aR * areaNorm.X(), 
			     vR - aR * areaNorm.Y(), 
			     wR - aR * areaNorm.Z(), 
			     hR - aR * velRSum};

  // vector<double> entropyEigV(5);
  // entropyEigV[0] = 1.0;
  // entropyEigV[1] = uR;
  // entropyEigV[2] = vR;
  // entropyEigV[3] = wR;
  // entropyEigV[4] = 0.5 * ( uR * uR + vR * vR + wR * wR);

  double entropyEigV[5] = {1.0, 
			   uR, 
			   vR, 
			   wR, 
			   0.5 * ( uR * uR + vR * vR + wR * wR)};

  // vector<double> rAcousticEigV(5);
  // rAcousticEigV[0] = 1.0;
  // rAcousticEigV[1] = uR + aR * areaNorm.X();
  // rAcousticEigV[2] = vR + aR * areaNorm.Y();
  // rAcousticEigV[3] = wR + aR * areaNorm.Z();
  // rAcousticEigV[4] = hR + aR * velRSum;

  double rAcousticEigV[5] = {1.0, 
			     uR + aR * areaNorm.X(), 
			     vR + aR * areaNorm.Y(), 
			     wR + aR * areaNorm.Z(), 
			     hR + aR * velRSum};

  // vector<double> shearEigV(5);
  // shearEigV[0] = 0.0;
  // shearEigV[1] = (right.Vx() - left.Vx()) - normVelDiff * areaNorm.X();
  // shearEigV[2] = (right.Vy() - left.Vy()) - normVelDiff * areaNorm.Y();
  // shearEigV[3] = (right.Vz() - left.Vz()) - normVelDiff * areaNorm.Z();
  // shearEigV[4] = uR * (right.Vx() - left.Vx()) + vR * (right.Vy() - left.Vy()) + 
  //                wR * (right.Vz() - left.Vz()) - velRSum * normVelDiff;

  double shearEigV[5] = {0.0, 
			 (right.U() - left.U()) - normVelDiff * areaNorm.X(), 
			 (right.V() - left.V()) - normVelDiff * areaNorm.Y(), 
			 (right.W() - left.W()) - normVelDiff * areaNorm.Z(), 
			 uR * (right.U() - left.U()) + vR * (right.V() - left.V()) + wR * (right.W() - left.W()) - velRSum * normVelDiff};

  //calculate dissipation term ( eigenvector * wave speed * wave strength)
  //vector<double> dissipation(5,0.0);

  double dissipation[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
  unsigned int ii = 0;
  for ( ii=0; ii < 5; ii++ ) {                                    
    dissipation[ii] = waveSpeed[0] * waveStrength[0] * lAcousticEigV[ii]   //contribution from left acoustic wave
                    + waveSpeed[1] * waveStrength[1] * entropyEigV[ii]     //contribution from entropy wave
                    + waveSpeed[2] * waveStrength[2] * rAcousticEigV[ii]   //contribution from right acoustic wave
                    + waveSpeed[3] * waveStrength[3] * shearEigV[ii];      //contribution from shear wave
  }


  //cout << "Roe Dissipation term " << dissipation[0] << ", " << dissipation[1] << ", " << dissipation[2] << ", " << dissipation[3] << ", " << dissipation[4] << endl;

  //calculate left/right physical flux
  inviscidFlux leftFlux(left, eqnState, areaNorm);
  inviscidFlux rightFlux(right, eqnState, areaNorm);

  //calculate numerical flux
  inviscidFlux flux;
  flux.SetRhoVel(  0.5 * (leftFlux.RhoVel()  + rightFlux.RhoVel()  - dissipation[0]) );
  flux.SetRhoVelU( 0.5 * (leftFlux.RhoVelU() + rightFlux.RhoVelU() - dissipation[1]) );
  flux.SetRhoVelV( 0.5 * (leftFlux.RhoVelV() + rightFlux.RhoVelV() - dissipation[2]) );
  flux.SetRhoVelW( 0.5 * (leftFlux.RhoVelW() + rightFlux.RhoVelW() - dissipation[3]) );
  flux.SetRhoVelH( 0.5 * (leftFlux.RhoVelH() + rightFlux.RhoVelH() - dissipation[4]) );

  return flux;

}

//function to calculate exact Roe flux jacobians
void RoeFluxJacobian( const primVars &left, const primVars &right, const idealGas &eqnState, const vector3d<double>& areaVec, double &maxWS, squareMatrix &dF_dUl, squareMatrix &dF_dUr){

  //left --> primative variables from left side
  //right --> primative variables from right side
  //eqnStat --> ideal gas equation of state
  //areaVec --> face area vector
  //maxWS --> maximum wave speed
  //dF_dUl --> dF/dUl, derivative of the Roe flux wrt the left state (conservative variables)
  //dF_dUr --> dF/dUlr, derivative of the Roe flux wrt the right state (conservative variables)


  //check to see that output matricies are correct size
  if( (dF_dUl.Size() != 5) || (dF_dUr.Size() != 5)){
    cerr << "ERROR: Input matricies to RoeFLuxJacobian function are not the correct size!" << endl;
  }

  //compute Rho averaged quantities
  double denRatio = sqrt(right.Rho()/left.Rho());
  double rhoR = left.Rho() * denRatio;  //Roe averaged density
  double uR = (left.U() + denRatio * right.U()) / (1.0 + denRatio);  //Roe averaged u-velocity
  double vR = (left.V() + denRatio * right.V()) / (1.0 + denRatio);  //Roe averaged v-velocity
  double wR = (left.W() + denRatio * right.W()) / (1.0 + denRatio);  //Roe averaged w-velocity
  double hR = (left.Enthalpy(eqnState) + denRatio * right.Enthalpy(eqnState)) / (1.0 + denRatio);  //Roe averaged total enthalpy
  double aR = sqrt( (eqnState.Gamma() - 1.0) * (hR - 0.5 * (uR*uR + vR*vR + wR*wR)) );  //Roe averaged speed of sound
  //Roe averaged face normal velocity
  vector3d<double> velR(uR,vR,wR);

  vector3d<double> areaNorm = areaVec / areaVec.Mag();  //normalize area vector to unit vector

  //dot product of velocities (Roe, left, right) with unit area vector
  double velRSum = velR.DotProd(areaNorm);
  double velLeftSum = left.Velocity().DotProd(areaNorm);
  double velRightSum = right.Velocity().DotProd(areaNorm);

  //calculate differences: normal velocity, pressure, u-velocity, v-velocity, w-velocity
  double normVelDiff = right.Velocity().DotProd(areaNorm) - left.Velocity().DotProd(areaNorm);
  double pDiff = right.P() - left.P();
  double uDiff = right.U() - left.U();
  double vDiff = right.V() - left.V();
  double wDiff = right.W() - left.W();

  //calculate wave strengths
  double waveStrength[4] = {(pDiff - rhoR * aR * normVelDiff) / (2.0 * aR * aR), 
			    (right.Rho() - left.Rho()) - pDiff / (aR * aR), 
			    (pDiff + rhoR * aR * normVelDiff) / (2.0 * aR * aR), 
			    rhoR};

  //left moving acoustic wave speed, entropy wave speed, right moving acoustic wave speed, shear wave speed
  double waveSpeed[4] = {fabs(velRSum - aR), 
			 fabs(velRSum), 
			 fabs(velRSum + aR), 
			 fabs(velRSum)};

  //calculate entropy fix
  //Harten JCP 1983
  double entropyFix = 0.1;                                                            // default setting for entropy fix to kick in

  if ( waveSpeed[0] < entropyFix ){
    waveSpeed[0] = 0.5 * (waveSpeed[0] * waveSpeed[0] / entropyFix + entropyFix);
  }
  if ( waveSpeed[2] < entropyFix ){
    waveSpeed[2] = 0.5 * (waveSpeed[2] * waveSpeed[2] / entropyFix + entropyFix);
  }

  //maxWS = fabs(velRSum) + aR; //maxWS will be calculated in RHS (explicit)

  //right eigenvectors
  double lAcousticEigV[5] = {1.0, 
			     uR - aR * areaNorm.X(), 
			     vR - aR * areaNorm.Y(), 
			     wR - aR * areaNorm.Z(), 
			     hR - aR * velRSum};

  double entropyEigV[5] = {1.0, 
			   uR, 
			   vR, 
			   wR, 
			   0.5 * velR.MagSq()};

  double rAcousticEigV[5] = {1.0, 
			     uR + aR * areaNorm.X(), 
			     vR + aR * areaNorm.Y(), 
			     wR + aR * areaNorm.Z(), 
			     hR + aR * velRSum};

  double shearEigV[5] = {0.0, 
			 uDiff - normVelDiff * areaNorm.X(), 
			 vDiff - normVelDiff * areaNorm.Y(), 
			 wDiff - normVelDiff * areaNorm.Z(), 
			 uR * uDiff + vR * vDiff + wR * wDiff - velRSum * normVelDiff};

  //begin jacobian calculation ////////////////////////////////////////////////////////////////////////////////////////

  //derivative of Roe flux wrt left conservative variables
  dF_dUl.Zero();

  //Roe flux --> F = 0.5 * (F(Ul) + F(Ur)) - 0.5 * |lambda| * LdU * r
  //take derivative of each term wrt to Ul and add to flux jacobian variable

  //calculate flux derivatives --- d(0.5*F(Ul))/d(Ul)
  //column zero
  dF_dUl.SetData(0, 0, 0.0);
  dF_dUl.SetData(1, 0, 0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() * areaNorm.X() - left.U() * velLeftSum);
  dF_dUl.SetData(2, 0, 0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() * areaNorm.Y() - left.V() * velLeftSum);
  dF_dUl.SetData(3, 0, 0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() * areaNorm.Z() - left.W() * velLeftSum);
  dF_dUl.SetData(4, 0, (0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() - left.Enthalpy(eqnState)) * velLeftSum); 
		       
  //column one
  dF_dUl.SetData(1, 0, areaNorm.X());
  dF_dUl.SetData(1, 1, left.U() * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * areaNorm.X() + velLeftSum);
  dF_dUl.SetData(1, 2, left.V() * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * areaNorm.Y());
  dF_dUl.SetData(1, 3, left.W() * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * areaNorm.Z());
  dF_dUl.SetData(1, 4, left.Enthalpy(eqnState) * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * velLeftSum);

  //column two
  dF_dUl.SetData(2, 0, areaNorm.Y());
  dF_dUl.SetData(2, 1, left.U() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * areaNorm.X());
  dF_dUl.SetData(2, 2, left.V() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * areaNorm.Y() + velLeftSum);
  dF_dUl.SetData(2, 3, left.W() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * areaNorm.Z());
  dF_dUl.SetData(2, 4, left.Enthalpy(eqnState) * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * velLeftSum);

  //column three
  dF_dUl.SetData(3, 0, areaNorm.Z());
  dF_dUl.SetData(3, 1, left.U() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * areaNorm.X());
  dF_dUl.SetData(3, 2, left.V() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * areaNorm.Y());
  dF_dUl.SetData(3, 3, left.W() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * areaNorm.Z() + velLeftSum);
  dF_dUl.SetData(3, 4, left.Enthalpy(eqnState) * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * velLeftSum);

  //column four
  dF_dUl.SetData(4, 0, 0.0);
  dF_dUl.SetData(4, 1, (eqnState.Gamma() - 1.0) * areaNorm.X());
  dF_dUl.SetData(4, 2, (eqnState.Gamma() - 1.0) * areaNorm.Y());
  dF_dUl.SetData(4, 3, (eqnState.Gamma() - 1.0) * areaNorm.Z());
  dF_dUl.SetData(4, 4, eqnState.Gamma() * velLeftSum);

  //multiply by factor 1/2
  dF_dUl = dF_dUl * 0.5;

  double one_rl_r = 1.0 / (left.Rho() + rhoR);

  //calculate derivatives of Roe quantities
  //derivative of normal Roe velocity wrt left conservative variables
  double dQr_dUl[5] = { -0.5 * (velLeftSum + velRSum) * one_rl_r,
			areaNorm.X() * one_rl_r,
			areaNorm.Y() * one_rl_r,
			areaNorm.Z() * one_rl_r,
			0.0};

  //derivative of absolute value of Roe velocity wrt left conservative variables
  double dAbsQr_dUl[5] = { -0.5 * copysign(1.0,velRSum) * (velLeftSum + velRSum) * one_rl_r,
			   copysign(1.0,velRSum) * areaNorm.X() * one_rl_r,
			   copysign(1.0,velRSum) * areaNorm.Y() * one_rl_r,
			   copysign(1.0,velRSum) * areaNorm.Z() * one_rl_r,
			   0.0};

  //derivative of Roe speed of sound wrt left conservative variables
  double dA_dUl[5] = { ((0.5 * (eqnState.Gamma() - 1.0) / aR) * (0.5 * (velR.MagSq() + left.Velocity().DotProd(velR))) 
            	        + 0.5 * (left.Enthalpy(eqnState)- hR) - (left.SoS(eqnState) * left.SoS(eqnState)) / (eqnState.Gamma() - 1.0)
			+ 0.5 * (eqnState.Gamma() - 2.0) * left.Velocity().MagSq()) * one_rl_r,
		    (-0.5 * (eqnState.Gamma() - 1.0) * (uR + (eqnState.Gamma() - 1.0) * left.U()) / aR) * one_rl_r,
		    (-0.5 * (eqnState.Gamma() - 1.0) * (vR + (eqnState.Gamma() - 1.0) * left.V()) / aR) * one_rl_r,
		    (-0.5 * (eqnState.Gamma() - 1.0) * (wR + (eqnState.Gamma() - 1.0) * left.W()) / aR) * one_rl_r,
		       (0.5 * eqnState.Gamma() * (eqnState.Gamma() - 1.0) / aR) * one_rl_r};

  //derivative of Roe density wrt left conservative variables
  double dR_dUl[5] = { 0.5 * rhoR / left.Rho(),
		       0.0,
		       0.0,
		       0.0,
		       0.0};

  //derivative of Roe u velocity wrt left conservative variables
  double dU_dUl[5] = {-0.5 * (left.U() + uR) * one_rl_r,
		      one_rl_r,
		      0.0,
		      0.0,
		      0.0};

  //derivative of Roe v velocity wrt left conservative variables
  double dV_dUl[5] = {-0.5 * (left.V() + vR) * one_rl_r,
		      0.0,
		      one_rl_r,
		      0.0,
		      0.0};

  //derivative of Roe w velocity wrt left conservative variables
  double dW_dUl[5] = {-0.5 * (left.W() + wR) * one_rl_r,
		      0.0,
		      0.0,
		      one_rl_r,
		      0.0};

  //derivative of Roe enthalpy wrt left conservative variables
  double dH_dUl[5] = { (0.5 * (left.Enthalpy(eqnState) - hR) - left.SoS(eqnState) * left.SoS(eqnState) / (eqnState.Gamma() - 1.0) 
			+ 0.5 * (eqnState.Gamma() - 2.0) * left.Velocity().MagSq()) * one_rl_r,
		       (1.0 - eqnState.Gamma()) * left.U() * one_rl_r,
		       (1.0 - eqnState.Gamma()) * left.V() * one_rl_r,
		       (1.0 - eqnState.Gamma()) * left.W() * one_rl_r,
		       eqnState.Gamma() * one_rl_r };

  //derivatives of differences
  //derivative of density difference wrt left conservative variables
  double dDeltR_dUl[5] = {-1.0,
			  0.0,
			  0.0,
			  0.0,
			  0.0};

  //derivative of pressure difference wrt left conservative variables
  double dDeltP_dUl[5] = { -0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq(),
			   (eqnState.Gamma() - 1.0) * left.U(),
			   (eqnState.Gamma() - 1.0) * left.V(),
			   (eqnState.Gamma() - 1.0) * left.W(),
			   -1.0 * (eqnState.Gamma() - 1.0)};

  //derivative of velocity magnitude difference wrt left conservative variables
  double dDeltVmag_dUl[5] = { velLeftSum / left.Rho(),
			      -1.0 * areaNorm.X() / left.Rho(),
			      -1.0 * areaNorm.Y() / left.Rho(),
			      -1.0 * areaNorm.Z() / left.Rho(),
			      0.0};

  //derivative of u velocity difference wrt left conservative variables
  double dDeltU_dUl[5] = { left.U() / left.Rho(),
			   -1.0 / left.Rho(),
			   0.0,
			   0.0,
			   0.0};

  //derivative of v velocity difference wrt left conservative variables
  double dDeltV_dUl[5] = { left.V() / left.Rho(),
			   0.0,
			   -1.0 / left.Rho(),
			   0.0,
			   0.0};

  //derivative of w velocity difference wrt left conservative variables
  double dDeltW_dUl[5] = { left.W() / left.Rho(),
			   0.0,
			   0.0,
			   -1.0 / left.Rho(),
			   0.0};

  //differentiate the absolute value of the wave speeds and add to flux jacobian variable
  double dWs1_dUl[5];
  if(velRSum - aR > 0.0){
    for(int kk = 0; kk < 5; kk++ ){
      dWs1_dUl[kk] = dQr_dUl[kk] - dA_dUl[kk];
    }
  }
  else{
    for(int kk = 0; kk < 5; kk++ ){
      dWs1_dUl[kk] = -1.0 * (dQr_dUl[kk] - dA_dUl[kk]);
    }
  }
  //entropy fix
  if(waveSpeed[0] < entropyFix){
    for(int kk = 0; kk < 5; kk++ ){
      dWs1_dUl[kk] = waveSpeed[0] / entropyFix * dWs1_dUl[kk];
    }
  }

  //add to flux jacobian for first wave speed contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUl.SetData(kk,0, dF_dUl.Data(kk,0) - 0.5 * dWs1_dUl[0] * waveStrength[0] * lAcousticEigV[kk]);
    dF_dUl.SetData(kk,1, dF_dUl.Data(kk,1) - 0.5 * dWs1_dUl[1] * waveStrength[0] * lAcousticEigV[kk]);
    dF_dUl.SetData(kk,2, dF_dUl.Data(kk,2) - 0.5 * dWs1_dUl[2] * waveStrength[0] * lAcousticEigV[kk]);
    dF_dUl.SetData(kk,3, dF_dUl.Data(kk,3) - 0.5 * dWs1_dUl[3] * waveStrength[0] * lAcousticEigV[kk]);
    dF_dUl.SetData(kk,4, dF_dUl.Data(kk,4) - 0.5 * dWs1_dUl[4] * waveStrength[0] * lAcousticEigV[kk]);
  }

  //add to flux jacobian for second wave speed contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUl.SetData(kk,0, dF_dUl.Data(kk,0) - 0.5 * dAbsQr_dUl[0] * waveStrength[1] * entropyEigV[kk]);
    dF_dUl.SetData(kk,1, dF_dUl.Data(kk,1) - 0.5 * dAbsQr_dUl[1] * waveStrength[1] * entropyEigV[kk]);
    dF_dUl.SetData(kk,2, dF_dUl.Data(kk,2) - 0.5 * dAbsQr_dUl[2] * waveStrength[1] * entropyEigV[kk]);
    dF_dUl.SetData(kk,3, dF_dUl.Data(kk,3) - 0.5 * dAbsQr_dUl[3] * waveStrength[1] * entropyEigV[kk]);
    dF_dUl.SetData(kk,4, dF_dUl.Data(kk,4) - 0.5 * dAbsQr_dUl[4] * waveStrength[1] * entropyEigV[kk]);
  }

  //calculate 3rd wave speed
  double dWs3_dUl[5];
  if(velRSum + aR > 0.0){
    for(int kk = 0; kk < 5; kk++ ){
      dWs3_dUl[kk] = dQr_dUl[kk] + dA_dUl[kk];
    }
  }
  else{
    for(int kk = 0; kk < 5; kk++ ){
      dWs3_dUl[kk] = -1.0 * (dQr_dUl[kk] + dA_dUl[kk]);
    }
  }
  //entropy fix
  if(waveSpeed[2] < entropyFix){
    for(int kk = 0; kk < 5; kk++ ){
      dWs3_dUl[kk] = waveSpeed[2] / entropyFix * dWs3_dUl[kk];
    }
  }

  //add to flux jacobian for third wave speed contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUl.SetData(kk,0, dF_dUl.Data(kk,0) - 0.5 * dWs3_dUl[0] * waveStrength[2] * rAcousticEigV[kk]);
    dF_dUl.SetData(kk,1, dF_dUl.Data(kk,1) - 0.5 * dWs3_dUl[1] * waveStrength[2] * rAcousticEigV[kk]);
    dF_dUl.SetData(kk,2, dF_dUl.Data(kk,2) - 0.5 * dWs3_dUl[2] * waveStrength[2] * rAcousticEigV[kk]);
    dF_dUl.SetData(kk,3, dF_dUl.Data(kk,3) - 0.5 * dWs3_dUl[3] * waveStrength[2] * rAcousticEigV[kk]);
    dF_dUl.SetData(kk,4, dF_dUl.Data(kk,4) - 0.5 * dWs3_dUl[4] * waveStrength[2] * rAcousticEigV[kk]);
  }

  //add to flux jacobian for fourth wave speed contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUl.SetData(kk,0, dF_dUl.Data(kk,0) - 0.5 * dAbsQr_dUl[0] * waveStrength[3] * shearEigV[kk]);
    dF_dUl.SetData(kk,1, dF_dUl.Data(kk,1) - 0.5 * dAbsQr_dUl[1] * waveStrength[3] * shearEigV[kk]);
    dF_dUl.SetData(kk,2, dF_dUl.Data(kk,2) - 0.5 * dAbsQr_dUl[2] * waveStrength[3] * shearEigV[kk]);
    dF_dUl.SetData(kk,3, dF_dUl.Data(kk,3) - 0.5 * dAbsQr_dUl[3] * waveStrength[3] * shearEigV[kk]);
    dF_dUl.SetData(kk,4, dF_dUl.Data(kk,4) - 0.5 * dAbsQr_dUl[4] * waveStrength[3] * shearEigV[kk]);
  }

  //Differentiate the wave strength and add the third term to flux jacobian variable
  double dWst1_dUl[5];
  for(int kk = 0; kk < 5; kk ++){
    dWst1_dUl[kk] = 0.5 * (-2.0 * pDiff + rhoR * aR * normVelDiff) / pow(aR,3) * dA_dUl[kk] - 0.5 * normVelDiff / aR * dR_dUl[kk]
      + 0.5 * dDeltP_dUl[kk] / (aR * aR) - 0.5 * rhoR * dDeltVmag_dUl[kk] / aR;
  }

  //add to flux jacobian for first wave strength contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUl.SetData(kk,0, dF_dUl.Data(kk,0) - 0.5 * waveSpeed[0] * dWst1_dUl[0] * lAcousticEigV[kk]);
    dF_dUl.SetData(kk,1, dF_dUl.Data(kk,1) - 0.5 * waveSpeed[0] * dWst1_dUl[1] * lAcousticEigV[kk]);
    dF_dUl.SetData(kk,2, dF_dUl.Data(kk,2) - 0.5 * waveSpeed[0] * dWst1_dUl[2] * lAcousticEigV[kk]);
    dF_dUl.SetData(kk,3, dF_dUl.Data(kk,3) - 0.5 * waveSpeed[0] * dWst1_dUl[3] * lAcousticEigV[kk]);
    dF_dUl.SetData(kk,4, dF_dUl.Data(kk,4) - 0.5 * waveSpeed[0] * dWst1_dUl[4] * lAcousticEigV[kk]);
  }

  //second wave strength term
  double dWst2_dUl[5];
  for(int kk = 0; kk < 5; kk ++){
    dWst2_dUl[kk] = dDeltR_dUl[kk] - dDeltP_dUl[kk] / (aR * aR) + 2.0 * pDiff / pow(aR,3) * dA_dUl[kk];
  }
  //add to flux jacobian for second wave strength contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUl.SetData(kk,0, dF_dUl.Data(kk,0) - 0.5 * waveSpeed[1] * dWst2_dUl[0] * entropyEigV[kk]);
    dF_dUl.SetData(kk,1, dF_dUl.Data(kk,1) - 0.5 * waveSpeed[1] * dWst2_dUl[1] * entropyEigV[kk]);
    dF_dUl.SetData(kk,2, dF_dUl.Data(kk,2) - 0.5 * waveSpeed[1] * dWst2_dUl[2] * entropyEigV[kk]);
    dF_dUl.SetData(kk,3, dF_dUl.Data(kk,3) - 0.5 * waveSpeed[1] * dWst2_dUl[3] * entropyEigV[kk]);
    dF_dUl.SetData(kk,4, dF_dUl.Data(kk,4) - 0.5 * waveSpeed[1] * dWst2_dUl[4] * entropyEigV[kk]);
  }

  //third wave strength term
  double dWst3_dUl[5];
  for(int kk = 0; kk < 5; kk ++){
    dWst3_dUl[kk] = 0.5 * (-2.0 * pDiff - rhoR * aR * normVelDiff) / pow(aR,3) * dA_dUl[kk] + 0.5 * normVelDiff / aR * dR_dUl[kk]
      + 0.5 * dDeltP_dUl[kk] / (aR * aR) + 0.5 * rhoR * dDeltVmag_dUl[kk] / aR;
  }
  //add to flux jacobian for third wave strength contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUl.SetData(kk,0, dF_dUl.Data(kk,0) - 0.5 * waveSpeed[2] * dWst3_dUl[0] * rAcousticEigV[kk]);
    dF_dUl.SetData(kk,1, dF_dUl.Data(kk,1) - 0.5 * waveSpeed[2] * dWst3_dUl[1] * rAcousticEigV[kk]);
    dF_dUl.SetData(kk,2, dF_dUl.Data(kk,2) - 0.5 * waveSpeed[2] * dWst3_dUl[2] * rAcousticEigV[kk]);
    dF_dUl.SetData(kk,3, dF_dUl.Data(kk,3) - 0.5 * waveSpeed[2] * dWst3_dUl[3] * rAcousticEigV[kk]);
    dF_dUl.SetData(kk,4, dF_dUl.Data(kk,4) - 0.5 * waveSpeed[2] * dWst3_dUl[4] * rAcousticEigV[kk]);
  }

  //add to flux jacobian for fourth wave strength contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUl.SetData(kk,0, dF_dUl.Data(kk,0) - 0.5 * waveSpeed[3] * dR_dUl[0] * shearEigV[kk]);
    dF_dUl.SetData(kk,1, dF_dUl.Data(kk,1) - 0.5 * waveSpeed[3] * dR_dUl[1] * shearEigV[kk]);
    dF_dUl.SetData(kk,2, dF_dUl.Data(kk,2) - 0.5 * waveSpeed[3] * dR_dUl[2] * shearEigV[kk]);
    dF_dUl.SetData(kk,3, dF_dUl.Data(kk,3) - 0.5 * waveSpeed[3] * dR_dUl[3] * shearEigV[kk]);
    dF_dUl.SetData(kk,4, dF_dUl.Data(kk,4) - 0.5 * waveSpeed[3] * dR_dUl[4] * shearEigV[kk]);
  }


  //Differentiate eigenvectors and add the fourth term
  //first eigenvalue
  squareMatrix dLAc_dUl(5);
  for(int kk = 0; kk < 5; kk++){
    dLAc_dUl.SetData(0, kk, 0.0);
    dLAc_dUl.SetData(1, kk, dU_dUl[kk] - dA_dUl[kk] * areaNorm.X());
    dLAc_dUl.SetData(2, kk, dV_dUl[kk] - dA_dUl[kk] * areaNorm.Y());
    dLAc_dUl.SetData(3, kk, dW_dUl[kk] - dA_dUl[kk] * areaNorm.Z());
    dLAc_dUl.SetData(4, kk, dH_dUl[kk] - dA_dUl[kk] * velRSum - dQr_dUl[kk] * aR);
  }

  //add to flux jacobian term contribution from first eigenvalue
  dF_dUl = dF_dUl - 0.5 * waveSpeed[0] * waveStrength[0] * dLAc_dUl;

  //second eigenvalue
  squareMatrix dEnt_dUl(5);
  for(int kk = 0; kk < 5; kk++){
    dEnt_dUl.SetData(0, kk, 0.0);
    dEnt_dUl.SetData(1, kk, dU_dUl[kk] );
    dEnt_dUl.SetData(2, kk, dV_dUl[kk] );
    dEnt_dUl.SetData(3, kk, dW_dUl[kk] );
    dEnt_dUl.SetData(4, kk, uR * dU_dUl[kk] + vR * dV_dUl[kk] + wR * dW_dUl[kk] );
  }

  //add to flux jacobian term contribution from second eigenvalue
  dF_dUl = dF_dUl - 0.5 * waveSpeed[1] * waveStrength[1] * dEnt_dUl;

  //third eigenvalue
  squareMatrix dRAc_dUl(5);
  for(int kk = 0; kk < 5; kk++){
    dRAc_dUl.SetData(0, kk, 0.0);
    dRAc_dUl.SetData(1, kk, dU_dUl[kk] + dA_dUl[kk] * areaNorm.X());
    dRAc_dUl.SetData(2, kk, dV_dUl[kk] + dA_dUl[kk] * areaNorm.Y());
    dRAc_dUl.SetData(3, kk, dW_dUl[kk] + dA_dUl[kk] * areaNorm.Z());
    dRAc_dUl.SetData(4, kk, dH_dUl[kk] + dA_dUl[kk] * velRSum + dQr_dUl[kk] * aR);
  }

  //add to flux jacobian term contribution from third eigenvalue
  dF_dUl = dF_dUl - 0.5 * waveSpeed[2] * waveStrength[2] * dRAc_dUl;

  //fourth eigenvalue
  squareMatrix dSh_dUl(5);
  for(int kk = 0; kk < 5; kk++){
    dSh_dUl.SetData(0, kk, 0.0);
    dSh_dUl.SetData(1, kk, dDeltU_dUl[kk] - dDeltVmag_dUl[kk] * areaNorm.X());
    dSh_dUl.SetData(2, kk, dDeltV_dUl[kk] - dDeltVmag_dUl[kk] * areaNorm.Y());
    dSh_dUl.SetData(3, kk, dDeltW_dUl[kk] - dDeltVmag_dUl[kk] * areaNorm.Z());
    dSh_dUl.SetData(4, kk, uDiff * dU_dUl[kk] + vDiff * dV_dUl[kk] + wDiff * dW_dUl[kk] - normVelDiff * dQr_dUl[kk]
		    + uR * dDeltU_dUl[kk] + vR * dDeltV_dUl[kk] + wR * dDeltW_dUl[kk] - velRSum * dDeltVmag_dUl[kk]);
  }

  //add to flux jacobian term contribution from fourth eigenvalue
  dF_dUl = dF_dUl - 0.5 * waveSpeed[3] * waveStrength[3] * dSh_dUl;

  //Compute derivative of flux wrt right conservative variables //////////////////////////////////////////////////////////////
  dF_dUr.Zero();

  //calculate flux derivatives
  //column zero
  dF_dUr.SetData(0, 0, 0.0);
  dF_dUr.SetData(1, 0, 0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() * areaNorm.X() - right.U() * velRightSum);
  dF_dUr.SetData(2, 0, 0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() * areaNorm.Y() - right.V() * velRightSum);
  dF_dUr.SetData(3, 0, 0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() * areaNorm.Z() - right.W() * velRightSum);
  dF_dUr.SetData(4, 0, (0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() - right.Enthalpy(eqnState)) * velRightSum); 
		       
  //column one
  dF_dUr.SetData(1, 0, areaNorm.X());
  dF_dUr.SetData(1, 1, right.U() * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * areaNorm.X() + velRightSum);
  dF_dUr.SetData(1, 2, right.V() * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * areaNorm.Y());
  dF_dUr.SetData(1, 3, right.W() * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * areaNorm.Z());
  dF_dUr.SetData(1, 4, right.Enthalpy(eqnState) * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * velRightSum);

  //column two
  dF_dUr.SetData(2, 0, areaNorm.Y());
  dF_dUr.SetData(2, 1, right.U() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * areaNorm.X());
  dF_dUr.SetData(2, 2, right.V() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * areaNorm.Y() + velRightSum);
  dF_dUr.SetData(2, 3, right.W() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * areaNorm.Z());
  dF_dUr.SetData(2, 4, right.Enthalpy(eqnState) * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * velRightSum);

  //column three
  dF_dUr.SetData(3, 0, areaNorm.Z());
  dF_dUr.SetData(3, 1, right.U() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * areaNorm.X());
  dF_dUr.SetData(3, 2, right.V() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * areaNorm.Y());
  dF_dUr.SetData(3, 3, right.W() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * areaNorm.Z() + velRightSum);
  dF_dUr.SetData(3, 4, right.Enthalpy(eqnState) * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * velRightSum);

  //column four
  dF_dUr.SetData(4, 0, 0.0);
  dF_dUr.SetData(4, 1, (eqnState.Gamma() - 1.0) * areaNorm.X());
  dF_dUr.SetData(4, 2, (eqnState.Gamma() - 1.0) * areaNorm.Y());
  dF_dUr.SetData(4, 3, (eqnState.Gamma() - 1.0) * areaNorm.Z());
  dF_dUr.SetData(4, 4, eqnState.Gamma() * velRightSum);

  //multiply by factor 1/2
  dF_dUr = dF_dUr * 0.5;

  double one_rr_r = 1.0 / (right.Rho() + rhoR);

  //calculate derivatives of Roe quantities
  //derivative of normal Roe velocity wrt right conservative variables
  double dQr_dUr[5] = { -0.5 * (velRightSum + velRSum) * one_rr_r,
			areaNorm.X() * one_rr_r,
			areaNorm.Y() * one_rr_r,
			areaNorm.Z() * one_rr_r,
			0.0};

  //derivative of absolute value of Roe velocity wrt right conservative variables
  double dAbsQr_dUr[5] = { -0.5 * copysign(1.0,velRSum) * (velRightSum + velRSum) * one_rr_r,
			   copysign(1.0,velRSum) * areaNorm.X() * one_rr_r,
			   copysign(1.0,velRSum) * areaNorm.Y() * one_rr_r,
			   copysign(1.0,velRSum) * areaNorm.Z() * one_rr_r,
			   0.0};

  //derivative of Roe speed of sound wrt right conservative variables
  double dA_dUr[5] = { ((0.5 * (eqnState.Gamma() - 1.0) / aR) * (0.5 * (velR.MagSq() + right.Velocity().DotProd(velR))) 
		    + 0.5 * (right.Enthalpy(eqnState)- hR) - (right.SoS(eqnState) * right.SoS(eqnState)) / (eqnState.Gamma() - 1.0)
		    + 0.5 * (eqnState.Gamma() - 2.0) * right.Velocity().MagSq()) * one_rr_r,
		    (-0.5 * (eqnState.Gamma() - 1.0) * (uR + (eqnState.Gamma() - 1.0) * right.U()) / aR) * one_rr_r,
		    (-0.5 * (eqnState.Gamma() - 1.0) * (vR + (eqnState.Gamma() - 1.0) * right.V()) / aR) * one_rr_r,
		    (-0.5 * (eqnState.Gamma() - 1.0) * (wR + (eqnState.Gamma() - 1.0) * right.W()) / aR) * one_rr_r,
		       (0.5 * eqnState.Gamma() * (eqnState.Gamma() - 1.0) / aR) * one_rr_r};

  //derivative of Roe density wrt right conservative variables
  double dR_dUr[5] = { 0.5 * rhoR / right.Rho(),
		       0.0,
		       0.0,
		       0.0,
		       0.0};

  //derivative of Roe u velocity wrt right conservative variables
  double dU_dUr[5] = {-0.5 * (right.U() + uR) * one_rr_r,
		      one_rr_r,
		      0.0,
		      0.0,
		      0.0};

  //derivative of Roe v velocity wrt right conservative variables
  double dV_dUr[5] = {-0.5 * (right.V() + vR) * one_rr_r,
		      0.0,
		      one_rr_r,
		      0.0,
		      0.0};

  //derivative of Roe w velocity wrt right conservative variables
  double dW_dUr[5] = {-0.5 * (right.W() + wR) * one_rr_r,
		      0.0,
		      0.0,
		      one_rr_r,
		      0.0};

  //derivative of Roe enthalpy wrt right conservative variables
  double dH_dUr[5] = { (0.5 * (right.Enthalpy(eqnState) - hR) - right.SoS(eqnState) * right.SoS(eqnState) / (eqnState.Gamma() - 1.0) 
			+ 0.5 * (eqnState.Gamma() - 2.0) * right.Velocity().MagSq()) * one_rr_r,
		       (1.0 - eqnState.Gamma()) * right.U() * one_rr_r,
		       (1.0 - eqnState.Gamma()) * right.V() * one_rr_r,
		       (1.0 - eqnState.Gamma()) * right.W() * one_rr_r,
		       eqnState.Gamma() * one_rr_r};

  //derivatives of differences
  //derivative of density difference wrt right conservative variables
  double dDeltR_dUr[5] = {1.0,
			  0.0,
			  0.0,
			  0.0,
			  0.0};

  //derivative of pressure difference wrt right conservative variables
  double dDeltP_dUr[5] = { 0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq(),
			   -1.0 * (eqnState.Gamma() - 1.0) * right.U(),
			   -1.0 * (eqnState.Gamma() - 1.0) * right.V(),
			   -1.0 * (eqnState.Gamma() - 1.0) * right.W(),
			   (eqnState.Gamma() - 1.0)};

  //derivative of velocity magnitude difference wrt right conservative variables
  double dDeltVmag_dUr[5] = { -1.0 * velRightSum / right.Rho(),
			      areaNorm.X() / right.Rho(),
			      areaNorm.Y() / right.Rho(),
			      areaNorm.Z() / right.Rho(),
			      0.0};

  //derivative of u velocity difference wrt right conservative variables
  double dDeltU_dUr[5] = { -1.0 * right.U() / right.Rho(),
			   1.0 / right.Rho(),
			   0.0,
			   0.0,
			   0.0};

  //derivative of v velocity difference wrt right conservative variables
  double dDeltV_dUr[5] = { -1.0 * right.V() / right.Rho(),
			   0.0,
			   1.0 / right.Rho(),
			   0.0,
			   0.0};

  //derivative of w velocity difference wrt right conservative variables
  double dDeltW_dUr[5] = { -1.0 * right.W() / right.Rho(),
			   0.0,
			   0.0,
			   1.0 / right.Rho(),
			   0.0};

  //differentiate the absolute value of the wave speeds and add to flux jacobian variable
  double dWs1_dUr[5];
  if(velRSum - aR > 0.0){
    for(int kk = 0; kk < 5; kk++ ){
      dWs1_dUr[kk] = dQr_dUr[kk] - dA_dUr[kk];
    }
  }
  else{
    for(int kk = 0; kk < 5; kk++ ){
      dWs1_dUr[kk] = -1.0 * (dQr_dUr[kk] - dA_dUr[kk]);
    }
  }
  //entropy fix
  if(waveSpeed[0] < entropyFix){
    for(int kk = 0; kk < 5; kk++ ){
      dWs1_dUr[kk] = waveSpeed[0] / entropyFix * dWs1_dUr[kk];
    }
  }
  //add to flux jacobian for first wave speed contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUr.SetData(kk,0, dF_dUr.Data(kk,0) - 0.5 * dWs1_dUr[0] * waveStrength[0] * lAcousticEigV[kk]);
    dF_dUr.SetData(kk,1, dF_dUr.Data(kk,1) - 0.5 * dWs1_dUr[1] * waveStrength[0] * lAcousticEigV[kk]);
    dF_dUr.SetData(kk,2, dF_dUr.Data(kk,2) - 0.5 * dWs1_dUr[2] * waveStrength[0] * lAcousticEigV[kk]);
    dF_dUr.SetData(kk,3, dF_dUr.Data(kk,3) - 0.5 * dWs1_dUr[3] * waveStrength[0] * lAcousticEigV[kk]);
    dF_dUr.SetData(kk,4, dF_dUr.Data(kk,4) - 0.5 * dWs1_dUr[4] * waveStrength[0] * lAcousticEigV[kk]);
  }

  //add to flux jacobian for second wave speed contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUr.SetData(kk,0, dF_dUr.Data(kk,0) - 0.5 * dAbsQr_dUr[0] * waveStrength[1] * entropyEigV[kk]);
    dF_dUr.SetData(kk,1, dF_dUr.Data(kk,1) - 0.5 * dAbsQr_dUr[1] * waveStrength[1] * entropyEigV[kk]);
    dF_dUr.SetData(kk,2, dF_dUr.Data(kk,2) - 0.5 * dAbsQr_dUr[2] * waveStrength[1] * entropyEigV[kk]);
    dF_dUr.SetData(kk,3, dF_dUr.Data(kk,3) - 0.5 * dAbsQr_dUr[3] * waveStrength[1] * entropyEigV[kk]);
    dF_dUr.SetData(kk,4, dF_dUr.Data(kk,4) - 0.5 * dAbsQr_dUr[4] * waveStrength[1] * entropyEigV[kk]);
  }

  //calculate 3rd wave speed
  double dWs3_dUr[5];
  if(velRSum + aR > 0.0){
    for(int kk = 0; kk < 5; kk++ ){
      dWs3_dUr[kk] = dQr_dUr[kk] + dA_dUr[kk];
    }
  }
  else{
    for(int kk = 0; kk < 5; kk++ ){
      dWs3_dUr[kk] = -1.0 * (dQr_dUr[kk] + dA_dUr[kk]);
    }
  }
  //entropy fix
  if(waveSpeed[2] < entropyFix){
    for(int kk = 0; kk < 5; kk++ ){
      dWs3_dUr[kk] = waveSpeed[2] / entropyFix * dWs3_dUr[kk];
    }
  }

  //add to flux jacobian for third wave speed contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUr.SetData(kk,0, dF_dUr.Data(kk,0) - 0.5 * dWs3_dUr[0] * waveStrength[2] * rAcousticEigV[kk]);
    dF_dUr.SetData(kk,1, dF_dUr.Data(kk,1) - 0.5 * dWs3_dUr[1] * waveStrength[2] * rAcousticEigV[kk]);
    dF_dUr.SetData(kk,2, dF_dUr.Data(kk,2) - 0.5 * dWs3_dUr[2] * waveStrength[2] * rAcousticEigV[kk]);
    dF_dUr.SetData(kk,3, dF_dUr.Data(kk,3) - 0.5 * dWs3_dUr[3] * waveStrength[2] * rAcousticEigV[kk]);
    dF_dUr.SetData(kk,4, dF_dUr.Data(kk,4) - 0.5 * dWs3_dUr[4] * waveStrength[2] * rAcousticEigV[kk]);
  }

  //add to flux jacobian for fourth wave speed contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUr.SetData(kk,0, dF_dUr.Data(kk,0) - 0.5 * dAbsQr_dUr[0] * waveStrength[3] * shearEigV[kk]);
    dF_dUr.SetData(kk,1, dF_dUr.Data(kk,1) - 0.5 * dAbsQr_dUr[1] * waveStrength[3] * shearEigV[kk]);
    dF_dUr.SetData(kk,2, dF_dUr.Data(kk,2) - 0.5 * dAbsQr_dUr[2] * waveStrength[3] * shearEigV[kk]);
    dF_dUr.SetData(kk,3, dF_dUr.Data(kk,3) - 0.5 * dAbsQr_dUr[3] * waveStrength[3] * shearEigV[kk]);
    dF_dUr.SetData(kk,4, dF_dUr.Data(kk,4) - 0.5 * dAbsQr_dUr[4] * waveStrength[3] * shearEigV[kk]);
  }

  //Differentiate the wave strength and add the first term to flux jacobian variable
  double dWst1_dUr[5];
  for(int kk = 0; kk < 5; kk ++){
    dWst1_dUr[kk] = 0.5 * (-2.0 * pDiff + rhoR * aR * normVelDiff) / pow(aR,3) * dA_dUr[kk] - 0.5 * normVelDiff / aR * dR_dUr[kk]
      + 0.5 * dDeltP_dUr[kk] / (aR * aR) - 0.5 * rhoR * dDeltVmag_dUr[kk] / aR;
  }

  //add to flux jacobian for first wave strength contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUr.SetData(kk,0, dF_dUr.Data(kk,0) - 0.5 * waveSpeed[0] * dWst1_dUr[0] * lAcousticEigV[kk]);
    dF_dUr.SetData(kk,1, dF_dUr.Data(kk,1) - 0.5 * waveSpeed[0] * dWst1_dUr[1] * lAcousticEigV[kk]);
    dF_dUr.SetData(kk,2, dF_dUr.Data(kk,2) - 0.5 * waveSpeed[0] * dWst1_dUr[2] * lAcousticEigV[kk]);
    dF_dUr.SetData(kk,3, dF_dUr.Data(kk,3) - 0.5 * waveSpeed[0] * dWst1_dUr[3] * lAcousticEigV[kk]);
    dF_dUr.SetData(kk,4, dF_dUr.Data(kk,4) - 0.5 * waveSpeed[0] * dWst1_dUr[4] * lAcousticEigV[kk]);
  }

  //second wave strength term
  double dWst2_dUr[5];
  for(int kk = 0; kk < 5; kk ++){
    dWst2_dUr[kk] = dDeltR_dUr[kk] - dDeltP_dUr[kk] / (aR * aR) + 2.0 * pDiff / pow(aR,3) * dA_dUr[kk];
  }
  //add to flux jacobian for second wave strength contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUr.SetData(kk,0, dF_dUr.Data(kk,0) - 0.5 * waveSpeed[1] * dWst2_dUr[0] * entropyEigV[kk]);
    dF_dUr.SetData(kk,1, dF_dUr.Data(kk,1) - 0.5 * waveSpeed[1] * dWst2_dUr[1] * entropyEigV[kk]);
    dF_dUr.SetData(kk,2, dF_dUr.Data(kk,2) - 0.5 * waveSpeed[1] * dWst2_dUr[2] * entropyEigV[kk]);
    dF_dUr.SetData(kk,3, dF_dUr.Data(kk,3) - 0.5 * waveSpeed[1] * dWst2_dUr[3] * entropyEigV[kk]);
    dF_dUr.SetData(kk,4, dF_dUr.Data(kk,4) - 0.5 * waveSpeed[1] * dWst2_dUr[4] * entropyEigV[kk]);
  }

  //third wave strength term
  double dWst3_dUr[5];
  for(int kk = 0; kk < 5; kk ++){
    dWst3_dUr[kk] = 0.5 * (-2.0 * pDiff - rhoR * aR * normVelDiff) / pow(aR,3) * dA_dUr[kk] + 0.5 * normVelDiff / aR * dR_dUr[kk]
      + 0.5 * dDeltP_dUr[kk] / (aR * aR) + 0.5 * rhoR * dDeltVmag_dUr[kk] / aR;
  }
  //add to flux jacobian for third wave strength contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUr.SetData(kk,0, dF_dUr.Data(kk,0) - 0.5 * waveSpeed[2] * dWst3_dUr[0] * rAcousticEigV[kk]);
    dF_dUr.SetData(kk,1, dF_dUr.Data(kk,1) - 0.5 * waveSpeed[2] * dWst3_dUr[1] * rAcousticEigV[kk]);
    dF_dUr.SetData(kk,2, dF_dUr.Data(kk,2) - 0.5 * waveSpeed[2] * dWst3_dUr[2] * rAcousticEigV[kk]);
    dF_dUr.SetData(kk,3, dF_dUr.Data(kk,3) - 0.5 * waveSpeed[2] * dWst3_dUr[3] * rAcousticEigV[kk]);
    dF_dUr.SetData(kk,4, dF_dUr.Data(kk,4) - 0.5 * waveSpeed[2] * dWst3_dUr[4] * rAcousticEigV[kk]);
  }

  //add to flux jacobian for fourth wave strength contribution
  for(int kk = 0; kk < 5; kk++){
    dF_dUr.SetData(kk,0, dF_dUr.Data(kk,0) - 0.5 * waveSpeed[3] * dR_dUr[0] * shearEigV[kk]);
    dF_dUr.SetData(kk,1, dF_dUr.Data(kk,1) - 0.5 * waveSpeed[3] * dR_dUr[1] * shearEigV[kk]);
    dF_dUr.SetData(kk,2, dF_dUr.Data(kk,2) - 0.5 * waveSpeed[3] * dR_dUr[2] * shearEigV[kk]);
    dF_dUr.SetData(kk,3, dF_dUr.Data(kk,3) - 0.5 * waveSpeed[3] * dR_dUr[3] * shearEigV[kk]);
    dF_dUr.SetData(kk,4, dF_dUr.Data(kk,4) - 0.5 * waveSpeed[3] * dR_dUr[4] * shearEigV[kk]);
  }

  //Differentiate eigenvectors and add the fourth term
  //first eigenvector
  squareMatrix dLAc_dUr(5);
  for(int kk = 0; kk < 5; kk++){
    dLAc_dUr.SetData(0, kk, 0.0);
    dLAc_dUr.SetData(1, kk, dU_dUr[kk] - dA_dUr[kk] * areaNorm.X());
    dLAc_dUr.SetData(2, kk, dV_dUr[kk] - dA_dUr[kk] * areaNorm.Y());
    dLAc_dUr.SetData(3, kk, dW_dUr[kk] - dA_dUr[kk] * areaNorm.Z());
    dLAc_dUr.SetData(4, kk, dH_dUr[kk] - dA_dUr[kk] * velRSum - dQr_dUr[kk] * aR);
  }

  //add to flux jacobian term contribution from first eigenvalue
  dF_dUr = dF_dUr - 0.5 * waveSpeed[0] * waveStrength[0] * dLAc_dUr;

  //second eigenvector
  squareMatrix dEnt_dUr(5);
  for(int kk = 0; kk < 5; kk++){
    dEnt_dUr.SetData(0, kk, 0.0);
    dEnt_dUr.SetData(1, kk, dU_dUr[kk] );
    dEnt_dUr.SetData(2, kk, dV_dUr[kk] );
    dEnt_dUr.SetData(3, kk, dW_dUr[kk] );
    dEnt_dUr.SetData(4, kk, uR * dU_dUr[kk] + vR * dV_dUr[kk] + wR * dW_dUr[kk] );
  }

  //add to flux jacobian term contribution from second eigenvalue
  dF_dUr = dF_dUr - 0.5 * waveSpeed[1] * waveStrength[1] * dEnt_dUr;

  //third eigenvector
  squareMatrix dRAc_dUr(5);
  for(int kk = 0; kk < 5; kk++){
    dRAc_dUr.SetData(0, kk, 0.0);
    dRAc_dUr.SetData(1, kk, dU_dUr[kk] + dA_dUr[kk] * areaNorm.X());
    dRAc_dUr.SetData(2, kk, dV_dUr[kk] + dA_dUr[kk] * areaNorm.Y());
    dRAc_dUr.SetData(3, kk, dW_dUr[kk] + dA_dUr[kk] * areaNorm.Z());
    dRAc_dUr.SetData(4, kk, dH_dUr[kk] + dA_dUr[kk] * velRSum + dQr_dUr[kk] * aR);
  }

  //add to flux jacobian term contribution from third eigenvalue
  dF_dUr = dF_dUr - 0.5 * waveSpeed[2] * waveStrength[2] * dRAc_dUr;

  //fourth eigenvector
  squareMatrix dSh_dUr(5);
  for(int kk = 0; kk < 5; kk++){
    dSh_dUr.SetData(0, kk, 0.0);
    dSh_dUr.SetData(1, kk, dDeltU_dUr[kk] - dDeltVmag_dUr[kk] * areaNorm.X());
    dSh_dUr.SetData(2, kk, dDeltV_dUr[kk] - dDeltVmag_dUr[kk] * areaNorm.Y());
    dSh_dUr.SetData(3, kk, dDeltW_dUr[kk] - dDeltVmag_dUr[kk] * areaNorm.Z());
    dSh_dUr.SetData(4, kk, uDiff * dU_dUr[kk] + vDiff * dV_dUr[kk] + wDiff * dW_dUr[kk] - normVelDiff * dQr_dUr[kk]
		    + uR * dDeltU_dUr[kk] + vR * dDeltV_dUr[kk] + wR * dDeltW_dUr[kk] - velRSum * dDeltVmag_dUr[kk]);
  }

  //add to flux jacobian term contribution from fourth eigenvalue
  dF_dUr = dF_dUr - 0.5 * waveSpeed[3] * waveStrength[3] * dSh_dUr;


}

//function to calculate exact Roe flux jacobians
void ApproxRoeFluxJacobian( const primVars &left, const primVars &right, const idealGas &eqnState, const vector3d<double>& areaVec, double &maxWS, squareMatrix &dF_dUl, squareMatrix &dF_dUr){

  //left --> primative variables from left side
  //right --> primative variables from right side
  //eqnStat --> ideal gas equation of state
  //areaVec --> face area vector
  //maxWS --> maximum wave speed
  //dF_dUl --> dF/dUl, derivative of the Roe flux wrt the left state (conservative variables)
  //dF_dUr --> dF/dUlr, derivative of the Roe flux wrt the right state (conservative variables)


  //check to see that output matricies are correct size
  if( (dF_dUl.Size() != 5) || (dF_dUr.Size() != 5)){
    cerr << "ERROR: Input matricies to RoeFLuxJacobian function are not the correct size!" << endl;
  }

  //compute Rho averaged quantities
  double denRatio = sqrt(right.Rho()/left.Rho());
  //double rhoR = left.Rho() * denRatio;  //Roe averaged density
  double uR = (left.U() + denRatio * right.U()) / (1.0 + denRatio);  //Roe averaged u-velocity
  double vR = (left.V() + denRatio * right.V()) / (1.0 + denRatio);  //Roe averaged v-velocity
  double wR = (left.W() + denRatio * right.W()) / (1.0 + denRatio);  //Roe averaged w-velocity
  double hR = (left.Enthalpy(eqnState) + denRatio * right.Enthalpy(eqnState)) / (1.0 + denRatio);  //Roe averaged total enthalpy
  double aR = sqrt( (eqnState.Gamma() - 1.0) * (hR - 0.5 * (uR*uR + vR*vR + wR*wR)) );  //Roe averaged speed of sound
  //Roe averaged face normal velocity
  vector3d<double> velR(uR,vR,wR);

  vector3d<double> areaNorm = areaVec / areaVec.Mag();  //normalize area vector to unit vector

  //dot product of velocities (Roe, left, right) with unit area vector
  double velRNorm = velR.DotProd(areaNorm);
  double velLeftNorm = left.Velocity().DotProd(areaNorm);
  double velRightNorm = right.Velocity().DotProd(areaNorm);

  //calculate diagonal eigenvalue matrix |lambda|
  squareMatrix lambda(5);
  lambda.Zero();
  lambda.SetData(0, 0, fabs(velRNorm - aR) );
  lambda.SetData(1, 1, fabs(velRNorm) );
  lambda.SetData(2, 2, fabs(velRNorm + aR) );
  lambda.SetData(3, 3, fabs(velRNorm) );
  lambda.SetData(4, 4, fabs(velRNorm) );

  //calculate Roe jacobian matrix A
  //contribution due to normal velocity eigenvalues
  squareMatrix A(5);

  //column zero
  A.SetData(0, 0, 1.0 - 0.5 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) );
  A.SetData(1, 0, -(1.0 + 0.5 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR)) * uR + velRNorm * areaNorm.X() );
  A.SetData(2, 0, -(1.0 + 0.5 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR)) * vR + velRNorm * areaNorm.Y() );
  A.SetData(3, 0, -(1.0 + 0.5 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR)) * wR + velRNorm * areaNorm.Z() );
  A.SetData(4, 0, velRNorm * velRNorm - 0.5 * velR.MagSq() * (1.0 + 0.5 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR)) ); 

  //column one
  A.SetData(0, 1, (eqnState.Gamma() - 1.0) / (aR * aR) * uR );
  A.SetData(1, 1, (eqnState.Gamma() - 1.0) / (aR * aR) * uR * uR + 1.0 - areaNorm.X() * areaNorm.X() );
  A.SetData(2, 1, (eqnState.Gamma() - 1.0) / (aR * aR) * vR * uR       - areaNorm.Y() * areaNorm.X() );
  A.SetData(3, 1, (eqnState.Gamma() - 1.0) / (aR * aR) * wR * uR       - areaNorm.Z() * areaNorm.X() );
  A.SetData(4, 1, (1.0 + 0.5 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR)) * uR - velRNorm * areaNorm.X() );

  //column two
  A.SetData(0, 2, (eqnState.Gamma() - 1.0) / (aR * aR) * vR );
  A.SetData(1, 2, (eqnState.Gamma() - 1.0) / (aR * aR) * uR * vR       - areaNorm.X() * areaNorm.Y() );
  A.SetData(2, 2, (eqnState.Gamma() - 1.0) / (aR * aR) * vR * vR + 1.0 - areaNorm.Y() * areaNorm.Y() );
  A.SetData(3, 2, (eqnState.Gamma() - 1.0) / (aR * aR) * wR * vR       - areaNorm.Z() * areaNorm.Y() );
  A.SetData(4, 2, (1.0 + 0.5 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR)) * vR - velRNorm * areaNorm.Y() );

  //column three
  A.SetData(0, 3, (eqnState.Gamma() - 1.0) / (aR * aR) * wR );
  A.SetData(1, 3, (eqnState.Gamma() - 1.0) / (aR * aR) * uR * wR       - areaNorm.X() * areaNorm.Z() );
  A.SetData(2, 3, (eqnState.Gamma() - 1.0) / (aR * aR) * vR * wR       - areaNorm.Y() * areaNorm.Z() );
  A.SetData(3, 3, (eqnState.Gamma() - 1.0) / (aR * aR) * wR * wR + 1.0 - areaNorm.Z() * areaNorm.Z() );
  A.SetData(4, 3, (1.0 + 0.5 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR)) * wR - velRNorm * areaNorm.Z() );

  //column four
  A.SetData(0, 4, -(eqnState.Gamma() - 1.0) / (aR * aR) );
  A.SetData(1, 4, -(eqnState.Gamma() - 1.0) / (aR * aR) * uR );
  A.SetData(2, 4, -(eqnState.Gamma() - 1.0) / (aR * aR) * vR );
  A.SetData(3, 4, -(eqnState.Gamma() - 1.0) / (aR * aR) * wR );
  A.SetData(4, 4, -(eqnState.Gamma() - 1.0) / (aR * aR) * velR.MagSq() / (aR * aR) );

  A = fabs(velRNorm) * A;

  //contribution due to u - c wave
  squareMatrix temp(5);

  //column zero
  temp.SetData(0, 0, 0.25 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) + 0.5 * velRNorm / aR );
  temp.SetData(1, 0, (uR - aR * areaNorm.X()) * (0.25 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) + 0.5 * velRNorm / aR) );
  temp.SetData(2, 0, (vR - aR * areaNorm.Y()) * (0.25 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) + 0.5 * velRNorm / aR) );
  temp.SetData(3, 0, (wR - aR * areaNorm.Z()) * (0.25 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) + 0.5 * velRNorm / aR) );
  temp.SetData(4, 0, (hR - velRNorm * aR * areaNorm.X()) * (0.25 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) + 0.5 * velRNorm / aR) );

  //column one
  temp.SetData(0, 1, -(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * uR - areaNorm.X() / (2.0 * aR ) );
  temp.SetData(1, 1,  (uR - aR * areaNorm.X()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * uR - areaNorm.X() / (2.0 * aR)) );
  temp.SetData(2, 1,  (vR - aR * areaNorm.Y()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * uR - areaNorm.X() / (2.0 * aR)) );
  temp.SetData(3, 1,  (wR - aR * areaNorm.Z()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * uR - areaNorm.X() / (2.0 * aR)) );
  temp.SetData(4, 1,  (hR - aR * velRNorm)     * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * uR - areaNorm.X() / (2.0 * aR)) );

  //column two
  temp.SetData(0, 2, -(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * vR - areaNorm.Y() / (2.0 * aR ) );
  temp.SetData(1, 2,  (uR - aR * areaNorm.X()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * vR - areaNorm.Y() / (2.0 * aR)) );
  temp.SetData(2, 2,  (vR - aR * areaNorm.Y()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * vR - areaNorm.Y() / (2.0 * aR)) );
  temp.SetData(3, 2,  (wR - aR * areaNorm.Z()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * vR - areaNorm.Y() / (2.0 * aR)) );
  temp.SetData(4, 2,  (hR - aR * velRNorm)     * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * vR - areaNorm.Y() / (2.0 * aR)) );

  //column three
  temp.SetData(0, 3, -(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * wR - areaNorm.Z() / (2.0 * aR ) );
  temp.SetData(1, 3,  (uR - aR * areaNorm.X()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * wR - areaNorm.Z() / (2.0 * aR)) );
  temp.SetData(2, 3,  (vR - aR * areaNorm.Y()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * wR - areaNorm.Z() / (2.0 * aR)) );
  temp.SetData(3, 3,  (wR - aR * areaNorm.Z()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * wR - areaNorm.Z() / (2.0 * aR)) );
  temp.SetData(4, 3,  (hR - aR * velRNorm)     * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * wR - areaNorm.Z() / (2.0 * aR)) );

  //column four
  temp.SetData(0, 4, (eqnState.Gamma() - 1.0) / (2.0 * aR * aR) );
  temp.SetData(1, 4, (uR - aR * areaNorm.X()) * (eqnState.Gamma() - 1.0) / (2.0 * aR * aR) );
  temp.SetData(2, 4, (vR - aR * areaNorm.Y()) * (eqnState.Gamma() - 1.0) / (2.0 * aR * aR) );
  temp.SetData(3, 4, (wR - aR * areaNorm.Z()) * (eqnState.Gamma() - 1.0) / (2.0 * aR * aR) );
  temp.SetData(4, 4, (hR - aR * velRNorm)     * (eqnState.Gamma() - 1.0) / (2.0 * aR * aR) );

  A = A + (fabs(velRNorm - aR) * temp);

  //contribution due to u + c wave

  //column zero
  temp.SetData(0, 0, 0.25 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) - 0.5 * velRNorm / aR );
  temp.SetData(1, 0, (uR + aR * areaNorm.X()) * (0.25 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) - 0.5 * velRNorm / aR) );
  temp.SetData(2, 0, (vR + aR * areaNorm.Y()) * (0.25 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) - 0.5 * velRNorm / aR) );
  temp.SetData(3, 0, (wR + aR * areaNorm.Z()) * (0.25 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) - 0.5 * velRNorm / aR) );
  temp.SetData(4, 0, (hR + velRNorm * aR * areaNorm.X()) * (0.25 * (eqnState.Gamma() - 1.0) * velR.MagSq() / (aR * aR) - 0.5 * velRNorm / aR) );

  //column one
  temp.SetData(0, 1, -(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * uR + areaNorm.X() / (2.0 * aR ) );
  temp.SetData(1, 1,  (uR + aR * areaNorm.X()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * uR + areaNorm.X() / (2.0 * aR)) );
  temp.SetData(2, 1,  (vR + aR * areaNorm.Y()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * uR + areaNorm.X() / (2.0 * aR)) );
  temp.SetData(3, 1,  (wR + aR * areaNorm.Z()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * uR + areaNorm.X() / (2.0 * aR)) );
  temp.SetData(4, 1,  (hR + aR * velRNorm)     * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * uR + areaNorm.X() / (2.0 * aR)) );

  //column two
  temp.SetData(0, 2, -(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * vR + areaNorm.Y() / (2.0 * aR ) );
  temp.SetData(1, 2,  (uR + aR * areaNorm.X()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * vR + areaNorm.Y() / (2.0 * aR)) );
  temp.SetData(2, 2,  (vR + aR * areaNorm.Y()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * vR + areaNorm.Y() / (2.0 * aR)) );
  temp.SetData(3, 2,  (wR + aR * areaNorm.Z()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * vR + areaNorm.Y() / (2.0 * aR)) );
  temp.SetData(4, 2,  (hR + aR * velRNorm)     * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * vR + areaNorm.Y() / (2.0 * aR)) );

  //column three
  temp.SetData(0, 3, -(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * wR + areaNorm.Z() / (2.0 * aR ) );
  temp.SetData(1, 3,  (uR + aR * areaNorm.X()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * wR + areaNorm.Z() / (2.0 * aR)) );
  temp.SetData(2, 3,  (vR + aR * areaNorm.Y()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * wR + areaNorm.Z() / (2.0 * aR)) );
  temp.SetData(3, 3,  (wR + aR * areaNorm.Z()) * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * wR + areaNorm.Z() / (2.0 * aR)) );
  temp.SetData(4, 3,  (hR + aR * velRNorm)     * (-(eqnState.Gamma() - 1.0) / (2.0 * aR * aR) * wR + areaNorm.Z() / (2.0 * aR)) );

  //column four
  temp.SetData(0, 4, (eqnState.Gamma() - 1.0) / (2.0 * aR * aR) );
  temp.SetData(1, 4, (uR + aR * areaNorm.X()) * (eqnState.Gamma() - 1.0) / (2.0 * aR * aR) );
  temp.SetData(2, 4, (vR + aR * areaNorm.Y()) * (eqnState.Gamma() - 1.0) / (2.0 * aR * aR) );
  temp.SetData(3, 4, (wR + aR * areaNorm.Z()) * (eqnState.Gamma() - 1.0) / (2.0 * aR * aR) );
  temp.SetData(4, 4, (hR + aR * velRNorm)     * (eqnState.Gamma() - 1.0) / (2.0 * aR * aR) );

  A = A + (fabs(velRNorm + aR) * temp);

  //begin jacobian calculation ////////////////////////////////////////////////////////////////////////////////////////

  //derivative of Roe flux wrt left conservative variables
  dF_dUl.Zero();

  //column zero
  dF_dUl.SetData(0, 0, 0.0);
  dF_dUl.SetData(1, 0, 0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() * areaNorm.X() - left.U() * velLeftNorm);
  dF_dUl.SetData(2, 0, 0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() * areaNorm.Y() - left.V() * velLeftNorm);
  dF_dUl.SetData(3, 0, 0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() * areaNorm.Z() - left.W() * velLeftNorm);
  dF_dUl.SetData(4, 0, (0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() - left.Enthalpy(eqnState)) * velLeftNorm); 
		       
  //column one
  dF_dUl.SetData(1, 0, areaNorm.X());
  dF_dUl.SetData(1, 1, left.U() * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * areaNorm.X() + velLeftNorm);
  dF_dUl.SetData(1, 2, left.V() * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * areaNorm.Y());
  dF_dUl.SetData(1, 3, left.W() * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * areaNorm.Z());
  dF_dUl.SetData(1, 4, left.Enthalpy(eqnState) * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * velLeftNorm);

  //column two
  dF_dUl.SetData(2, 0, areaNorm.Y());
  dF_dUl.SetData(2, 1, left.U() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * areaNorm.X());
  dF_dUl.SetData(2, 2, left.V() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * areaNorm.Y() + velLeftNorm);
  dF_dUl.SetData(2, 3, left.W() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * areaNorm.Z());
  dF_dUl.SetData(2, 4, left.Enthalpy(eqnState) * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * velLeftNorm);

  //column three
  dF_dUl.SetData(3, 0, areaNorm.Z());
  dF_dUl.SetData(3, 1, left.U() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * areaNorm.X());
  dF_dUl.SetData(3, 2, left.V() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * areaNorm.Y());
  dF_dUl.SetData(3, 3, left.W() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * areaNorm.Z() + velLeftNorm);
  dF_dUl.SetData(3, 4, left.Enthalpy(eqnState) * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * velLeftNorm);

  //column four
  dF_dUl.SetData(4, 0, 0.0);
  dF_dUl.SetData(4, 1, (eqnState.Gamma() - 1.0) * areaNorm.X());
  dF_dUl.SetData(4, 2, (eqnState.Gamma() - 1.0) * areaNorm.Y());
  dF_dUl.SetData(4, 3, (eqnState.Gamma() - 1.0) * areaNorm.Z());
  dF_dUl.SetData(4, 4, eqnState.Gamma() * velLeftNorm);

  dF_dUl = 0.5 * (dF_dUl + A);


  //Compute derivative of flux wrt right conservative variables //////////////////////////////////////////////////////////////
  dF_dUr.Zero();

  //calculate flux derivatives
  //column zero
  dF_dUr.SetData(0, 0, 0.0);
  dF_dUr.SetData(1, 0, 0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() * areaNorm.X() - right.U() * velRightNorm);
  dF_dUr.SetData(2, 0, 0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() * areaNorm.Y() - right.V() * velRightNorm);
  dF_dUr.SetData(3, 0, 0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() * areaNorm.Z() - right.W() * velRightNorm);
  dF_dUr.SetData(4, 0, (0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() - right.Enthalpy(eqnState)) * velRightNorm); 
		       
  //column one
  dF_dUr.SetData(1, 0, areaNorm.X());
  dF_dUr.SetData(1, 1, right.U() * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * areaNorm.X() + velRightNorm);
  dF_dUr.SetData(1, 2, right.V() * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * areaNorm.Y());
  dF_dUr.SetData(1, 3, right.W() * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * areaNorm.Z());
  dF_dUr.SetData(1, 4, right.Enthalpy(eqnState) * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * velRightNorm);

  //column two
  dF_dUr.SetData(2, 0, areaNorm.Y());
  dF_dUr.SetData(2, 1, right.U() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * areaNorm.X());
  dF_dUr.SetData(2, 2, right.V() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * areaNorm.Y() + velRightNorm);
  dF_dUr.SetData(2, 3, right.W() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * areaNorm.Z());
  dF_dUr.SetData(2, 4, right.Enthalpy(eqnState) * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * velRightNorm);

  //column three
  dF_dUr.SetData(3, 0, areaNorm.Z());
  dF_dUr.SetData(3, 1, right.U() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * areaNorm.X());
  dF_dUr.SetData(3, 2, right.V() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * areaNorm.Y());
  dF_dUr.SetData(3, 3, right.W() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * areaNorm.Z() + velRightNorm);
  dF_dUr.SetData(3, 4, right.Enthalpy(eqnState) * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * velRightNorm);

  //column four
  dF_dUr.SetData(4, 0, 0.0);
  dF_dUr.SetData(4, 1, (eqnState.Gamma() - 1.0) * areaNorm.X());
  dF_dUr.SetData(4, 2, (eqnState.Gamma() - 1.0) * areaNorm.Y());
  dF_dUr.SetData(4, 3, (eqnState.Gamma() - 1.0) * areaNorm.Z());
  dF_dUr.SetData(4, 4, eqnState.Gamma() * velRightNorm);

  dF_dUr = 0.5 * (dF_dUr - A);

}


//function to calculate Lax-Friedrichs flux jacobians
void LaxFriedrichsFluxJacobian( const primVars &left, const primVars &right, const idealGas &eqnState, const vector3d<double>& areaVec, double &specRadL, double &specRadR, squareMatrix &dF_dUl, squareMatrix &dF_dUr){

  //left --> primative variables from left side
  //right --> primative variables from right side
  //eqnStat --> ideal gas equation of state
  //areaVec --> face area vector
  //maxWS --> maximum wave speed
  //dF_dUl --> dF/dUl, derivative of the Roe flux wrt the left state (conservative variables)
  //dF_dUr --> dF/dUlr, derivative of the Roe flux wrt the right state (conservative variables)


  //check to see that output matricies are correct size
  if( (dF_dUl.Size() != 5) || (dF_dUr.Size() != 5)){
    cerr << "ERROR: Input matricies to LaxFreidrichsFLuxJacobian function are not the correct size!" << endl;
  }

  vector3d<double> areaNorm = areaVec / areaVec.Mag();  //normalize area vector to unit vector

  //dot product of velocities with unit area vector
  vector3d<double> avgVel = 0.5 * (left.Velocity() + right.Velocity());
  double avgVelNorm = avgVel.DotProd(areaNorm);
  double avgSoS = 0.5 * (left.SoS(eqnState) + right.SoS(eqnState));

  double velLeftNorm = left.Velocity().DotProd(areaNorm);
  double velRightNorm = right.Velocity().DotProd(areaNorm);

  //calculate spectral radii
  //specRadL = fabs(velLeftNorm)  + left.SoS(eqnState);
  //specRadR = fabs(velRightNorm) + right.SoS(eqnState);

  specRadL = fabs(avgVelNorm) + avgSoS;
  specRadR = fabs(avgVelNorm) + avgSoS;

  //form spectral radii identity matrices
  squareMatrix dissLeft(5);
  dissLeft.Identity();
  dissLeft = specRadL * dissLeft;

  squareMatrix dissRight(5);
  dissRight.Identity();
  dissRight = specRadR * dissRight;

  //begin jacobian calculation ////////////////////////////////////////////////////////////////////////////////////////

  dF_dUl.Zero();
  dF_dUr.Zero();

  //calculate flux derivatives wrt left state
  //column zero
  dF_dUl.SetData(0, 0, 0.0);
  dF_dUl.SetData(1, 0, 0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() * areaNorm.X() - left.U() * velLeftNorm);
  dF_dUl.SetData(2, 0, 0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() * areaNorm.Y() - left.V() * velLeftNorm);
  dF_dUl.SetData(3, 0, 0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() * areaNorm.Z() - left.W() * velLeftNorm);
  dF_dUl.SetData(4, 0, (0.5 * (eqnState.Gamma() - 1.0) * left.Velocity().MagSq() - left.Enthalpy(eqnState)) * velLeftNorm); 
		       
  //column one
  dF_dUl.SetData(0, 1, areaNorm.X());
  dF_dUl.SetData(1, 1, left.U() * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * areaNorm.X() + velLeftNorm);
  dF_dUl.SetData(2, 1, left.V() * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * areaNorm.Y());
  dF_dUl.SetData(3, 1, left.W() * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * areaNorm.Z());
  dF_dUl.SetData(4, 1, left.Enthalpy(eqnState) * areaNorm.X() - (eqnState.Gamma() - 1.0) * left.U() * velLeftNorm);

  //column two
  dF_dUl.SetData(0, 2, areaNorm.Y());
  dF_dUl.SetData(1, 2, left.U() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * areaNorm.X());
  dF_dUl.SetData(2, 2, left.V() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * areaNorm.Y() + velLeftNorm);
  dF_dUl.SetData(3, 2, left.W() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * areaNorm.Z());
  dF_dUl.SetData(4, 2, left.Enthalpy(eqnState) * areaNorm.Y() - (eqnState.Gamma() - 1.0) * left.V() * velLeftNorm);

  //column three
  dF_dUl.SetData(0, 3, areaNorm.Z());
  dF_dUl.SetData(1, 3, left.U() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * areaNorm.X());
  dF_dUl.SetData(2, 3, left.V() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * areaNorm.Y());
  dF_dUl.SetData(3, 3, left.W() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * areaNorm.Z() + velLeftNorm);
  dF_dUl.SetData(4, 3, left.Enthalpy(eqnState) * areaNorm.Z() - (eqnState.Gamma() - 1.0) * left.W() * velLeftNorm);

  //column four
  dF_dUl.SetData(0, 4, 0.0);
  dF_dUl.SetData(1, 4, (eqnState.Gamma() - 1.0) * areaNorm.X());
  dF_dUl.SetData(2, 4, (eqnState.Gamma() - 1.0) * areaNorm.Y());
  dF_dUl.SetData(3, 4, (eqnState.Gamma() - 1.0) * areaNorm.Z());
  dF_dUl.SetData(4, 4, eqnState.Gamma() * velLeftNorm);

  dF_dUl = 0.5 * (dF_dUl + dissLeft);


  //calculate flux derivatives wrt right state
  //column zero
  dF_dUr.SetData(0, 0, 0.0);
  dF_dUr.SetData(1, 0, 0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() * areaNorm.X() - right.U() * velRightNorm);
  dF_dUr.SetData(2, 0, 0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() * areaNorm.Y() - right.V() * velRightNorm);
  dF_dUr.SetData(3, 0, 0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() * areaNorm.Z() - right.W() * velRightNorm);
  dF_dUr.SetData(4, 0, (0.5 * (eqnState.Gamma() - 1.0) * right.Velocity().MagSq() - right.Enthalpy(eqnState)) * velRightNorm); 
		       
  //column one
  dF_dUr.SetData(0, 1, areaNorm.X());
  dF_dUr.SetData(1, 1, right.U() * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * areaNorm.X() + velRightNorm);
  dF_dUr.SetData(2, 1, right.V() * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * areaNorm.Y());
  dF_dUr.SetData(3, 1, right.W() * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * areaNorm.Z());
  dF_dUr.SetData(4, 1, right.Enthalpy(eqnState) * areaNorm.X() - (eqnState.Gamma() - 1.0) * right.U() * velRightNorm);

  //column two
  dF_dUr.SetData(0, 2, areaNorm.Y());
  dF_dUr.SetData(1, 2, right.U() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * areaNorm.X());
  dF_dUr.SetData(2, 2, right.V() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * areaNorm.Y() + velRightNorm);
  dF_dUr.SetData(3, 2, right.W() * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * areaNorm.Z());
  dF_dUr.SetData(4, 2, right.Enthalpy(eqnState) * areaNorm.Y() - (eqnState.Gamma() - 1.0) * right.V() * velRightNorm);

  //column three
  dF_dUr.SetData(0, 3, areaNorm.Z());
  dF_dUr.SetData(1, 3, right.U() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * areaNorm.X());
  dF_dUr.SetData(2, 3, right.V() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * areaNorm.Y());
  dF_dUr.SetData(3, 3, right.W() * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * areaNorm.Z() + velRightNorm);
  dF_dUr.SetData(4, 3, right.Enthalpy(eqnState) * areaNorm.Z() - (eqnState.Gamma() - 1.0) * right.W() * velRightNorm);

  //column four
  dF_dUr.SetData(0, 4, 0.0);
  dF_dUr.SetData(1, 4, (eqnState.Gamma() - 1.0) * areaNorm.X());
  dF_dUr.SetData(2, 4, (eqnState.Gamma() - 1.0) * areaNorm.Y());
  dF_dUr.SetData(3, 4, (eqnState.Gamma() - 1.0) * areaNorm.Z());
  dF_dUr.SetData(4, 4, eqnState.Gamma() * velRightNorm);

  dF_dUr = 0.5 * (dF_dUr - dissRight);


}


//member function to return flux on boundaries
inviscidFlux BoundaryFlux( const string &bcName, const vector3d<double>& areaVec, const primVars &state1, const primVars &state2, const idealGas& eqnState, const input& inputVars, const string &surf, double &maxWS, const double up2face, const double upwind ){

  inviscidFlux flux;

  // vector3d<double> vel;
  //vector3d<double> velFace;

  vector3d<double> normArea = areaVec / areaVec.Mag();

  primVars state;

  if (bcName == "slipWall"){
    //state = (2.0 * state1) - (1.0 * state2);
    state = state1;
  }
  else{
    state = state1;
    //state = (2.0 * state1) - (1.0 * state2);
  }

  //Apply correct flux based on boundary condition to be applied 
  if ( bcName == "subsonicInflow" ){

    if (inputVars.Kappa() == -2.0 ){ //first order
      primVars ghostState1 = state.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	lState = ghostState1.FaceReconConst();
	rState = state.FaceReconConst();
      }
      else {
	rState = ghostState1.FaceReconConst();
	lState = state.FaceReconConst();
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS);
    }
    else{ //second order
      primVars ghostState1 = state.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars ghostState2 = ghostState1;
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	lState = ghostState1.FaceReconMUSCL( ghostState2, state1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	rState = state1.FaceReconMUSCL( state2, ghostState1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }
      else {
	rState = ghostState1.FaceReconMUSCL( ghostState2, state1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	lState = state1.FaceReconMUSCL( state2, ghostState1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS);

    }


  }
  else if ( bcName == "subsonicOutflow" ){

    if (inputVars.Kappa() == -2.0 ){ //first order
      primVars ghostState1 = state.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	rState = state.FaceReconConst();
	lState = ghostState1.FaceReconConst();
      }
      else {
	lState = state.FaceReconConst();
	rState = ghostState1.FaceReconConst();
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS);
    }
    else{ //second order
      primVars ghostState1 = state.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars ghostState2 = ghostState1;
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	rState = state1.FaceReconMUSCL( state2, ghostState1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	lState = ghostState1.FaceReconMUSCL( ghostState2, state1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }
      else {
	lState = state1.FaceReconMUSCL( state2, ghostState1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	rState = ghostState1.FaceReconMUSCL( ghostState2, state1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS); 

    }

  }
  else if ( bcName == "supersonicInflow" ){

    if (inputVars.Kappa() == -2.0 ){ //first order
      primVars ghostState1 = state.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	lState = ghostState1.FaceReconConst();
	rState = state.FaceReconConst();
      }
      else {
	rState = ghostState1.FaceReconConst();
	lState = state.FaceReconConst();
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS);
    }
    else{ //second order
      primVars ghostState1 = state.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars ghostState2 = ghostState1;
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	lState = ghostState1.FaceReconMUSCL( ghostState2, state1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	rState = state1.FaceReconMUSCL( state2, ghostState1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }
      else {
	rState = ghostState1.FaceReconMUSCL( ghostState2, state1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	lState = state1.FaceReconMUSCL( state2, ghostState1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS);

    }

  }
  else if ( bcName == "supersonicOutflow" ){

    if (inputVars.Kappa() == -2.0 ){ //first order
      primVars ghostState1 = state.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	rState = state.FaceReconConst();
	lState = ghostState1.FaceReconConst();
      }
      else {
	lState = state.FaceReconConst();
	rState = ghostState1.FaceReconConst();
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS);
    }
    else{ //second order
      primVars ghostState1 = state.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars ghostState2 = ghostState1;
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	rState = state1.FaceReconMUSCL( state2, ghostState1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	lState = ghostState1.FaceReconMUSCL( ghostState2, state1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }
      else {
	lState = state1.FaceReconMUSCL( state2, ghostState1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	rState = ghostState1.FaceReconMUSCL( ghostState2, state1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS); 

    }

  }
  else if ( bcName == "slipWall" || "viscousWall" ){

    if (inputVars.Kappa() == -2.0 ){ //first order
      primVars ghostState1 = state1.GetGhostState( "slipWall", normArea, surf, inputVars, eqnState );
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	rState = state1.FaceReconConst();
	lState = ghostState1.FaceReconConst();
      }
      else {
	lState = state1.FaceReconConst();
	rState = ghostState1.FaceReconConst();
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS);
    }
    else{ //second order
      primVars ghostState1 = state1.GetGhostState( "slipWall", normArea, surf, inputVars, eqnState );
      primVars ghostState2 = state2.GetGhostState( "slipWall", normArea, surf, inputVars, eqnState );
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	rState = state1.FaceReconMUSCL( state2, ghostState1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	lState = ghostState1.FaceReconMUSCL( ghostState2, state1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }
      else {
	lState = state1.FaceReconMUSCL( state2, ghostState1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	rState = ghostState1.FaceReconMUSCL( ghostState2, state1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS);

    }

  }
  else if ( bcName == "characteristic" ){

    if (inputVars.Kappa() == -2.0 ){ //first order
      primVars ghostState1 = state1.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	rState = state1.FaceReconConst();
	lState = ghostState1.FaceReconConst();
      }
      else {
	lState = state1.FaceReconConst();
	rState = ghostState1.FaceReconConst();
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS);
    }
    else{ //second order
      primVars ghostState1 = state1.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars ghostState2 = state2.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
      primVars lState, rState;

      if (surf == "il" || surf == "jl" || surf == "kl"){
	rState = state1.FaceReconMUSCL( state2, ghostState1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	lState = ghostState1.FaceReconMUSCL( ghostState2, state1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }
      else {
	lState = state1.FaceReconMUSCL( state2, ghostState1, "left", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
	rState = ghostState1.FaceReconMUSCL( ghostState2, state1, "right", inputVars.Kappa(), inputVars.Limiter(), up2face, upwind, up2face*2.0 );
      }

      flux = RoeFlux( lState, rState, eqnState, normArea, maxWS);

    }

  }
  
  else{
    cerr << "ERROR: Boundary condition " << bcName << " is not recognized!" << endl;
  }

  return flux;

}

squareMatrix BoundaryFluxJacobian( const string &bcName, const vector3d<double>& areaVec, const primVars &state, const idealGas& eqnState, const input& inputVars, const string &surf, const string &fluxJacType, double &maxWS){

  squareMatrix fluxJacL(5);
  squareMatrix fluxJacR(5);
  squareMatrix fluxJac(5);

  double maxWSL = 0.0;
  double maxWSR = 0.0;

  vector3d<double> normArea = areaVec / areaVec.Mag();

  //Apply correct flux based on boundary condition to be applied 
  if ( bcName == "subsonicInflow" || bcName == "subsonicOutflow" || bcName == "supersonicInflow" || bcName == "supersonicOutflow" || bcName == "characteristic"){

    primVars ghostState1 = state.GetGhostState( bcName, normArea, surf, inputVars, eqnState );
    primVars lState, rState;

    if (surf == "il" || surf == "jl" || surf == "kl"){
      lState = ghostState1.FaceReconConst();
      rState = state.FaceReconConst();

      if ( fluxJacType == "approximateRoe" ){
	ApproxRoeFluxJacobian( lState, rState, eqnState, normArea, maxWS, fluxJacL, fluxJacR);
      }
      else if ( fluxJacType == "exactRoe" ){
	RoeFluxJacobian( lState, rState, eqnState, normArea, maxWS, fluxJacL, fluxJacR);
      }
      else if ( fluxJacType == "laxFriedrichs" ){
	LaxFriedrichsFluxJacobian( lState, rState, eqnState, normArea, maxWSL, maxWSR, fluxJacL, fluxJacR);
	maxWS = maxWSR;
      }
      else{
	cerr << "ERROR: Inviscid flux jacobian type " << fluxJacType << " is not recognized!" << endl;
	exit(1);
      }

      fluxJac = fluxJacR;
    }
    else {
      rState = ghostState1.FaceReconConst();
      lState = state.FaceReconConst();

      if ( fluxJacType == "approximateRoe" ){
	ApproxRoeFluxJacobian( lState, rState, eqnState, normArea, maxWS, fluxJacL, fluxJacR);
      }
      else if ( fluxJacType == "exactRoe" ){
	RoeFluxJacobian( lState, rState, eqnState, normArea, maxWS, fluxJacL, fluxJacR);
      }
      else if ( fluxJacType == "laxFriedrichs" ){
	LaxFriedrichsFluxJacobian( lState, rState, eqnState, normArea, maxWSL, maxWSR, fluxJacL, fluxJacR);
	maxWS = maxWSL;
      }
      else{
	cerr << "ERROR: Inviscid flux jacobian type " << fluxJacType << " is not recognized!" << endl;
	exit(1);
      }

      fluxJac = fluxJacL;
    }

  }
  else if ( bcName == "slipWall" || "viscousWall" ){

    primVars lState, rState;

    fluxJac.Zero();
    //2nd row
    fluxJac.SetData(1, 0, 0.5 * (eqnState.Gamma() - 1.0) * state.Velocity().MagSq() * normArea.X() );
    fluxJac.SetData(1, 1, -1.0 * (eqnState.Gamma() - 1.0) * state.U() * normArea.X() );
    fluxJac.SetData(1, 2, -1.0 * (eqnState.Gamma() - 1.0) * state.V() * normArea.X() );
    fluxJac.SetData(1, 3, -1.0 * (eqnState.Gamma() - 1.0) * state.W() * normArea.X() );
    fluxJac.SetData(1, 4, (eqnState.Gamma() - 1.0) * normArea.X() );
    //3rd row
    fluxJac.SetData(2, 0, 0.5 * (eqnState.Gamma() - 1.0) * state.Velocity().MagSq() * normArea.Y() );
    fluxJac.SetData(2, 1, -1.0 * (eqnState.Gamma() - 1.0) * state.U() * normArea.Y() );
    fluxJac.SetData(2, 2, -1.0 * (eqnState.Gamma() - 1.0) * state.V() * normArea.Y() );
    fluxJac.SetData(2, 3, -1.0 * (eqnState.Gamma() - 1.0) * state.W() * normArea.Y() );
    fluxJac.SetData(2, 4, (eqnState.Gamma() - 1.0) * normArea.Y() );
    //4th row
    fluxJac.SetData(3, 0, 0.5 * (eqnState.Gamma() - 1.0) * state.Velocity().MagSq() * normArea.Z() );
    fluxJac.SetData(3, 1, -1.0 * (eqnState.Gamma() - 1.0) * state.U() * normArea.Z() );
    fluxJac.SetData(3, 2, -1.0 * (eqnState.Gamma() - 1.0) * state.V() * normArea.Z() );
    fluxJac.SetData(3, 3, -1.0 * (eqnState.Gamma() - 1.0) * state.W() * normArea.Z() );
    fluxJac.SetData(3, 4, (eqnState.Gamma() - 1.0) * normArea.Z() );

    maxWS = 0.0;
    maxWS = state.SoS(eqnState);

  }
  else{
    cerr << "ERROR: Boundary condition " << bcName << " is not recognized!" << endl;
  }

  return fluxJac;

}


//non-member functions -----------------------------------------------------------------------------------------------------------//

//operator overload for << - allows use of cout, cerr, etc.
ostream & operator<< (ostream &os, inviscidFlux &flux){

  os << flux.rhoVel << "   " << flux.rhoVelU << "   " << flux.rhoVelV << "   " << flux.rhoVelW << "   " << flux.rhoVelH << endl;

  return os;
}

//member function for scalar multiplication
inviscidFlux  inviscidFlux::operator * (const double &scalar){
  inviscidFlux temp = *this;
  temp.rhoVel *= scalar;
  temp.rhoVelU *= scalar;
  temp.rhoVelV *= scalar;
  temp.rhoVelW *= scalar;
  temp.rhoVelH *= scalar;
  return temp;
}

//friend function to allow multiplication from either direction
inviscidFlux operator* (const double &scalar, const inviscidFlux &flux){
  inviscidFlux temp;
  temp.SetRhoVel(flux.RhoVel() * scalar);
  temp.SetRhoVelU(flux.RhoVelU() * scalar);
  temp.SetRhoVelV(flux.RhoVelV() * scalar);
  temp.SetRhoVelW(flux.RhoVelW() * scalar);
  temp.SetRhoVelH(flux.RhoVelH() * scalar);
  return temp;
}

//operator overload for addition
inviscidFlux inviscidFlux::operator + (const inviscidFlux& invf2)const{
  inviscidFlux invf1 = *this;
  invf1.rhoVel += invf2.rhoVel;
  invf1.rhoVelU += invf2.rhoVelU;
  invf1.rhoVelV += invf2.rhoVelV;
  invf1.rhoVelW += invf2.rhoVelW;
  invf1.rhoVelH += invf2.rhoVelH;
  return invf1;
}

//operator overload for subtraction
inviscidFlux inviscidFlux::operator - (const inviscidFlux& invf2)const{
  inviscidFlux invf1 = *this;
  invf1.rhoVel -= invf2.rhoVel;
  invf1.rhoVelU -= invf2.rhoVelU;
  invf1.rhoVelV -= invf2.rhoVelV;
  invf1.rhoVelW -= invf2.rhoVelW;
  invf1.rhoVelH -= invf2.rhoVelH;
  return invf1;
}


//member function for scalar division
inviscidFlux  inviscidFlux::operator / (const double &scalar){
  inviscidFlux temp = *this;
  temp.rhoVel /= scalar;
  temp.rhoVelU /= scalar;
  temp.rhoVelV /= scalar;
  temp.rhoVelW /= scalar;
  temp.rhoVelH /= scalar;
  return temp;
}

//friend function to allow division from either direction
inviscidFlux operator/ (const double &scalar, const inviscidFlux &flux){
  inviscidFlux temp;
  temp.SetRhoVel(scalar / flux.RhoVel());
  temp.SetRhoVelU(scalar / flux.RhoVelU());
  temp.SetRhoVelV(scalar / flux.RhoVelV());
  temp.SetRhoVelW(scalar / flux.RhoVelW());
  temp.SetRhoVelH(scalar / flux.RhoVelH());
  return temp;
}


//convert the inviscid flux to a column matrix
colMatrix inviscidFlux::ConvertToColMatrix()const{
  colMatrix temp(5);
  temp.SetData(0, (*this).RhoVel());
  temp.SetData(1, (*this).RhoVelU());
  temp.SetData(2, (*this).RhoVelV());
  temp.SetData(3, (*this).RhoVelW());
  temp.SetData(4, (*this).RhoVelH());
  return temp;
}

//function to take in the primative variables, equation of state, face area vector, and conservative variable update and calculate the change in the convective flux
colMatrix ConvectiveFluxUpdate( const primVars &state, const idealGas &eqnState, const vector3d<double> &fArea, const colMatrix &du){

  inviscidFlux oldFlux(state, eqnState, fArea);

  primVars stateUpdate = state.UpdateWithConsVars(eqnState, du);
  inviscidFlux newFlux(stateUpdate, eqnState, fArea);

  inviscidFlux dFlux = newFlux - oldFlux;
    
  return dFlux.ConvertToColMatrix();
}


inviscidFlux LaxFriedrichsFlux( const primVars &left, const primVars &right, const idealGas &eqnState, const vector3d<double> &fArea, double &maxWS ){

  inviscidFlux lFlux(left, eqnState, fArea);
  inviscidFlux rFlux(right, eqnState, fArea);

  maxWS = ConvSpecRad(fArea, 0.5 * (left + right), eqnState);

  colMatrix lCons = left.ConsVars(eqnState);
  colMatrix rCons = right.ConsVars(eqnState);

  inviscidFlux lfFlux;
  lfFlux.SetRhoVel(  0.5 * (rFlux.RhoVel()  + lFlux.RhoVel()  - maxWS * (rCons.Data(0) - lCons.Data(0)) ) );
  lfFlux.SetRhoVelU( 0.5 * (rFlux.RhoVelU() + lFlux.RhoVelU() - maxWS * (rCons.Data(1) - lCons.Data(1)) ) );
  lfFlux.SetRhoVelV( 0.5 * (rFlux.RhoVelV() + lFlux.RhoVelV() - maxWS * (rCons.Data(2) - lCons.Data(2)) ) );
  lfFlux.SetRhoVelW( 0.5 * (rFlux.RhoVelW() + lFlux.RhoVelW() - maxWS * (rCons.Data(3) - lCons.Data(3)) ) );
  lfFlux.SetRhoVelH( 0.5 * (rFlux.RhoVelH() + lFlux.RhoVelH() - maxWS * (rCons.Data(4) - lCons.Data(4)) ) );

  return lfFlux;

}

//member function to take in an integer and string defining the face location, and the primative variables at a cell and calculate
//the convective spectral radius
double ConvSpecRad(const vector3d<double> &fArea, const primVars &state, const idealGas &eqnState){

  vector3d<double> normArea = fArea / fArea.Mag();
  double a = state.SoS(eqnState);
  double u = state.Velocity().DotProd(normArea);

  return fabs(u) + a;

}
