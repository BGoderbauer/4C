/*----------------------------------------------------------------------------*/
/*! \file
\brief This file contains routines for constraint mixture growth and remodeling.
example input line
MAT 1 MAT_ConstraintMixture DENS 0.001 MUE 1.0 NUE 0.49 PHIE 0.08
PREELA 1.0 K1 1.0 K2 1.0 NUMHOM 1 PRECOLL 1.06 DAMAGE 1.5 K1M 1.0 K2M 1.0
PHIM 1.0 PREMUS 1.0 SMAX 0.0 KAPPA 1.0E6 LIFETIME 5.0 GROWTHFAC 0.5
HOMSTR 6.75E4 SHEARGROWTHFAC 0.0 HOMRAD 1.0 STARTTIME 5.0
INTEGRATION Explicit TOL 1.0E-4 GROWTHFORCE Single ELASTINDEGRAD None
MASSPROD Lin INITSTRETCH None CURVE 0 DEGOPTION Cos MAXMASSPRODFAC 4.0
STOREHISTORY No


Here an approach for growth and remodeling of an artery is modeled.
For a detailed description see:
- Humphrey, J. & Rajagopal, K.: A constrained mixture model for arterial
  adaptations to a sustained step change in blood flow,
  Biomechanics and Modeling in Mechanobiology, 2003, 2, 109-126

\level 2

*/
/*----------------------------------------------------------------------------*/

#ifndef FOUR_C_MAT_CONSTRAINTMIXTURE_HPP
#define FOUR_C_MAT_CONSTRAINTMIXTURE_HPP


#include "4C_config.hpp"

#include "4C_comm_parobjectfactory.hpp"
#include "4C_mat_so3_material.hpp"
#include "4C_material_parameter_base.hpp"

FOUR_C_NAMESPACE_OPEN

namespace MAT
{
  namespace PAR
  {
    /*----------------------------------------------------------------------*/
    /// material parameters
    class ConstraintMixture : public CORE::MAT::PAR::Parameter
    {
     public:
      /// standard constructor
      ConstraintMixture(Teuchos::RCP<CORE::MAT::PAR::Material> matdata);

      /// @name material parameters
      //@{
      /// density
      const double density_;
      /// shear modulus
      const double mue_;
      /// Poisson's ratio
      const double nue_;
      /// mass fraction of elastin
      const double phielastin_;
      /// prestretch of elastin
      const double prestretchelastin_;
      /// parameter for linear fiber stiffness of collagen
      const double k1_;
      /// parameter for exponential fiber stiffness of collagen
      const double k2_;
      /// number of homeostatic variables
      const int numhom_;
      /// prestretch of collagen fibers
      const std::vector<double> prestretchcollagen_;
      /// stretch at which collagen fibers are damaged
      const double damagestretch_;
      /// parameter for linear fiber stiffness of smooth muscle
      const double k1muscle_;
      /// parameter for exponential fiber stiffness of smooth muscle
      const double k2muscle_;
      /// mass fraction of smooth muscle
      const double phimuscle_;
      /// prestretch of smooth muscle fibers
      const double prestretchmuscle_;
      /// maximal active stress
      const double Smax_;
      /// dilatation modulus
      const double kappa_;
      /// lifetime of collagen fibers
      const double lifetime_;
      /// growth factor for stress
      // const double growthfactor_;
      /// homeostatic target value of scalar stress measure
      const std::vector<double> homstress_;
      /// growth factor for shear
      const double sheargrowthfactor_;
      /// homeostatic target value of inner radius
      const double homradius_;
      /// at this time turnover of collagen starts
      const double starttime_;
      /// time integration scheme (Explicit,Implicit)
      const std::string integration_;
      /// tolerance for local Newton iteration
      const double abstol_;
      /// driving force of growth (Single,All,ElaCol)
      const std::string growthforce_;
      /// form of elastin  degradation
      const std::string elastindegrad_;
      /// how mass depends on driving force
      const std::string massprodfunc_;
      /// how to set stretches in the beginning
      const std::string initstretch_;
      /// number of timecurve for increase of prestretch in time
      const int timecurve_;
      /// which degradation function
      const std::string degoption_;
      /// maximal factor of mass production
      const double maxmassprodfac_;
      /// store all history variables
      const bool storehistory_;
      /// tolerance for degradation
      const double degtol_;
      //@}

