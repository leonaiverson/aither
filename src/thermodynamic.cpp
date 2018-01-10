/*  This file is part of aither.
    Copyright (C) 2015-17  Michael Nucci (michael.nucci@gmail.com)

    Aither is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Aither is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <iostream>     // cout
#include "thermodynamic.hpp"
#include "utility.hpp"  // FindRoot
#include "fluid.hpp"
#include "macros.hpp"

using std::cout;
using std::endl;
using std::cerr;

// constructor
caloricallyPerfect::caloricallyPerfect(const vector<fluid>& fl,
                                       const double& tRef, const double& aRef) {
  const auto numSpecies = fl.size();
  n_.reserve(numSpecies);
  gasConst_.reserve(numSpecies);
  for (auto &f : fl) {
    n_.push_back(f.N());
    gasConst_.push_back(f.GasConstant() * tRef / (aRef * aRef));
  }
}

thermallyPerfect::thermallyPerfect(const vector<fluid>& fl, const double& tRef,
                                   const double& aRef)
    : caloricallyPerfect(fl, tRef, aRef) {
  vibTemp_.reserve(fl.size());
  for (auto& f : fl) {
    vibTemp_.push_back(f.VibrationalTemperature());
  }
}

// ---------------------------------------------------------------------------
// shared functions
double thermodynamic::Cp(const double& t, const vector<double>& mf) const {
  MSG_ASSERT(this->NumSpecies() == static_cast<int>(mf.size()),
             "species size mismatch");
  auto cp = 0.0;
  for (auto ss = 0; ss < this->NumSpecies(); ++ss) {
    cp += mf[ss] * this->SpeciesCp(t, ss);
  }
  return cp;
}

double thermodynamic::Cv(const double& t, const vector<double>& mf) const {
  MSG_ASSERT(this->NumSpecies() == static_cast<int>(mf.size()),
             "species size mismatch");
  auto cv = 0.0;
  for (auto ss = 0; ss < this->NumSpecies(); ++ss) {
    cv += mf[ss] * this->SpeciesCv(t, ss);
  }
  return cv;
}

// ---------------------------------------------------------------------------
// Member functions for calorically perfect class
double caloricallyPerfect::TemperatureFromSpecEnergy(
    const double& e, const vector<double>& mf) const {
  const auto t = 1.0;  // cpg has constant Cv, so value of t is meaningless
  return e / this->Cv(t, mf);
}

// ---------------------------------------------------------------------------
// thermally perfect functions
double thermallyPerfect::SpeciesSpecEnergy(const double& t,
                                           const int& ss) const {
  MSG_ASSERT(ss <= this->NumSpecies(), "species out of range");
  return this->R(ss) * (this->N(ss) * t + this->VibEqTerm(t, ss));
}

double thermallyPerfect::SpecEnergy(const double& t,
                                    const vector<double>& mf) const {
  MSG_ASSERT(this->NumSpecies() == static_cast<int>(mf.size()),
             "species size mismatch");
  auto e = 0.0;
  for (auto ss = 0; ss < this->NumSpecies(); ++ss) {
    e += mf[ss] * this->SpeciesSpecEnergy(t, ss);
  }
  return e;
}

double thermallyPerfect::SpeciesSpecEnthalpy(const double& t,
                                             const int& ss) const {
  MSG_ASSERT(ss <= this->NumSpecies(), "species out of range");
  return this->R(ss) * ((this->N(ss) + 1) * t + this->VibEqTerm(t, ss));
}

double thermallyPerfect::SpecEnthalpy(const double& t,
                                      const vector<double>& mf) const {
  MSG_ASSERT(this->NumSpecies() == static_cast<int>(mf.size()),
             "species size mismatch");
  auto h = 0.0;
  for (auto ss = 0; ss < this->NumSpecies(); ++ss) {
    h += mf[ss] * this->SpeciesSpecEnthalpy(t, ss);
  }
  return h;
}

double thermallyPerfect::TemperatureFromSpecEnergy(
    const double& e, const vector<double>& mf) const {
  auto temperature = 0.0;
  auto func = [&](const double& t) {
    temperature = t;
    return e - this->SpecEnergy(t, mf);
  };
  FindRoot(func, 1.0e-8, 1.0e4, 1.0e-8);

  return temperature;
}