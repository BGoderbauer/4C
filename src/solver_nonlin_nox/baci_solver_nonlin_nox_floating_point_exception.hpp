/*----------------------------------------------------------------------------*/
/*! \file

\brief Bundle the floating point exception handling in one class.



\level 3

*/
/*----------------------------------------------------------------------------*/

#ifndef BACI_SOLVER_NONLIN_NOX_FLOATING_POINT_EXCEPTION_HPP
#define BACI_SOLVER_NONLIN_NOX_FLOATING_POINT_EXCEPTION_HPP

#include "baci_config.hpp"

#include <sstream>

BACI_NAMESPACE_OPEN

namespace NOX
{
  namespace NLN
  {
    /** \brief Class which is used to disable and enable the inherent floating
     *  point exception checks.
     *
     *  This class wraps the functionality needed to avoid system abortion calls
     *  because of floating point exceptions. This is mainly used in globalization
     *  techniques where a trial point may lead to a floating point exception but
     *  is rejected afterwards.
     *
     *  \author hiermeier \date 08/18 */
    class FloatingPointException
    {
     public:
      /** \brief Wrapper function which disables the floating point exception
       *  checks if desired.
       *
       *  \author hiermeier \date 08/18 */
      void precompute() const;

      /** \brief Wrapper function which checks, clears and re-enables the
       *  floating point exception checks if desired
       *
       *  \author hiermeier \date 08/18 */
      int postcompute(std::ostream& os) const;

      /** \brief Disable the automatic program abortion if a floating point
       *  exception is detected
       *
       *  \note This routine is only executed if the boolean \c shall_be_caught_
       *  is set to TRUE.
       *
       *  \author hiermeier \date 08/18 */
      void disable() const;

      /** \brief Check for any set floating point exception flags
       *
       *  \author hiermeier \date 08/18 */
      static int checkAndPrint(std::ostream& os);

      /** \brief Clear all potentially set floating point flags
       *
       *  \note This routine is only executed if the boolean \c shall_be_caught_
       *  is set to TRUE.
       *
       *  \author hiermeier \date 08/18 */
      void clear() const;

      /** \brief Enable the automatic program abortion if a floating point
       *  exception is detected
       *
       *  \note This routine is only executed if the boolean \c shall_be_caught_
       *  is set to TRUE.
       *
       *  \author hiermeier \date 08/18 */
      void enable() const;

      /** set this variable to TRUE if you want to disable the floating point
       *  exception checks for a moment. */
      bool shall_be_caught_ = false;
    };
  }  // namespace NLN
}  // namespace NOX

BACI_NAMESPACE_CLOSE

#endif  // SOLVER_NONLIN_NOX_FLOATING_POINT_EXCEPTION_H