/*-----------------------------------------------------------------------------------------------*/
/*! \file

\brief base class for all beam elements

\level 2

*/
/*-----------------------------------------------------------------------------------------------*/

#ifndef FOUR_C_BEAM3_BASE_HPP
#define FOUR_C_BEAM3_BASE_HPP

#include "4C_config.hpp"

#include "4C_beam3_spatial_discretization_utils.hpp"
#include "4C_discretization_fem_general_utils_fem_shapefunctions.hpp"
#include "4C_discretization_fem_general_utils_integration.hpp"
#include "4C_inpar_beaminteraction.hpp"
#include "4C_inpar_structure.hpp"
#include "4C_lib_discret.hpp"
#include "4C_lib_element.hpp"
#include "4C_lib_elementtype.hpp"
#include "4C_utils_local_newton.hpp"

#include <Epetra_Vector.h>
#include <Teuchos_RCP.hpp>
#include <Teuchos_StandardParameterEntryValidators.hpp>

FOUR_C_NAMESPACE_OPEN


// forward declaration ...
namespace CORE::LINALG
{
  class SerialDenseVector;
  class SerialDenseMatrix;
}  // namespace CORE::LINALG
namespace CORE::GEO
{
  namespace MESHFREE
  {
    class BoundingBox;
  }
}  // namespace CORE::GEO
namespace STR
{
  namespace ELEMENTS
  {
    class ParamsInterface;
  }
}  // namespace STR

namespace BROWNIANDYN
{
  class ParamsInterface;
}

namespace MAT
{
  class BeamMaterial;
  template <typename T>
  class BeamMaterialTemplated;
}  // namespace MAT

namespace DRT
{
  namespace ELEMENTS
  {
    //! base class for all beam elements
    class Beam3Base : public DRT::Element
    {
     public:
      /*!
      \brief Standard Constructor

      \param id    (in): A globally unique element id
      \param etype (in): Type of element
      \param owner (in): owner processor of the element
      */
      Beam3Base(int id, int owner);

      /*!
      \brief Copy Constructor

      Makes a deep copy of a Element
      */
      Beam3Base(const Beam3Base& old);

      /*!
      \brief Pack this class so it can be communicated

      \ref Pack and \ref Unpack are used to communicate this element

      */
      void Pack(CORE::COMM::PackBuffer& data) const override;

      /*!
      \brief Unpack data from a char vector into this class

      \ref Pack and \ref Unpack are used to communicate this element

      */
      void Unpack(const std::vector<char>& data) override;

      /** \brief set the parameter interface ptr for the solid elements
       *
       *  \param p (in): Parameter list coming from the time integrator.
       *
       *  \author hiermeier
       *  \date 04/16 */
      void SetParamsInterfacePtr(const Teuchos::ParameterList& p) override;

      virtual void SetBrownianDynParamsInterfacePtr();

      /** \brief returns true if the parameter interface is defined and initialized, otherwise false
       *
       *  \author hiermeier
       *  \date 04/16 */
      inline bool IsParamsInterface() const override { return (not interface_ptr_.is_null()); }

      /** \brief get access to the parameter interface pointer
       *
       *  \author hiermeier
       *  \date 04/16 */
      Teuchos::RCP<DRT::ELEMENTS::ParamsInterface> ParamsInterfacePtr() override;
      virtual Teuchos::RCP<BROWNIANDYN::ParamsInterface> BrownianDynParamsInterfacePtr() const;

      //! computes the number of different random numbers required in each time step for generation
      //! of stochastic forces
      virtual int HowManyRandomNumbersINeed() const = 0;

      /** \brief get access to the element reference length
       *        (i.e. arc-length in stress-free configuration)
       *
       *  \author grill
       *  \date 05/16 */
      virtual double RefLength() const = 0;