      /// create material instance of matching type with my parameters
      Teuchos::RCP<CORE::MAT::Material> CreateMaterial() override;

      // !brief enum for mapping between material parameter and entry in the matparams_ vector
      enum Matparamnames
      {
        growthfactor,
        elastin_survival,
        first = growthfactor,
        last = elastin_survival
      };

    };  // class ConstraintMixture

  }  // namespace PAR

  class ConstraintMixtureHistory;

  class ConstraintMixtureType : public CORE::COMM::ParObjectType
  {
   public:
    std::string Name() const override { return "ConstraintMixtureType"; }

    static ConstraintMixtureType& Instance() { return instance_; };

    CORE::COMM::ParObject* Create(const std::vector<char>& data) override;

   private:
    static ConstraintMixtureType instance_;
  };

  /*----------------------------------------------------------------------*/
  /// Wrapper for constraint mixture material
  class ConstraintMixture : public So3Material
  {
   public:
    /// construct empty material object
    ConstraintMixture();

    /// construct the material object given material parameters
    explicit ConstraintMixture(MAT::PAR::ConstraintMixture* params);

    //! @name Packing and Unpacking

    /*!
      \brief Return unique ParObject id

      every class implementing ParObject needs a unique id defined at the
      top of parobject.H (this file) and should return it in this method.
    */
    int UniqueParObjectId() const override
    {
      return ConstraintMixtureType::Instance().UniqueParObjectId();
    }

    /*!
      \brief Pack this class so it can be communicated

      Resizes the vector data and stores all information of a class in it.
      The first information to be stored in data has to be the
      unique parobject id delivered by UniqueParObjectId() which will then
      identify the exact class on the receiving processor.
      This material contains history variables, which are packed for restart purposes.

      \param data (in/out): char vector to store class information
    */
    void Pack(CORE::COMM::PackBuffer& data) const override;

    /*!
      \brief Unpack data from a char vector into this class

      The vector data contains all information to rebuild the
      exact copy of an instance of a class on a different processor.
      The first entry in data has to be an integer which is the unique
      parobject id defined at the top of this file and delivered by
      UniqueParObjectId().
      History data is unpacked in restart.

      \param data (in) : vector storing all data to be unpacked into this
      instance.
    */
    void Unpack(const std::vector<char>& data) override;

    //@}

    /// material type
    CORE::Materials::MaterialType MaterialType() const override
    {
      return CORE::Materials::m_constraintmixture;
    }

    /// check if element kinematics and material kinematics are compatible
    void ValidKinematics(INPAR::STR::KinemType kinem) override
    {
      if (!(kinem == INPAR::STR::KinemType::nonlinearTotLag))
        FOUR_C_THROW("element and material kinematics are not compatible");
    }

    /// return copy of this material object
    Teuchos::RCP<CORE::MAT::Material> Clone() const override
    {
      return Teuchos::rcp(new ConstraintMixture(*this));
    }

    /// Setup
    void Setup(int numgp,               ///< number of Gauss points
        INPUT::LineDefinition* linedef  ///< definition of element line
        ) override;

    /// SetupHistory
    void ResetAll(const int numgp);

    /// Update
    void Update() override;

    /// Reset time step
    void ResetStep() override;

    /// Evaluate material
    void Evaluate(const CORE::LINALG::Matrix<3, 3>* defgrd,
        const CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* glstrain, Teuchos::ParameterList& params,
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* stress,
        CORE::LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>* cmat, const int gp,
        const int eleGID) override;

    /// Return density
    double Density() const override { return params_->density_; }

