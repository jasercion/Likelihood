/** 
 * @file LogGaussian.h
 * @brief Declaration for the LogGaussian Function class
 * @author J. Chiang
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/ScienceTools-scons/Likelihood/Likelihood/LogGaussian.h,v 1.1 2011/01/29 06:53:03 jchiang Exp $
 */

#ifndef Likelihood_LogGaussian_h
#define Likelihood_LogGaussian_h

#include "optimizers/Arg.h"
#include "optimizers/Function.h"

namespace Likelihood {

/** 
 * @class LogGaussian
 *
 * @brief A power-law function that uses integrated flux and index
 * as free parameters and upper and lower bounds of integration as 
 * fixed parameters.
 *
 * @author J. Chiang
 *    
 * $Header: /nfs/slac/g/glast/ground/cvs/ScienceTools-scons/Likelihood/Likelihood/LogGaussian.h,v 1.1 2011/01/29 06:53:03 jchiang Exp $
 */
    
class LogGaussian : public optimizers::Function {

public:

   LogGaussian(double norm=1, double mean=0, double sigma=1);

   double value(optimizers::Arg & arg) const;

   double derivByParam(optimizers::Arg & x, 
                       const std::string & paramName) const;

   double derivative(optimizers::Arg & x) const;
   
   virtual Function * clone() const {
      return new LogGaussian(*this);
   }

private:

   double m_log_term;

};

} // namespace Likelihood

#endif // Likelihood_LogGaussian_h