      /** \brief get the radius of the element which is used for interactions (contact, viscous,
       *         potential-based, ...)
       *         - if needed, extend this to other than circular cross-section shapes and dimensions
       *           to be specified via input file
       *         - allow for different assumed shapes for different interaction types if needed
       *
       *  \author grill
       *  \date 02/17 */
      double GetCircularCrossSectionRadiusForInteractions() const;

      /** \brief get number of nodes used for centerline interpolation
       *
       *  \author grill
       *  \date 05/16 */
      virtual int NumCenterlineNodes() const = 0;

      /** \brief find out whether given node is used for centerline interpolation
       *
       *  \author grill
       *  \date 10/16 */
      virtual bool IsCenterlineNode(const DRT::Node& node) const = 0;

      /** \brief return GIDs of all additive DoFs for a given node
       *
       *  \author grill
       *  \date 07/16 */
      std::vector<int> GetAdditiveDofGIDs(
          const DRT::Discretization& discret, const DRT::Node& node) const;

      /** \brief return GIDs of all non-additive, i.e. rotation pseudo vector DoFs for a given node
       *
       *  \author grill
       *  \date 07/16 */
      std::vector<int> GetRotVecDofGIDs(
          const DRT::Discretization& discret, const DRT::Node& node) const;

      /** \brief add indices of those DOFs of a given node that are positions
       *
       *  \author grill
       *  \date 07/16 */
      virtual void PositionDofIndices(std::vector<int>& posdofs, const DRT::Node& node) const = 0;

      /** \brief add indices of those DOFs of a given node that are tangents (in the case of Hermite
       * interpolation)
       *
       *  \author grill
       *  \date 07/16 */
      virtual void TangentDofIndices(std::vector<int>& tangdofs, const DRT::Node& node) const = 0;

      /** \brief add indices of those DOFs of a given node that are rotation DOFs (non-additive
       * rotation vectors)
       *
       *  \author grill
       *  \date 07/16 */
      virtual void RotationVecDofIndices(
          std::vector<int>& rotvecdofs, const DRT::Node& node) const = 0;

      /** \brief add indices of those DOFs of a given node that are 1D rotation DOFs
       *         (planar rotations are additive, e.g. in case of relative twist DOF of beam3k with
       * rotvec=false)
       *
       *  \author grill
       *  \date 07/16 */
      virtual void Rotation1DDofIndices(
          std::vector<int>& twistdofs, const DRT::Node& node) const = 0;

      /** \brief add indices of those DOFs of a given node that represent norm of tangent vector
       *         (additive, e.g. in case of beam3k with rotvec=true)
       *
       *  \author grill
       *  \date 07/16 */
      virtual void TangentLengthDofIndices(
          std::vector<int>& tangnormdofs, const DRT::Node& node) const = 0;

      /** \brief get element local indices of those Dofs that are used for centerline interpolation
       *
       *  \author grill
       *  \date 12/16 */
      virtual void CenterlineDofIndicesOfElement(
          std::vector<unsigned int>& centerlinedofindices) const = 0;

      /** \brief get Jacobi factor ds/dxi(xi) at xi \in [-1;1]
       *
       *  \author grill
       *  \date 06/16 */
      virtual double GetJacobiFacAtXi(const double& xi) const = 0;

      /** \brief Get material cross-section deformation measures, i.e. strain resultants
       *
       *  \author grill
       *  \date 04/17 */
      virtual inline void GetMaterialStrainResultantsAtAllGPs(std::vector<double>& axial_strain_GPs,
          std::vector<double>& shear_strain_2_GPs, std::vector<double>& shear_strain_3_GPs,
          std::vector<double>& twist_GPs, std::vector<double>& curvature_2_GPs,
          std::vector<double>& curvature_3_GPs) const
      {
        FOUR_C_THROW("not implemented");
      }

      /** \brief Get spatial cross-section stress resultants
       *
       *  \author eichinger
       *  \date 05/17 */
      virtual inline void GetSpatialStressResultantsAtAllGPs(
          std::vector<double>& spatial_axial_force_GPs,
          std::vector<double>& spatial_shear_force_2_GPs,
          std::vector<double>& spatial_shear_force_3_GPs, std::vector<double>& spatial_torque_GPs,
          std::vector<double>& spatial_bending_moment_2_GPs,
          std::vector<double>& spatial_bending_moment_3_GPs) const
      {
        FOUR_C_THROW("not implemented");
      }

