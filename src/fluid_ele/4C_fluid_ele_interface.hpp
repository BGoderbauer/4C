/*----------------------------------------------------------------------*/
/*! \file

\brief Interface class for the evaluation routines of the fluid element


\level 1

*/
/*----------------------------------------------------------------------*/

#ifndef FOUR_C_FLUID_ELE_INTERFACE_HPP
#define FOUR_C_FLUID_ELE_INTERFACE_HPP

#include "4C_config.hpp"

#include "4C_cut_utils.hpp"
#include "4C_discretization_fem_general_utils_gausspoints.hpp"
#include "4C_linalg_serialdensematrix.hpp"
#include "4C_linalg_serialdensevector.hpp"

#include <Epetra_Vector.h>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>

#include <map>
#include <vector>

FOUR_C_NAMESPACE_OPEN

namespace MAT
{
  class Material;
}

namespace CORE::GEO
{
  namespace CUT
  {
    class BoundaryCell;
    class VolumeCell;
  }  // namespace CUT
}  // namespace CORE::GEO

namespace XFEM
{
  class ConditionManager;
  class MeshCouplingFluidFluid;
}  // namespace XFEM

namespace DRT
{
  class Discretization;

  namespace ELEMENTS
  {
    class Fluid;

    /// Interface base class for FluidEleCalc
    /*!
      This class exists to provide a common interface for all template
      versions of FluidEleCalc. The only function this class actually defines
      is Ele, which returns a pointer to the appropriate version of FluidEleCalc.
     */
    class FluidEleInterface
    {
     public:
      /**
       * Virtual destructor.
       */
      virtual ~FluidEleInterface() = default;

      /// Empty constructor
      FluidEleInterface() = default;

      /// Evaluate the element
      /*!
        This class does not provide a definition for this function; it
        must be defined in FluidEleCalc.
       */
      virtual int Evaluate(DRT::ELEMENTS::Fluid* ele, DRT::Discretization& discretization,
          const std::vector<int>& lm, Teuchos::ParameterList& params,
          Teuchos::RCP<CORE::MAT::Material>& mat, CORE::LINALG::SerialDenseMatrix& elemat1_epetra,
          CORE::LINALG::SerialDenseMatrix& elemat2_epetra,
          CORE::LINALG::SerialDenseVector& elevec1_epetra,
          CORE::LINALG::SerialDenseVector& elevec2_epetra,
          CORE::LINALG::SerialDenseVector& elevec3_epetra, bool offdiag = false) = 0;

      /// evaluate element at specified Gauss points
      virtual int Evaluate(DRT::ELEMENTS::Fluid* ele, DRT::Discretization& discretization,
          const std::vector<int>& lm, Teuchos::ParameterList& params,
          Teuchos::RCP<CORE::MAT::Material>& mat, CORE::LINALG::SerialDenseMatrix& elemat1_epetra,
          CORE::LINALG::SerialDenseMatrix& elemat2_epetra,
          CORE::LINALG::SerialDenseVector& elevec1_epetra,
          CORE::LINALG::SerialDenseVector& elevec2_epetra,
          CORE::LINALG::SerialDenseVector& elevec3_epetra,
          const CORE::FE::GaussIntegration& intpoints, bool offdiag = false) = 0;

      /// Evaluate the XFEM cut element
      virtual int EvaluateXFEM(DRT::ELEMENTS::Fluid* ele, DRT::Discretization& discretization,
          const std::vector<int>& lm, Teuchos::ParameterList& params,
          Teuchos::RCP<CORE::MAT::Material>& mat, CORE::LINALG::SerialDenseMatrix& elemat1_epetra,
          CORE::LINALG::SerialDenseMatrix& elemat2_epetra,
          CORE::LINALG::SerialDenseVector& elevec1_epetra,
          CORE::LINALG::SerialDenseVector& elevec2_epetra,
          CORE::LINALG::SerialDenseVector& elevec3_epetra,
          const std::vector<CORE::FE::GaussIntegration>& intpoints,
          const CORE::GEO::CUT::plain_volumecell_set& cells, bool offdiag = false) = 0;

      virtual int IntegrateShapeFunction(DRT::ELEMENTS::Fluid* ele,
          DRT::Discretization& discretization, const std::vector<int>& lm,
          CORE::LINALG::SerialDenseVector& elevec1,
          const CORE::FE::GaussIntegration& intpoints) = 0;

      virtual int IntegrateShapeFunctionXFEM(DRT::ELEMENTS::Fluid* ele,
          DRT::Discretization& discretization, const std::vector<int>& lm,
          CORE::LINALG::SerialDenseVector& elevec1,
          const std::vector<CORE::FE::GaussIntegration>& intpoints,
          const CORE::GEO::CUT::plain_volumecell_set& cells) = 0;

      /// Evaluate supporting methods of the element
      virtual int EvaluateService(DRT::ELEMENTS::Fluid* ele, Teuchos::ParameterList& params,
          Teuchos::RCP<CORE::MAT::Material>& mat, DRT::Discretization& discretization,
          std::vector<int>& lm, CORE::LINALG::SerialDenseMatrix& elemat1,
          CORE::LINALG::SerialDenseMatrix& elemat2, CORE::LINALG::SerialDenseVector& elevec1,
          CORE::LINALG::SerialDenseVector& elevec2, CORE::LINALG::SerialDenseVector& elevec3) = 0;

