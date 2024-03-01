/*--------------------------------------------------------------------------*/
/*! \file

\brief Interface of Lubrication element evaluation routine

\level 3

*/
/*--------------------------------------------------------------------------*/

#ifndef BACI_LUBRICATION_ELE_INTERFACE_HPP
#define BACI_LUBRICATION_ELE_INTERFACE_HPP

#include "baci_config.hpp"

#include "baci_lib_element.hpp"
#include "baci_linalg_serialdensematrix.hpp"
#include "baci_linalg_serialdensevector.hpp"

#include <Teuchos_ParameterList.hpp>

#include <vector>

BACI_NAMESPACE_OPEN

namespace DRT
{
  class Discretization;
  class Element;

  namespace ELEMENTS
  {
    /// Interface base class for LubricationEleCalc
    /*!
      This class exists to provide a common interface for all template
      versions of LubricationEleCalc.
     */
    class LubricationEleInterface
    {
     public:
      /**
       * Virtual destructor.
       */
      virtual ~LubricationEleInterface() = default;

      /// Default constructor.
      LubricationEleInterface() = default;

      /// Setup element evaluation
      virtual int SetupCalc(DRT::Element* ele, DRT::Discretization& discretization) = 0;

      /// Evaluate the element
      /*!
        This class does not provide a definition for this function; it
        must be defined in LubricationEleCalc.
        The Evaluate() method is meant only for the assembling of the
        linearized matrix and the right hand side
       */
      virtual int Evaluate(DRT::Element* ele, Teuchos::ParameterList& params,
          DRT::Discretization& discretization, DRT::Element::LocationArray& la,
          CORE::LINALG::SerialDenseMatrix& elemat1_epetra,
          CORE::LINALG::SerialDenseMatrix& elemat2_epetra,
          CORE::LINALG::SerialDenseVector& elevec1_epetra,
          CORE::LINALG::SerialDenseVector& elevec2_epetra,
          CORE::LINALG::SerialDenseVector& elevec3_epetra) = 0;

      virtual int EvaluateEHLMon(DRT::Element* ele, Teuchos::ParameterList& params,
          DRT::Discretization& discretization, DRT::Element::LocationArray& la,
          CORE::LINALG::SerialDenseMatrix& elemat1_epetra,
          CORE::LINALG::SerialDenseMatrix& elemat2_epetra,
          CORE::LINALG::SerialDenseVector& elevec1_epetra,
          CORE::LINALG::SerialDenseVector& elevec2_epetra,
          CORE::LINALG::SerialDenseVector& elevec3_epetra) = 0;

      /*!
        This class does not provide a definition for this function; it
        must be defined in LubricationEleCalc.
        The EvaluateService() method is meant for everything not related to
        the assembling of the linearized matrix and the right hand side
      */
      virtual int EvaluateService(DRT::Element* ele, Teuchos::ParameterList& params,
          DRT::Discretization& discretization, DRT::Element::LocationArray& la,
          CORE::LINALG::SerialDenseMatrix& elemat1_epetra,
          CORE::LINALG::SerialDenseMatrix& elemat2_epetra,
          CORE::LINALG::SerialDenseVector& elevec1_epetra,
          CORE::LINALG::SerialDenseVector& elevec2_epetra,
          CORE::LINALG::SerialDenseVector& elevec3_epetra) = 0;
    };
  }  // namespace ELEMENTS
}  // namespace DRT

BACI_NAMESPACE_CLOSE

#endif  // LUBRICATION_ELE_INTERFACE_H