      /** \brief Get spatial cross-section stress resultants
       *
       *  \author eichinger
       *  \date 05/17 */
      virtual inline void GetSpatialForcesAtAllGPs(std::vector<double>& spatial_axial_force_GPs,
          std::vector<double>& spatial_shear_force_2_GPs,
          std::vector<double>& spatial_shear_force_3_GPs) const
      {
        FOUR_C_THROW("not implemented");
      }

      /** \brief Get spatial cross-section stress resultants
       *
       *  \author eichinger
       *  \date 05/17 */
      virtual inline void GetSpatialMomentsAtAllGPs(std::vector<double>& spatial_torque_GPs,
          std::vector<double>& spatial_bending_moment_2_GPs,
          std::vector<double>& spatial_bending_moment_3_GPs) const
      {
        FOUR_C_THROW("not implemented");
      }

      /** \brief Get material cross-section stress resultants
       *
       *  \author grill
       *  \date 04/17 */
      virtual inline void GetMaterialStressResultantsAtAllGPs(
          std::vector<double>& material_axial_force_GPs,
          std::vector<double>& material_shear_force_2_GPs,
          std::vector<double>& material_shear_force_3_GPs, std::vector<double>& material_torque_GPs,
          std::vector<double>& material_bending_moment_2_GPs,
          std::vector<double>& material_bending_moment_3_GPs) const
      {
        FOUR_C_THROW("not implemented");
      }

      /** \brief Get number of degrees of freedom of a single node
       *
       *  \author eichinger
       *  \date 08/16 */
      int NumDofPerNode(const DRT::Node& node) const override
      {
        FOUR_C_THROW("not implemented");
        return -1;
      }

      /** \brief get centerline position at xi \in [-1,1] (element parameter space) in stress-free
       * reference configuration
       *
       *  \author grill
       *  \date 06/16 */
      void GetRefPosAtXi(CORE::LINALG::Matrix<3, 1>& refpos, const double& xi) const;

      /** \brief get unit tangent vector in reference configuration at i-th node of beam element
       * (element-internal numbering)
       *
       *  \author grill
       *  \date 06/16 */
      virtual void GetRefTangentAtNode(CORE::LINALG::Matrix<3, 1>& Tref_i, const int& i) const = 0;

      /** \brief get centerline position at xi \in [-1,1] (element parameter space) from
       * displacement state vector
       *
       *  \author grill
       *  \date 06/16 */
      virtual void GetPosAtXi(CORE::LINALG::Matrix<3, 1>& pos, const double& xi,
          const std::vector<double>& disp) const = 0;

      /** \brief get triad at xi \in [-1,1] (element parameter space)
       *
       *  \author grill
       *  \date 07/16 */
      virtual void GetTriadAtXi(CORE::LINALG::Matrix<3, 3>& triad, const double& xi,
          const std::vector<double>& disp) const
      {
        // ToDo make pure virtual and add/generalize implementations in beam eles
        FOUR_C_THROW("not implemented");
      }

      /** \brief get generalized interpolation matrix which yields the variation of the position and
       *         orientation at xi \in [-1,1] if multiplied with the vector of primary DoF
       * variations
       *
       *  \author grill
       *  \date 11/16 */
      virtual void GetGeneralizedInterpolationMatrixVariationsAtXi(
          CORE::LINALG::SerialDenseMatrix& Ivar, const double& xi,
          const std::vector<double>& disp) const
      {
        FOUR_C_THROW("not implemented");
      }