    /// Return quick accessible material parameter data
    CORE::MAT::PAR::Parameter* Parameter() const override { return params_; }

    /// Return variables for visualization
    CORE::LINALG::Matrix<3, 1> GetVis(int gp) const { return vismassstress_->at(gp); }
    /// Return actual mass density in reference configuration
    double GetMassDensity(int gp) const { return refmassdens_->at(gp); }
    /// Return actual mass density in reference configuration
    CORE::LINALG::Matrix<3, 1> GetMassDensityCollagen(int gp) const
    {
      return visrefmassdens_->at(gp);
    }
    /// Return prestretch of collagen fibers
    CORE::LINALG::Matrix<3, 1> GetPrestretch(int gp) const
    {
      CORE::LINALG::Matrix<3, 1> visprestretch(true);
      visprestretch(0) = localprestretch_->at(gp)(0);
      visprestretch(1) = localprestretch_->at(gp)(1);
      visprestretch(2) = localprestretch_->at(gp)(2);
      return visprestretch;
    }
    /// Return prestretch of collagen fibers
    CORE::LINALG::Matrix<3, 1> GetHomstress(int gp) const
    {
      CORE::LINALG::Matrix<3, 1> visprestretch(true);
      visprestretch(0) = localhomstress_->at(gp)(0);
      visprestretch(1) = localhomstress_->at(gp)(1);
      visprestretch(2) = localhomstress_->at(gp)(2);
      return visprestretch;
    }
    /// Return circumferential fiber direction
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<3, 1>>> Geta1() const { return a1_; }
    /// Return axial fiber direction
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<3, 1>>> Geta2() const { return a2_; }

    /// evaluate fiber directions from locsys, pull back
    void EvaluateFiberVecs(const int gp, const CORE::LINALG::Matrix<3, 3>& locsys,
        const CORE::LINALG::Matrix<3, 3>& defgrd);

    /// Return names of visualization data
    void VisNames(std::map<std::string, int>& names) override;

    /// Return visualization data
    bool VisData(const std::string& name, std::vector<double>& data, int numgp, int eleID) override;

   private:
    /// my material parameters
    MAT::PAR::ConstraintMixture* params_;

    /// temporary for visualization
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<3, 1>>> vismassstress_;
    /// actual mass density in reference configuration
    Teuchos::RCP<std::vector<double>> refmassdens_;
    /// actual mass density in reference configuration for collagen fibers
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<3, 1>>> visrefmassdens_;
    /// basal rate of mass production
    double massprodbasal_;

    /// first fiber vector per gp (reference), circumferential
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<3, 1>>> a1_;
    /// second fiber vector per gp (reference), axial
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<3, 1>>> a2_;
    /// third fiber vector per gp (reference), diagonal
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<3, 1>>> a3_;
    /// fourth fiber vector per gp (reference), diagonal
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<3, 1>>> a4_;
    /// homeostatic prestretch of collagen fibers
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<4, 1>>> localprestretch_;
    /// homeostatic stress for growth
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<4, 1>>> localhomstress_;
    /// homeostatic radius
    double homradius_;
    /// list of fibers which have been overstretched and are deleted at the end of the time step
    /// (gp, idpast, idfiber)
    Teuchos::RCP<std::vector<CORE::LINALG::Matrix<3, 1>>> deletemass_;
    /// index of oldest fiber still alive, needed if the complete history is stored to avoid
    /// summation of zeros
    int minindex_;
    /// indicates if material is initialized
    bool isinit_;

    /// history
    Teuchos::RCP<std::vector<ConstraintMixtureHistory>> history_;

    /// compute stress and cmat
    void EvaluateStress(
        const CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* glstrain,    ///< green lagrange strain
        const int gp,                                              ///< current Gauss point
        CORE::LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>* cmat,  ///< material stiffness matrix
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* stress,            ///< 2nd PK-stress
        const int firstiter,  ///< iteration index, different for explicit and implicit integration
        double time,          ///< time
        double elastin_survival  ///< amount of elastin which is still there
    );

