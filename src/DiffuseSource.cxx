/** 
 * @file DiffuseSource.cxx
 * @brief DiffuseSource class implementation
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/Likelihood/src/DiffuseSource.cxx,v 1.15 2004/06/30 20:21:39 jchiang Exp $
 */

#include <vector>
#include <valarray>
#include <string>
#include <cmath>

#include "optimizers/Function.h"
#include "optimizers/dArg.h"
#include "optimizers/Exception.h"

#include "Likelihood/DiffuseSource.h"
#include "Likelihood/Event.h"
#include "Likelihood/ExposureMap.h"
#include "Likelihood/RoiCuts.h"
#include "Likelihood/TrapQuad.h"
#include "Likelihood/ResponseFunctions.h"

namespace Likelihood {

bool DiffuseSource::s_haveStaticMembers = false;
std::vector<double> DiffuseSource::s_energies;

DiffuseSource::DiffuseSource(optimizers::Function* spatialDist,
                             bool requireExposure) 
   throw(Exception) : m_spectrum(0) {
// The spatial distribution of emission is required for instantiation.
   m_spatialDist = spatialDist->clone();
   m_functions["SpatialDist"] = m_spatialDist;

   if (!s_haveStaticMembers 
       || RoiCuts::instance()->getEnergyCuts().first != s_energies.front()
       || RoiCuts::instance()->getEnergyCuts().second != s_energies.back()) {
      makeEnergyVector();
      s_haveStaticMembers = true;
   }

// In order to compute exposure, RoiCuts and spacecraft data must be
// available; furthermore, the ExposureMap object must have been
// instantiated.
   if (requireExposure) {
      ExposureMap *emap = ExposureMap::instance();
      if (emap == 0) {
         throw Exception("The ExposureMap is not defined.");
      } else {
         emap->integrateSpatialDist(s_energies, spatialDist, m_exposure);
      }
   }
   m_srcType = "Diffuse";
}

DiffuseSource::DiffuseSource(const DiffuseSource &rhs) : Source(rhs) {
// make a deep copy
   m_spatialDist = rhs.m_spatialDist->clone();
   m_functions["SpatialDist"] = m_spatialDist;

   m_spectrum = rhs.m_spectrum->clone();
   m_functions["Spectrum"] = m_spectrum;

   m_exposure = rhs.m_exposure;
   m_srcType = rhs.m_srcType;
}

double DiffuseSource::fluxDensity(const Event &evt) const {
   double my_fluxDensity;
   if (ResponseFunctions::useEdisp()) {
      const std::vector<double> & trueEnergies = evt.trueEnergies();
      const std::vector<double> & diffuseResponses 
         = evt.diffuseResponse(getName());
      unsigned int npts(trueEnergies.size());
      std::vector<double> my_integrand(npts);
      for (unsigned int k = 0; k < npts; k++) {
         optimizers::dArg energy_arg(trueEnergies[k]);
         my_integrand[k] = (*m_spectrum)(energy_arg)*diffuseResponses[k];
      }
      TrapQuad trapQuad(trueEnergies, my_integrand);
      my_fluxDensity = trapQuad.integral();
   } else {
      double trueEnergy = evt.getEnergy(); 
      optimizers::dArg energy_arg(trueEnergy);
      my_fluxDensity = (*m_spectrum)(energy_arg)
         *evt.diffuseResponse(trueEnergy, getName());
   }
   return my_fluxDensity;
}

double DiffuseSource::fluxDensityDeriv(const Event &evt, 
                                       const std::string &paramName) const {
// For now, just implement for spectral Parameters and neglect
// the spatial ones, "longitude" and "latitude".
//
// This method needs to be generalized for spectra that are
// CompositeFunctions.
   double my_fluxDensityDeriv;
   if (paramName == "Prefactor") {
      my_fluxDensityDeriv 
         = fluxDensity(evt)/m_spectrum->getParamValue("Prefactor");
   } else {
      if (ResponseFunctions::useEdisp()) {
         const std::vector<double> & trueEnergies = evt.trueEnergies();
         const std::vector<double> & diffuseResponses 
            = evt.diffuseResponse(getName());
         unsigned int npts(trueEnergies.size());
         std::vector<double> my_integrand(npts);
         for (unsigned int k = 0; k < npts; k++) {
            optimizers::dArg energy_arg(trueEnergies[k]);
            my_integrand[k] = m_spectrum->derivByParam(energy_arg, paramName)
               *diffuseResponses[k];
         }
         TrapQuad trapQuad(trueEnergies, my_integrand);
         my_fluxDensityDeriv = trapQuad.integral();
      } else {
         double trueEnergy = evt.getEnergy(); 
         optimizers::dArg energy_arg(trueEnergy);
         my_fluxDensityDeriv = m_spectrum->derivByParam(energy_arg, paramName)
            *evt.diffuseResponse(trueEnergy, getName());
      }
   }
   return my_fluxDensityDeriv;
}

double DiffuseSource::Npred() {
   optimizers::Function *specFunc = m_functions["Spectrum"];

// Evaluate the Npred integrand at the abscissa points contained in
// s_energies.
   
   std::vector<double> NpredIntegrand(s_energies.size());
   for (unsigned int k = 0; k < s_energies.size(); k++) {
      optimizers::dArg eArg(s_energies[k]);
      NpredIntegrand[k] = (*specFunc)(eArg)*m_exposure[k];
   }
   TrapQuad trapQuad(s_energies, NpredIntegrand);
   return trapQuad.integral();
}

double DiffuseSource::NpredDeriv(const std::string &paramName) {
   optimizers::Function *specFunc = m_functions["Spectrum"];

   if (paramName == std::string("Prefactor")) {
      return Npred()/specFunc->getParamValue("Prefactor");
   } else {  // loop over energies and fill integrand vector
      std::vector<double> myIntegrand(s_energies.size());
      for (unsigned int k = 0; k < s_energies.size(); k++) {
         optimizers::dArg eArg(s_energies[k]);
         myIntegrand[k] = specFunc->derivByParam(eArg, paramName)
            *m_exposure[k];
      }
      TrapQuad trapQuad(s_energies, myIntegrand);
      return trapQuad.integral();
   }
}
   
void DiffuseSource::makeEnergyVector(int nee) {
   RoiCuts *roiCuts = RoiCuts::instance();
   
// set up a logrithmic grid of energies for doing the integral over 
// the spectrum
   double emin = (roiCuts->getEnergyCuts()).first;
   double emax = (roiCuts->getEnergyCuts()).second;
   double estep = log(emax/emin)/(nee-1);
   
   s_energies.clear();
   s_energies.reserve(nee);
   for (int i = 0; i < nee; i++) {
      s_energies.push_back(emin*exp(i*estep));
   }
}

} // namespace Likelihood