      /** \brief get linearization of the product of (generalized interpolation matrix for
       * variations (see above) and applied force vector) with respect to the primary DoFs of this
       * element
       *
       *  \author grill
       *  \date 01/17 */
      virtual void GetStiffmatResultingFromGeneralizedInterpolationMatrixAtXi(
          CORE::LINALG::SerialDenseMatrix& stiffmat, const double& xi,
          const std::vector<double>& disp, const CORE::LINALG::SerialDenseVector& force) const
      {
        FOUR_C_THROW("not implemented");
      }

      /** \brief get generalized interpolation matrix which yields the increments of the position
       * and orientation at xi \in [-1,1] if multiplied with the vector of primary DoF increments
       *
       *  \author grill
       *  \date 11/16 */
      virtual void GetGeneralizedInterpolationMatrixIncrementsAtXi(
          CORE::LINALG::SerialDenseMatrix& Iinc, const double& xi,
          const std::vector<double>& disp) const
      {
        FOUR_C_THROW("not implemented");
      }

      //! get internal (elastic) energy of element
      virtual double GetInternalEnergy() const = 0;

      //! get kinetic energy of element
      virtual double GetKineticEnergy() const = 0;

      //! shifts nodes so that proper evaluation is possible even in case of periodic boundary
      //! conditions
      virtual void UnShiftNodePosition(std::vector<double>& disp,  //!< element disp vector
          CORE::GEO::MESHFREE::BoundingBox const& periodic_boundingbox) const;

      //! get directions in which element might be cut by a periodic boundary
      virtual void GetDirectionsOfShifts(std::vector<double>& disp,
          CORE::GEO::MESHFREE::BoundingBox const& periodic_boundingbox,
          std::vector<bool>& shift_in_dim) const;

      /** \brief extract values for those Dofs relevant for centerline-interpolation from total
       * state vector
       *
       *  \author grill
       *  \date 11/16 */
      virtual void ExtractCenterlineDofValuesFromElementStateVector(
          const std::vector<double>& dofvec, std::vector<double>& dofvec_centerline,
          bool add_reference_values = false) const = 0;

      /** \brief return flag whether Hermite polynomials are applied for centerline interpolation
       */
      inline bool HermiteCenterlineInterpolation() const { return centerline_hermite_; }

     protected:
      //! vector holding reference tangent at the centerline nodes
      std::vector<CORE::LINALG::Matrix<3, 1>> Tref_;

      //! bool storing whether Hermite interpolation of centerline is applied (false: Lagrange
      //! interpolation)
      bool centerline_hermite_;

      /** \brief get access to the interface
       *
       *  \author hiermeier
       *  \date 04/16 */
      inline STR::ELEMENTS::ParamsInterface& ParamsInterface() const
      {
        if (not IsParamsInterface()) FOUR_C_THROW("The interface ptr is not set!");
        return *interface_ptr_;
      }

      inline BROWNIANDYN::ParamsInterface& BrownianDynParamsInterface() const
      {
        return *browndyn_interface_ptr_;
      }

      /** \brief add reference positions and tangents to (centerline) displacement state vector
       *
       * @tparam nnode number of nodes
       * @tparam vpernode values per nodal direction
       *
       * @param pos_ref_centerline Vector containing the centerline reference position values
       */
      template <unsigned int nnodecl, unsigned int vpernode, typename T>
      void AddRefValuesDispCenterline(
          CORE::LINALG::Matrix<3 * vpernode * nnodecl, 1, T>& pos_ref_centerline) const
      {
        for (unsigned int dim = 0; dim < 3; ++dim)
          for (unsigned int node = 0; node < nnodecl; ++node)
          {
            pos_ref_centerline(3 * vpernode * node + dim) += Nodes()[node]->X()[dim];
            if (HermiteCenterlineInterpolation())
              pos_ref_centerline(3 * vpernode * node + 3 + dim) += Tref_[node](dim);
          }
      }