      virtual int ComputeError(DRT::ELEMENTS::Fluid* ele, Teuchos::ParameterList& params,
          Teuchos::RCP<CORE::MAT::Material>& mat, DRT::Discretization& discretization,
          std::vector<int>& lm, CORE::LINALG::SerialDenseVector& elevec1,
          const CORE::FE::GaussIntegration& intpoints2) = 0;

      virtual int ComputeErrorInterface(DRT::ELEMENTS::Fluid* ele,   ///< fluid element
          DRT::Discretization& dis,                                  ///< background discretization
          const std::vector<int>& lm,                                ///< element local map
          const Teuchos::RCP<XFEM::ConditionManager>& cond_manager,  ///< XFEM condition manager
          Teuchos::RCP<CORE::MAT::Material>& mat,                    ///< material
          CORE::LINALG::SerialDenseVector& ele_interf_norms,  /// squared element interface norms
          const std::map<int, std::vector<CORE::GEO::CUT::BoundaryCell*>>&
              bcells,  ///< boundary cells
          const std::map<int, std::vector<CORE::FE::GaussIntegration>>&
              bintpoints,                                     ///< boundary integration points
          const CORE::GEO::CUT::plain_volumecell_set& vcSet,  ///< set of plain volume cells
          Teuchos::ParameterList& params                      ///< parameter list
          ) = 0;

      virtual void ElementXfemInterfaceHybridLM(DRT::ELEMENTS::Fluid* ele,  ///< fluid element
          DRT::Discretization& dis,                                  ///< background discretization
          const std::vector<int>& lm,                                ///< element local map
          const Teuchos::RCP<XFEM::ConditionManager>& cond_manager,  ///< XFEM condition manager
          const std::vector<CORE::FE::GaussIntegration>& intpoints,  ///< element gauss points
          const std::map<int, std::vector<CORE::GEO::CUT::BoundaryCell*>>&
              bcells,  ///< boundary cells
          const std::map<int, std::vector<CORE::FE::GaussIntegration>>&
              bintpoints,  ///< boundary integration points
          const std::map<int, std::vector<int>>&
              patchcouplm,  ///< lm vectors for coupling elements, key= global coupling side-Id
          std::map<int, std::vector<CORE::LINALG::SerialDenseMatrix>>&
              side_coupling,                       ///< side coupling matrices
          Teuchos::ParameterList& params,          ///< parameter list
          Teuchos::RCP<CORE::MAT::Material>& mat,  ///< material
          CORE::LINALG::SerialDenseMatrix&
              elemat1_epetra,  ///< local system matrix of intersected element
          CORE::LINALG::SerialDenseVector&
              elevec1_epetra,                      ///< local element vector of intersected element
          CORE::LINALG::SerialDenseMatrix& Cuiui,  ///< coupling matrix of a side with itself
          const CORE::GEO::CUT::plain_volumecell_set& vcSet  ///< set of plain volume cells
          ) = 0;

      /// add interface condition at cut to element matrix and rhs (two-sided Nitsche coupling)
      virtual void ElementXfemInterfaceNIT(DRT::ELEMENTS::Fluid* ele,  ///< fluid element
          DRT::Discretization& dis,                                  ///< background discretization
          const std::vector<int>& lm,                                ///< element local map
          const Teuchos::RCP<XFEM::ConditionManager>& cond_manager,  ///< XFEM condition manager
          const std::map<int, std::vector<CORE::GEO::CUT::BoundaryCell*>>&
              bcells,  ///< boundary cells
          const std::map<int, std::vector<CORE::FE::GaussIntegration>>&
              bintpoints,  ///< boundary integration points
          const std::map<int, std::vector<int>>& patchcouplm,
          Teuchos::ParameterList& params,                     ///< parameter list
          Teuchos::RCP<CORE::MAT::Material>& mat_master,      ///< material for the coupled side
          Teuchos::RCP<CORE::MAT::Material>& mat_slave,       ///< material for the coupled side
          CORE::LINALG::SerialDenseMatrix& elemat1_epetra,    ///< element matrix
          CORE::LINALG::SerialDenseVector& elevec1_epetra,    ///< element vector
          const CORE::GEO::CUT::plain_volumecell_set& vcSet,  ///< volumecell sets in this element
          std::map<int, std::vector<CORE::LINALG::SerialDenseMatrix>>&
              side_coupling,                       ///< side coupling matrices
          CORE::LINALG::SerialDenseMatrix& Cuiui,  ///< ui-ui coupling matrix
          bool evaluated_cut  ///< the CUT was updated before this evaluation is called
          ) = 0;

      virtual void CalculateContinuityXFEM(DRT::ELEMENTS::Fluid* ele, DRT::Discretization& dis,
          const std::vector<int>& lm, CORE::LINALG::SerialDenseVector& elevec1_epetra,
          const CORE::FE::GaussIntegration& intpoints) = 0;

      virtual void CalculateContinuityXFEM(DRT::ELEMENTS::Fluid* ele, DRT::Discretization& dis,
          const std::vector<int>& lm, CORE::LINALG::SerialDenseVector& elevec1_epetra) = 0;
    };

  }  // namespace ELEMENTS
}  // namespace DRT

FOUR_C_NAMESPACE_CLOSE

#endif