/*-----------------------------------------------------------*/
/*! \file



\level 3

*/
/*-----------------------------------------------------------*/

#ifndef BACI_SOLVER_NONLIN_NOX_SOLVER_PREPOSTOP_GENERIC_HPP
#define BACI_SOLVER_NONLIN_NOX_SOLVER_PREPOSTOP_GENERIC_HPP

#include "baci_config.hpp"

#include <NOX_Observer.hpp>

BACI_NAMESPACE_OPEN

namespace NOX
{
  namespace NLN
  {
    namespace Solver
    {
      namespace PrePostOp
      {
        /*! \brief Non-linear solver helper class
         *
         * The NOX::NLN::Solver s are supposed to use this helper class to save
         * solver dependent things, such as the number of nonlinear solver steps.
         *
         * ToDo Extend the functionality to non-linear solvers which are not
         * derived from the LineSearchBased class.
         *
         * \author Michael Hiermeier */

        class Generic : public ::NOX::Observer
        {
         public:
          //! constructor
          Generic();

          void runPreIterate(const ::NOX::Solver::Generic& solver) override;

          void runPreSolve(const ::NOX::Solver::Generic& nlnSolver) override;
        };
      }  // namespace PrePostOp
    }    // namespace Solver
  }      // namespace NLN
}  // namespace NOX

BACI_NAMESPACE_CLOSE

#endif  // SOLVER_NONLIN_NOX_SOLVER_PREPOSTOP_GENERIC_H