      /** \brief calculates the element length in reference configuration
       *
       * @tparam nnode number of nodes
       * @tparam vpernode values per nodal direction
       *
       * For Lagrange centerline interpolation the difference between both boundary points is used
       * as reference length. In the case of Hermite centerline interpolation the value is used as
       * start for a newton iteration.
       *
       * see "Meier, C.: Geometrically exact finite element formulations for slender beams and
       * their contact interaction, Technical University of Munich, Germany, 2016", chapter 3.2.2.2
       */
      template <unsigned int nnode, unsigned int vpernode>
      double CalcReflength(
          const CORE::LINALG::Matrix<3 * vpernode * nnode, 1, double>& disp_refe_centerline)
      {
        CORE::LINALG::Matrix<3, 1> tempvec(true);

        for (int dim = 0; dim < 3; dim++)
          tempvec(dim) = disp_refe_centerline(3 * vpernode * 1 + dim) - disp_refe_centerline(dim);
        double reflength = tempvec.Norm2();

        if (HermiteCenterlineInterpolation())
        {
          const auto gausspoints =
              CORE::FE::IntegrationPoints1D(CORE::FE::GaussRule1D::line_10point);
          const auto distype = this->Shape();

          auto lengthEquationAndDerivative = [&gausspoints, &disp_refe_centerline, &distype](
                                                 double reflength)
          {
            return DRT::UTILS::BEAM::IntegrateCenterlineArcLengthAndArcLengthDerivative<nnode,
                vpernode>(gausspoints, disp_refe_centerline, distype, reflength);
          };

          const double newton_tolerance = 1e-12;
          const int max_iterations = 200;
          reflength = CORE::UTILS::SolveLocalNewton(
              lengthEquationAndDerivative, reflength, newton_tolerance, max_iterations);
        }

        return reflength;
      }

      /*! \brief Get centerline position at given parameter coordinate xi
       *
       * @tparam nnode number of nodes
       * @tparam vpernode values per nodal direction
       *
       * The parameter disp_totlag has to contain the absolute (total Lagrange) values of the
       * centerline degrees of freedom, i.e., the reference values + the current displacement
       * values (\ref UpdateDispTotlag or \ref AddRefValuesDispCenterline).
       */
      template <unsigned int nnode, unsigned int vpernode, typename T>
      void GetPosAtXi(CORE::LINALG::Matrix<3, 1, T>& r, const double& xi,
          const CORE::LINALG::Matrix<3 * nnode * vpernode, 1, T>& disp_totlag) const
      {
        CORE::LINALG::Matrix<1, vpernode * nnode, T> N_i;

        DRT::UTILS::BEAM::EvaluateShapeFunctionsAtXi<nnode, vpernode>(
            xi, N_i, Shape(), RefLength());
        Calc_r<nnode, vpernode, T>(disp_totlag, N_i, r);
      }

      /** \brief compute beam centerline position vector at position \xi in element parameter space
       * [-1,1] via interpolation of nodal DoFs based on given shape function values
       *
       *  \author grill
       *  \date 03/16 */
      template <unsigned int nnode, unsigned int vpernode, typename T>
      void Calc_r(const CORE::LINALG::Matrix<3 * vpernode * nnode, 1, T>& disp_totlag_centerline,
          const CORE::LINALG::Matrix<1, vpernode * nnode, double>& funct,
          CORE::LINALG::Matrix<3, 1, T>& r) const
      {
        DRT::UTILS::BEAM::CalcInterpolation<nnode, vpernode, 3, T>(
            disp_totlag_centerline, funct, r);
      }

      /** \brief compute derivative of beam centerline (i.e. tangent vector) at position \xi in
       *         element parameter space [-1,1] with respect to \xi via interpolation of nodal DoFs
       *         based on given shape function derivative values
       *
       *  \author grill
       *  \date 03/16 */
      template <unsigned int nnode, unsigned int vpernode, typename T>
      void Calc_r_xi(const CORE::LINALG::Matrix<3 * vpernode * nnode, 1, T>& disp_totlag_centerline,
          const CORE::LINALG::Matrix<1, vpernode * nnode, double>& deriv,
          CORE::LINALG::Matrix<3, 1, T>& r_xi) const
      {
        DRT::UTILS::BEAM::CalcInterpolation<nnode, vpernode, 3, T>(
            disp_totlag_centerline, deriv, r_xi);
      }