    /// elastic response for one collagen fiber family
    void EvaluateFiberFamily(const CORE::LINALG::Matrix<NUM_STRESS_3D, 1> C,  ///< Cauchy-Green
        const int gp,                                              ///< current Gauss point
        CORE::LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>* cmat,  ///< material stiffness matrix
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* stress,            ///< 2nd PK-stress
        CORE::LINALG::Matrix<3, 1> a,                              ///< fiber vector
        double* currmassdens,                                      ///< current massdensity
        const int firstiter,  ///< iteration index, different for explicit and implicit integration
        double time,          ///< time
        const int idfiber     ///< number of fiber family 0,1,2,3
    );

    /// elastic response for one collagen fiber
    void EvaluateSingleFiberScalars(
        double
            I4,  ///< fourth invariant multiplied with (prestretch / stretch at deposition time)^2
        double& fac_cmat,   ///< scalar factor for material stiffness matrix
        double& fac_stress  ///< scalar factor for 2nd PK-stress
    );

    /// elastic response for Elastin
    void EvaluateElastin(const CORE::LINALG::Matrix<NUM_STRESS_3D, 1> C,  ///< Cauchy-Green
        CORE::LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>* cmat,  ///< material stiffness matrix
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* stress,            ///< 2nd PK-stress
        double time,                                               ///< time
        double* currmassdens,                                      ///< current massdensity
        double elastin_survival  ///< amount of elastin which is still there
    );

    /// elastic response for smooth muscle cells
    void EvaluateMuscle(const CORE::LINALG::Matrix<NUM_STRESS_3D, 1> C,  ///< Cauchy-Green
        CORE::LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>* cmat,  ///< material stiffness matrix
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* stress,            ///< 2nd PK-stress
        const int gp,                                              ///< current Gauss point
        double* currmassdens                                       ///< current massdensity
    );

    /// volumetric part
    void EvaluateVolumetric(const CORE::LINALG::Matrix<NUM_STRESS_3D, 1> C,  ///< Cauchy-Green
        CORE::LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>* cmat,  ///< material stiffness matrix
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* stress,            ///< 2nd PK-stress
        double currMassDens,                                       ///< current mass density
        double refMassDens                                         ///< initial mass density
    );

    /// computes mass production rates for all fiber families
    void MassProduction(const int gp,              ///< current Gauss point
        CORE::LINALG::Matrix<3, 3> defgrd,         ///< deformation gradient
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1> S,  ///< 2nd PK-stress
        CORE::LINALG::Matrix<4, 1>* massstress,    ///< growth stress measure
        double inner_radius,                       ///< inner radius
        CORE::LINALG::Matrix<4, 1>* massprodcomp,  ///< mass production rate
        double growthfactor                        ///< growth factor for stress
    );

    /// computes mass production rate for one fiber family
    void MassProductionSingleFiber(const int gp,   ///< current Gauss point
        CORE::LINALG::Matrix<3, 3> defgrd,         ///< deformation gradient
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1> S,  ///< 2nd PK-stress
        double* massstress,                        ///< growth stress measure
        double inner_radius,                       ///< inner radius
        double* massprodcomp,                      ///< mass production rate
        CORE::LINALG::Matrix<3, 1> a,              ///< fiber vector
        const int idfiber,                         ///< number of fiber family 0,1,2,3
        double growthfactor                        ///< growth factor for stress
    );

    /// function for massproduction
    void MassFunction(double growthfac,  ///< growth factor K
        double delta,                    ///< difference in stress or wall shear stress
        double mmax,                     ///< maximal massproduction fac
        double& massfac                  ///< factor
    );

    /// returns how much collagen has been degraded
    void Degradation(double t, double& degr);

    /// function of elastin degradation (initial)
    void ElastinDegradation(
        CORE::LINALG::Matrix<3, 1> coord,  ///< gp coordinate in reference configuration
        double& elastin_survival           ///< amount of elastin which is still there
    );

