/**
 * @file MeanPsf.h
 * @brief Position-dependent Psf averaged over an observation period.
 * @author J. Chiang
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/Likelihood/Likelihood/MeanPsf.h,v 1.1 2004/10/04 05:51:19 jchiang Exp $
 */

#ifndef Likelihood_MeanPsf_h
#define Likelihood_MeanPsf_h

#include "astro/SkyDir.h"

#include "map_tools/Exposure.h"

namespace Likelihood {

/**
 * @class MeanPsf
 *
 */

class MeanPsf {

public:

   MeanPsf(double ra, double dec, const std::vector<double> & energies) 
      : m_srcDir(ra, dec, astro::SkyDir::EQUATORIAL),
        m_energies(energies) {
      init();
   }

   MeanPsf(const astro::SkyDir & srcDir, const std::vector<double> & energies) 
      : m_srcDir(srcDir), m_energies(energies) {
      init();
   }

   /// @return The value of the psf.
   /// @param energy True photon energy (MeV)
   /// @param theta Inclination wrt instrument z-axis (degrees)
   /// @param phi Azimuthal angle wrt instrument x-axis (degrees)
   double operator()(double energy, double theta, double phi=0) const;

private:

   static std::vector<double> s_separations;

   astro::SkyDir m_srcDir;

   std::vector<double> m_energies;

   std::vector<double> m_psfValues;

   void init();

   void createLogArray(double xmin, double xmax, unsigned int npts,
                       std::vector<double> & xx) const;


   class Psf : public map_tools::Exposure::Aeff {
   public:
      Psf(double separation, double energy, int evtType) 
         : m_separation(separation), m_energy(energy), m_evtType(evtType) {}
      virtual ~Psf() {}
      virtual double operator()(double cosTheta) const;
   private:
      double m_separation;
      double m_energy;
      int m_evtType;
      static double s_phi;
   };

   class Aeff : public map_tools::Exposure::Aeff {
   public:
      Aeff(double energy, int evtType) 
         : m_energy(energy), m_evtType(evtType) {}
      virtual ~Aeff() {}
      virtual double operator()(double cosTheta) const;
   private:
      double m_separation;
      double m_energy;
      int m_evtType;
      static double s_phi;
   };

};

}

#endif // Likelihood_MeanPsf_h