      /** \brief compute derivative of beam centerline (i.e. tangent vector) at position \xi in
       *         element parameter space [-1,1] with respect to arc-length parameter s in reference
       *         configuration via interpolation of nodal DoFs based on given shape function
       * derivative values
       *
       *  \author grill
       *  \date 03/16 */
      template <unsigned int nnode, unsigned int vpernode, typename T>
      void Calc_r_s(const CORE::LINALG::Matrix<3 * vpernode * nnode, 1, T>& disp_totlag_centerline,
          const CORE::LINALG::Matrix<1, vpernode * nnode, double>& deriv, const double& jacobi,
          CORE::LINALG::Matrix<3, 1, T>& r_s) const
      {
        Calc_r_xi<nnode, vpernode, T>(disp_totlag_centerline, deriv, r_s);

        /* at this point we have computed derivative with respect to the element parameter \xi \in
         * [-1;1]; as we want derivative with respect to the reference arc-length parameter s, we
         * have to divide it by the Jacobi determinant at the respective point*/
        r_s.Scale(1.0 / jacobi);
      }

      /** \brief get applied beam material law object
       */
      MAT::BeamMaterial& GetBeamMaterial() const;


      /** \brief get elasto(plastic) beam material law object
       *
       */
      template <typename T>
      MAT::BeamMaterialTemplated<T>& GetTemplatedBeamMaterial() const;

      /** \brief setup constitutive matrices from material law
       *
       *  \author grill
       *  \date 03/16 */
      template <typename T>
      void GetConstitutiveMatrices(
          CORE::LINALG::Matrix<3, 3, T>& CN, CORE::LINALG::Matrix<3, 3, T>& CM) const;

      /** \brief setup mass inertia tensors from material law
       *
       *  \author grill
       *  \date 03/16 */
      template <typename T>
      void GetTranslationalAndRotationalMassInertiaTensor(
          double& mass_inertia_translational, CORE::LINALG::Matrix<3, 3, T>& J) const;

      /** \brief setup only translational mass inertia factor from material law
       *      this method is called by reduced beam formulation which don't inlcude
       *      rotational mass inertia
       *
       *  \author grill
       *  \date 03/17 */
      void GetTranslationalMassInertiaFactor(double& mass_inertia_translational) const;

      //! @name Methods and variables for Brownian dynamics or beaminteraction simulations
      //! @{
      //! computes damping coefficients
      void GetDampingCoefficients(CORE::LINALG::Matrix<3, 1>& gamma) const;

      //! computes velocity of background fluid and gradient of that velocity at a certain
      //! evaluation point in the physical space and adds respective terms to internal forces and
      //! damping matrix
      template <unsigned int ndim, typename T>  // number of dimensions of embedding space
      void GetBackgroundVelocity(Teuchos::ParameterList& params,  //!< parameter list
          const CORE::LINALG::Matrix<ndim, 1, T>&
              evaluationpoint,  //!< point at which background velocity and its gradient has to be
                                //!< computed
          CORE::LINALG::Matrix<ndim, 1, T>& velbackground,  //!< velocity of background fluid
          CORE::LINALG::Matrix<ndim, ndim, T>& velbackgroundgrad)
          const;  //!< gradient of velocity of background fluid

     public:
      //! get centerline pos at binding spot with locn x stored in element parameter space
      //! coordinates \in [-1,1] from displacement state vector
      void GetPosOfBindingSpot(CORE::LINALG::Matrix<3, 1>& pos, std::vector<double>& disp,
          INPAR::BEAMINTERACTION::CrosslinkerType linkertype, int bspotlocn,
          CORE::GEO::MESHFREE::BoundingBox const& periodic_boundingbox) const;

