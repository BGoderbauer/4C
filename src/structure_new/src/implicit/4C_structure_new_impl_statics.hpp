/*-----------------------------------------------------------*/
/*! \file

\brief Static (time) integrator.


\level 3

*/
/*-----------------------------------------------------------*/

#ifndef FOUR_C_STRUCTURE_NEW_IMPL_STATICS_HPP
#define FOUR_C_STRUCTURE_NEW_IMPL_STATICS_HPP

#include "4C_config.hpp"

#include "4C_structure_new_impl_generic.hpp"

#include <NOX_Abstract_Vector.H>

FOUR_C_NAMESPACE_OPEN

namespace STR
{
  namespace IMPLICIT
  {
    class Statics : public Generic
    {
     public:
      //! constructor
      Statics();


      //! Setup the class variables
      void Setup() override;

      //! (derived)
      void PostSetup() override;

      //! Reset state variables (derived)
      void SetState(const Epetra_Vector& x) override;

      //! Apply the rhs only (derived)
      bool ApplyForce(const Epetra_Vector& x, Epetra_Vector& f) override;

      //! Apply the stiffness only (derived)
      bool ApplyStiff(const Epetra_Vector& x, CORE::LINALG::SparseOperator& jac) override;

      //! Apply force and stiff at once (derived)
      bool ApplyForceStiff(
          const Epetra_Vector& x, Epetra_Vector& f, CORE::LINALG::SparseOperator& jac) override;

      //! (derived)
      bool AssembleForce(Epetra_Vector& f,
          const std::vector<INPAR::STR::ModelType>* without_these_models = nullptr) const override;

      //! (derived)
      void WriteRestart(
          IO::DiscretizationWriter& iowriter, const bool& forced_writerestart) const override;

      //! (derived)
      void ReadRestart(IO::DiscretizationReader& ioreader) override;

      //! (derived)
      double CalcRefNormForce(const enum ::NOX::Abstract::Vector::NormType& type) const override;

      //! return time integration factor (derived)
      [[nodiscard]] double GetIntParam() const override;

      //! derived
      double GetModelValue(const Epetra_Vector& x) override;

      //! @name Monolithic update routines
      //! @{
      //! things that should be done before updating (derived)
      void PreUpdate() override;

      //! Update configuration after time step (derived)
      void UpdateStepState() override;

      //! Update everything on element level after time step and after output (derived)
      void UpdateStepElement() override;
      //! @}

      //! @name Predictor routines (dependent on the implicit integration scheme)
      //! @{
      /*! Predict constant displacements, consistent velocities and accelerations (derived) */
      void PredictConstDisConsistVelAcc(
          Epetra_Vector& disnp, Epetra_Vector& velnp, Epetra_Vector& accnp) const override;

      /*! Predict displacements based on constant velocities and consistent accelerations (derived)
       */
      bool PredictConstVelConsistAcc(
          Epetra_Vector& disnp, Epetra_Vector& velnp, Epetra_Vector& accnp) const override;

      /*! Predict displacements based on constant accelerations and consistent velocities (derived)
       */
      bool PredictConstAcc(
          Epetra_Vector& disnp, Epetra_Vector& velnp, Epetra_Vector& accnp) const override;
      //! @}

      //! @name Attribute access functions
      //@{

      //! Return name
      enum INPAR::STR::DynamicType MethodName() const override { return INPAR::STR::dyna_statics; }

      //! Provide number of steps, a single-step method returns 1
      int MethodSteps() const override { return 1; }

      //! Give local order of accuracy of displacement part
      int MethodOrderOfAccuracyDis() const override
      {
        FOUR_C_THROW("Sensible to ask?");
        return 0;
      }

      //! Give local order of accuracy of velocity part
      int MethodOrderOfAccuracyVel() const override
      {
        FOUR_C_THROW("Sensible to ask?");
        return 0;
      }

      //! Return linear error coefficient of displacements
      double MethodLinErrCoeffDis() const override
      {
        FOUR_C_THROW("Sensible to ask?");
        return 0.0;
      }

      //! Return linear error coefficient of velocities
      double MethodLinErrCoeffVel() const override
      {
        FOUR_C_THROW("Sensible to ask?");
        return 0.0;
      }

      //@}

     protected:
      /*! \brief Add the viscous and mass contributions to the right hand side (TR-rule)
       *
       * \remark Nothing needs to be done in the static case. */
      void AddViscoMassContributions(Epetra_Vector& f) const override{};

      /*! \brief Add the viscous and mass contributions to the jacobian (TR-rule)
       *
       * \remark Nothing needs to be done in the static case. */
      void AddViscoMassContributions(CORE::LINALG::SparseOperator& jac) const override{};

      //! reset the time step dependent parameters for the element evaluation [derived]
      void ResetEvalParams() override;
    };
  }  // namespace IMPLICIT
}  // namespace STR


FOUR_C_NAMESPACE_CLOSE

#endif