    /// compute stress and cmat for implicit integration with whole stress as driving force
    void EvaluateImplicitAll(CORE::LINALG::Matrix<3, 3> defgrd,    ///< deformation gradient
        const CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* glstrain,    ///< green lagrange strain
        const int gp,                                              ///< current Gauss point
        CORE::LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>* cmat,  ///< material stiffness matrix
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* stress,            ///< 2nd PK-stress
        double dt,                                                 ///< delta time
        double time,                                               ///< time
        CORE::LINALG::Matrix<4, 1> massprodcomp,                   ///< mass production rate
        CORE::LINALG::Matrix<4, 1> massstress,                     ///< growth stress measure
        double elastin_survival,  ///< amount of elastin which is still there
        double growthfactor       ///< growth factor for stress
    );

    /// compute stress and cmat for implicit integration with fiber stress as driving force
    void EvaluateImplicitSingle(CORE::LINALG::Matrix<3, 3> defgrd,  ///< deformation gradient
        const CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* glstrain,     ///< green lagrange strain
        const int gp,                                               ///< current Gauss point
        CORE::LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>* cmat,   ///< material stiffness matrix
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* stress,             ///< 2nd PK-stress
        double dt,                                                  ///< delta time
        double time,                                                ///< time
        double elastin_survival,  ///< amount of elastin which is still there
        double growthfactor       ///< growth factor for stress
    );

    /// derivative of stress with respect to massproduction of a single fiber family
    void GradStressDMass(
        const CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* glstrain,  ///< green lagrange strain
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* derivative,      ///< result
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1> Cinv,             ///< inverse cauchy green strain
        CORE::LINALG::Matrix<3, 1> a,                            ///< fiber vector
        double stretch,  ///< prestretch / stretch at deposition time
        double J,        ///< determinant of F
        double dt,       ///< delta time
        bool option      ///< which driving force
    );

    /// derivative of massproduction of a single fiber family with respect to stress
    void GradMassDStress(CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* derivative,  ///< result
        CORE::LINALG::Matrix<3, 3> defgrd,   ///< deformation gradient
        CORE::LINALG::Matrix<3, 3> Smatrix,  ///< 2nd PK-stress in matrix notation
        CORE::LINALG::Matrix<3, 1> a,        ///< fiber vector
        double J,                            ///< determinant of F
        double massstress,                   ///< growth stress measure
        double homstress,                    ///< homeostatic stress
        double actcollstretch,               ///< actual collagen stretch
        double growthfactor                  ///< growth factor for stress
    );

    /// derivative of massproduction of a single fiber family with respect to stretch
    void GradMassDStretch(CORE::LINALG::Matrix<NUM_STRESS_3D, 1>* derivative,  ///< result
        CORE::LINALG::Matrix<3, 3> defgrd,   ///< deformation gradient
        CORE::LINALG::Matrix<3, 3> Smatrix,  ///< 2nd PK-stress in matrix notation
        CORE::LINALG::Matrix<NUM_STRESS_3D, 1> Cinv,
        CORE::LINALG::Matrix<3, 1> a,  ///< fiber vector
        double J,                      ///< determinant of F
        double massstress,             ///< growth stress measure
        double homstress,              ///< homeostatic stress
        double actcollstretch,         ///< actual collagen stretch
        double dt,                     ///< delta time
        double growthfactor            ///< growth factor for stress
    );
  };

  /// Debug output to gmsh-file
  /* this needs to be copied to STR::TimInt::OutputStep() to enable debug output
  {
    discret_->SetState("displacement",Dis());
    MAT::ConstraintMixtureOutputToGmsh(discret_, StepOld(), 1);
  }
  don't forget to include constraintmixture.H */
  void ConstraintMixtureOutputToGmsh(
      const Teuchos::RCP<DRT::Discretization> dis,  ///< discretization with displacements
      const int timestep,                           ///< index of timestep
      const int iter                                ///< iteration index of newton iteration
  );

}  // namespace MAT

FOUR_C_NAMESPACE_CLOSE

#endif