      //! get triad at binding spot with locn x stored in element parameter space coordinates \in
      //! [-1,1] from displacement state vector
      void GetTriadOfBindingSpot(CORE::LINALG::Matrix<3, 3>& triad, std::vector<double>& disp,
          INPAR::BEAMINTERACTION::CrosslinkerType linkertype, int bspotlocn) const;

      /** \brief get entire binding spot information of element
       *
       *  \author eichinger
       *  \date 06/17 */
      std::map<INPAR::BEAMINTERACTION::CrosslinkerType, std::vector<double>> const&
      GetBindingSpots() const
      {
        return bspotposxi_;
      }

      /** \brief get number of binding spot types on this element
       *
       *  \author eichinger
       *  \date 06/17 */
      unsigned int GetNumberOfBindingSpotTypes() const { return bspotposxi_.size(); }

      /** \brief get number of binding spots of certain binding spot type on this element
       *
       *  \author eichinger
       *  \date 06/17 */
      unsigned int GetNumberOfBindingSpots(INPAR::BEAMINTERACTION::CrosslinkerType linkertype) const
      {
        return bspotposxi_.at(linkertype).size();
      }

      /** \brief get binding spot positions xi
       *
       *  \author eichinger
       *  \date 03/17 */
      double GetBindingSpotXi(
          INPAR::BEAMINTERACTION::CrosslinkerType linkertype, unsigned int bspotlocn) const
      {
        if (bspotlocn > bspotposxi_.at(linkertype).size())
          FOUR_C_THROW("number of requested binding spot exceeds total number of binding spots");

        return bspotposxi_.at(linkertype)[bspotlocn];
      }

      /** \brief set binding spot positions and status in crosslinker simulation
       *
       *  \author eichinger
       *  \date 03/17 */
      void SetBindingSpots(
          std::map<INPAR::BEAMINTERACTION::CrosslinkerType, std::vector<double>> bspotposxi)
      {
        bspotposxi_.clear();
        bspotposxi_ = bspotposxi;
      }

      /** \brief set binding spot positions and status in crosslinker simulation
       *
       *  \author eichinger
       *  \date 03/17 */
      void SetPositionsOfBindingSpotType(
          INPAR::BEAMINTERACTION::CrosslinkerType linkertype, std::vector<double> const& bspotposxi)
      {
        bspotposxi_[linkertype] = bspotposxi;
      }

      /** \brief set/get type of filament the element is part of
       *
       *  \author eichinger
       *  \date 03/17 */
      void SetFilamentType(INPAR::BEAMINTERACTION::FilamentType filamenttype)
      {
        filamenttype_ = filamenttype;
      }

      INPAR::BEAMINTERACTION::FilamentType GetFilamentType() const { return filamenttype_; }

      /**
       * \brief Get the bounding volume of the element for geometric search
       *
       * @param discret Discretization of the respective field
       * @param result_data_dofbased Result data vector used for extracting positions
       * @return bounding volume of the respective element
       */
      CORE::GEOMETRICSEARCH::BoundingVolume GetBoundingVolume(const DRT::Discretization& discret,
          const Epetra_Vector& result_data_dofbased,
          const CORE::GEOMETRICSEARCH::GeometricSearchParams& params) const override;

     private:
      //! position of binding spots on beam element in local coordinate system
      //! size of vector equals number of binding spots on this element
      std::map<INPAR::BEAMINTERACTION::CrosslinkerType, std::vector<double>> bspotposxi_;

      //! type of filament element belongs to
      INPAR::BEAMINTERACTION::FilamentType filamenttype_;

      //! @}

      /** \brief interface ptr
       *
       *  data exchange between the element and the time integrator. */
      Teuchos::RCP<STR::ELEMENTS::ParamsInterface> interface_ptr_;

      Teuchos::RCP<BROWNIANDYN::ParamsInterface> browndyn_interface_ptr_;

      /*!
      \brief Default Constructor must not be called
      */
      Beam3Base();
    };

  }  // namespace ELEMENTS
}  // namespace DRT

FOUR_C_NAMESPACE_CLOSE

#endif