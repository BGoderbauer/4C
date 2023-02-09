/*-----------------------------------------------------------*/
/*! \file

\brief Implementation of utility methods for skeletal muscle


\level 3

*/
/*-----------------------------------------------------------*/

#include "mat_muscle_utils.H"
#include "lib_dserror.H"
#include "linalg_fixedsizematrix.H"
#include <cmath>
#include <functional>


double MAT::UTILS::MUSCLE::NewtonScalar(const std::function<ValuesFunctAndFunctDeriv(double)> &func,
    const double x_init, const double tol, const int maxiter)
{
  int numiter = 0;
  double x = x_init;
  ValuesFunctAndFunctDeriv funct_and_funct_deriv = func(x);

  // find x s.t. f(x) = 0
  while ((abs(funct_and_funct_deriv.val_funct) > tol) && (numiter <= maxiter))
  {
    numiter++;

    // x_{n+1} = x_{n} - func(x_{n}) / derivFunc(x_{n})
    x = x - funct_and_funct_deriv.val_funct / funct_and_funct_deriv.val_deriv_funct;

    // f(x_{n+1})
    funct_and_funct_deriv = func(x);
  }

  if (numiter >= maxiter and abs(funct_and_funct_deriv.val_funct) > tol)
    dserror(
        "Maximal number of iterations reached for Newton solver (%d iterations). Error is "
        "still at %14.14f",
        numiter, abs(funct_and_funct_deriv.val_funct));

  return x;
}

double MAT::UTILS::MUSCLE::Bisection(const std::function<ValuesFunctAndFunctDeriv(double)> &funct,
    const double a_init, const double b_init, const double tol, const int maxiter)
{
  double a = a_init;
  double b = b_init;
  double c;
  int numiter = 0;

  while (numiter <= maxiter)
  {
    numiter++;
    c = (a + b) / 2;

    auto f_c = funct(c).val_funct;
    auto f_a = funct(a).val_funct;

    if (abs(f_c) < tol)
    {
      return c;
    }
    // sgn(f(a)) == sgn(f(c))
    else if (f_c * f_a > 0)
    {
      a = c;
    }
    else
    {
      b = c;
    }
  }

  dserror(
      "Maximal number of iterations reached for Bisection method (%d iterations). Error is "
      "still at %14.14f",
      numiter, (b - a) / 2);

  return c;
}

MAT::UTILS::MUSCLE::ValuesFunctAndFunctDerivs
MAT::UTILS::MUSCLE::EvaluateFunctionAndDerivativesCentralDifferences(
    const std::function<double(double)> &func, const double x, const double delta_x)
{
  ValuesFunctAndFunctDerivs f_df_ddf;

  f_df_ddf.val_funct = func(x);

  double f_i_minus_1 = func(x - delta_x);
  double f_i_plus_1 = func(x + delta_x);
  f_df_ddf.val_deriv_funct = FirstDerivativeCentralDifferences(f_i_minus_1, f_i_plus_1, delta_x);
  f_df_ddf.val_deriv_deriv_funct =
      SecondDerivativeCentralDifferences(f_i_minus_1, f_df_ddf.val_funct, f_i_plus_1, delta_x);

  return f_df_ddf;
}

double MAT::UTILS::MUSCLE::FirstDerivativeCentralDifferences(
    const double f_i_minus_1, const double f_i_plus_1, const double delta_x)
{
  double dfdx = (f_i_plus_1 - f_i_minus_1) / (2 * delta_x);
  return dfdx;
}

double MAT::UTILS::MUSCLE::SecondDerivativeCentralDifferences(
    const double f_i_minus_1, const double f_i, const double f_i_plus_1, const double delta_x)
{
  double ddfddx = (f_i_plus_1 - 2 * f_i + f_i_minus_1) / (delta_x * delta_x);
  return ddfddx;
}

void MAT::UTILS::MUSCLE::EvaluateLambert(
    const double xi, double &W0, const double tol, const int maxiter)
{
  double W0_old =
      std::numeric_limits<double>::infinity();  // s.t. error is infinite in the beginning
  int numiter = 0;                              // number of iterations

  // Halley's method
  while ((std::abs(W0 - W0_old) / std::abs(W0) > tol) && (numiter <= maxiter))
  {
    numiter++;
    W0_old = W0;
    W0 = W0 - (W0 * std::exp(W0) - xi) /
                  ((std::exp(W0) * (W0 + 1) - (W0 + 2) * (W0 * std::exp(W0) - xi) / (2 * W0 + 2)));
  }

  // error handling
  if (numiter >= maxiter)
  {
    dserror(
        "Maximal number of iterations for evaluation of Lambert W function with Halley's method "
        "exceeded for tolerance %E.",
        tol);
  }
}

double MAT::UTILS::MUSCLE::EvaluateForceStretchDependencyEhret(
    const double lambdaM, const double lambdaMin, const double lambdaOpt)
{
  double fxi = 0.0;
  double explambda = std::exp(((2 * lambdaMin - lambdaM - lambdaOpt) * (lambdaM - lambdaOpt)) /
                              (2 * std::pow(lambdaMin - lambdaOpt, 2)));  // prefactor
  if (lambdaM > lambdaMin)
  {
    fxi = ((lambdaM - lambdaMin) / (lambdaOpt - lambdaMin)) * explambda;
  }

  return fxi;
}

double MAT::UTILS::MUSCLE::EvaluateDerivativeForceStretchDependencyEhret(
    const double lambdaM, const double lambdaMin, const double lambdaOpt)
{
  double dFxidLamdaM = 0.0;
  double explambda = std::exp(((2 * lambdaMin - lambdaM - lambdaOpt) * (lambdaM - lambdaOpt)) /
                              (2 * std::pow(lambdaMin - lambdaOpt, 2)));  // prefactor

  if (lambdaM > lambdaMin)
  {
    dFxidLamdaM = ((std::pow(lambdaMin - lambdaM, 2) - std::pow(lambdaMin - lambdaOpt, 2)) /
                      std::pow(lambdaMin - lambdaOpt, 3)) *
                  explambda;
  }

  return dFxidLamdaM;
}

double MAT::UTILS::MUSCLE::EvaluateIntegralForceStretchDependencyEhret(
    const double lambdaM, const double lambdaMin, const double lambdaOpt)
{
  double intFxi = 0.0;
  double explambda = std::exp(((2 * lambdaMin - lambdaM - lambdaOpt) * (lambdaM - lambdaOpt)) /
                              (2 * std::pow(lambdaMin - lambdaOpt, 2)));  // prefactor

  if (lambdaM > lambdaMin)
  {
    intFxi = (lambdaMin - lambdaOpt) * (explambda - std::exp(0.5));
  }

  return intFxi;
}

double MAT::UTILS::MUSCLE::EvaluateForceVelocityDependencyBoel(const double dotLambdaM,
    const double dotLambdaMMin, const double de, const double dc, const double ke, const double kc)
{
  // helper variable
  double ratioDotLambdaM = dotLambdaM / dotLambdaMMin;

  // velocity dependency fv
  double fv = 1.0;
  if (dotLambdaM > 0)
  {
    fv = (1 + de) - de * (1 + ratioDotLambdaM) / (1 - ke * kc * ratioDotLambdaM);
  }
  else
  {
    fv = (1 - dc) + dc * (1 - ratioDotLambdaM) / (1 + kc * ratioDotLambdaM);
  }

  return fv;
}

double MAT::UTILS::MUSCLE::EvaluateDerivativeForceVelocityDependencyBoel(const double dotLambdaM,
    const double dDotLambdaMdLambdaM, const double dotLambdaMMin, const double de, const double dc,
    const double ke, const double kc)
{
  // helper variable
  double ratioDotLambdaM = dotLambdaM / dotLambdaMMin;

  // derivative of fv w.r.t. dotLambdaM
  double dFvdDotLambdaM = 1.0;
  if (dotLambdaM > 0)
  {
    dFvdDotLambdaM =
        -de * ((1 + ke * kc) / (dotLambdaMMin * std::pow((1 - ke * kc * ratioDotLambdaM), 2)));
  }
  else
  {
    dFvdDotLambdaM = -dc * ((1 + kc) / (dotLambdaMMin * std::pow((1 + kc * ratioDotLambdaM), 2)));
  }

  // derivative of fv w.r.t. lambdaM
  double dFvdLambdaM = dFvdDotLambdaM * dDotLambdaMdLambdaM;

  return dFvdLambdaM;
}

double MAT::UTILS::MUSCLE::EvaluateTimeDependentActiveStressEhret(const double Na,
    const int muTypesNum, const std::vector<double> &rho, const std::vector<double> &I,
    const std::vector<double> &F, const std::vector<double> &T, const int actIntervalsNum,
    const std::vector<double> &actTimes, const std::vector<double> &actValues,
    const double currentTime)
{
  // compute twitch force of motor unit (MU) type iMU
  double t_iMU_jstim = 0;                // time of arriving stimulus jstim for MU type iMU
  double t_end = 0;                      // helper variable
  double ratiotime = 1.0;                // helper variable
  std::vector<double> sumg(muTypesNum);  // superposition of single twitches until current time
  std::vector<double> G(muTypesNum);     // gain function for MU type iMU
  std::vector<double> Ft(muTypesNum);    // twitch force for MU type iMU

  // for all motor unit types i
  for (int iMU = 0; iMU < muTypesNum; ++iMU)
  {
    // for all activation intervals
    for (int actinterval = 0; actinterval < actIntervalsNum; ++actinterval)
    {
      // set time of first stimulus jstim=1 to start time of the current activation interval
      t_iMU_jstim = actTimes[actinterval];

      // determine end time of activation interval
      // if inside of actinterval
      if (currentTime < actTimes[actinterval + 1])
      {
        t_end = currentTime;  // set end time to current simulation time
      }
      // if outside of actinterval
      else
      {
        t_end = actTimes[actinterval + 1];  // set end time to end time of actinterval
      }

      // superposition of single twitches
      // for all impulses from start of actinterval until determined end time of actinterval
      while (t_iMU_jstim < t_end)
      {
        ratiotime = (currentTime - t_iMU_jstim) / T[iMU];

        // add single twitch force response for stimulus jstim and motor unit iMU scaled by
        // percentage activation prescribed in actinterval
        sumg[iMU] += actValues[actinterval] * ratiotime * F[iMU] * std::exp(1 - ratiotime);

        // next impulse jstim at time t_iMU_jstim after stimulation interval I
        t_iMU_jstim += I[iMU];
      }
    }  // end actinterval

    // gain function for MU type iMU
    G[iMU] = (1 - std::exp(-2 * std::pow(T[iMU] / I[iMU], 3))) / (T[iMU] / I[iMU]);

    // twitch force for MU type iMU
    Ft[iMU] = G[iMU] * sumg[iMU];
  }  // end iMU

  // compute force-time/stimulation frequency dependency Poptft
  double Poptft = 0.0;
  for (int iMU = 0; iMU < muTypesNum; ++iMU)
  {
    // sum up twitch forces for all MU types weighted by the respective MU type fraction
    Poptft += Na * Ft[iMU] * rho[iMU];
  }

  return Poptft;
}

double MAT::UTILS::MUSCLE::EvaluateActiveForceStretchDependencyBlemker(
    const double lambdaM, const double lambdaOpt)
{
  // helper variable
  double ratio_lambda = lambdaM / lambdaOpt;

  // active stretch dependency fxi
  double fxi = 1.0;
  if (lambdaM <= 0.6 * lambdaOpt)
  {
    fxi = 9 * std::pow(ratio_lambda - 0.4, 2.0);
  }
  else if (lambdaM < 1.4 * lambdaOpt)
  {
    fxi = 1 - 4 * std::pow(1 - ratio_lambda, 2.0);
  }
  else
  {
    fxi = 9 * std::pow(ratio_lambda - 1.6, 2.0);
  }

  return fxi;
}

double MAT::UTILS::MUSCLE::EvaluateDerivativeActiveForceStretchDependencyBlemker(
    const double lambdaM, const double lambdaOpt)
{
  // helper variable
  double ratio_lambda = lambdaM / lambdaOpt;

  // derivative of active stretch dependency fxi w.r.t. fiber stretch
  double dFxidLamdaM = 1.0;
  if (lambdaM <= 0.6 * lambdaOpt)
  {
    dFxidLamdaM = 18 / lambdaOpt * (ratio_lambda - 0.4);
  }
  else if (lambdaM < 1.4 * lambdaOpt)
  {
    dFxidLamdaM = 8 / lambdaOpt * (1 - ratio_lambda);
  }
  else
  {
    dFxidLamdaM = 18 / lambdaOpt * (ratio_lambda - 1.6);
  }

  return dFxidLamdaM;
}

double MAT::UTILS::MUSCLE::EvaluatePassiveForceStretchDependencyBlemker(const double lambdaM,
    const double lambdaOpt, const double lambdaStar, const double P1, const double P2)
{
  // helper variable
  double ratio_lambda = lambdaM / lambdaOpt;

  // calculate constants
  double P3 = P1 * P2 * std::exp(P2 * (lambdaStar / lambdaOpt - 1.0)) / lambdaOpt;
  double P4 =
      P1 * (std::exp(P2 * ((lambdaStar / lambdaOpt) - 1.0)) - 1.0) - P3 * lambdaStar / lambdaOpt;

  // passive stretch dependency fxi
  double fxi = 1.0;
  if (lambdaM <= lambdaOpt)
  {
    fxi = 0.0;
  }
  else if (lambdaM < lambdaStar)
  {
    fxi = P1 * (std::exp(P2 * (ratio_lambda - 1.0)) - 1.0);
  }
  else
  {
    fxi = P3 * ratio_lambda + P4;
  }

  return fxi;
}

double MAT::UTILS::MUSCLE::EvaluateDerivativePassiveForceStretchDependencyBlemker(
    const double lambdaM, const double lambdaOpt, const double lambdaStar, const double P1,
    const double P2)
{
  // helper variable
  double ratio_lambda = lambdaM / lambdaOpt;

  // calculate constant
  double P3 = P1 * P2 * std::exp(P2 * (lambdaStar / lambdaOpt - 1.0)) / lambdaOpt;

  // derivative of passive stretch dependency fxi w.r.t. fibre stretch
  double dFxidLamdaM = 1.0;
  if (lambdaM <= lambdaOpt)
  {
    dFxidLamdaM = 0.0;
  }
  else if (lambdaM < lambdaStar)
  {
    dFxidLamdaM = P1 * std::exp(P2 * (ratio_lambda - 1.0)) * P2 / lambdaOpt;
  }
  else
  {
    dFxidLamdaM = P3 / lambdaOpt;
  }

  return dFxidLamdaM;
}

double MAT::UTILS::MUSCLE::EvaluateTimeDependentActiveStressTanh(const double sigma_max,
    const double alpha, const double beta, const double t_act_start, const double t_current)
{
  // compute time-depencency ft
  double ft = 0;
  if (t_current >= t_act_start)
  {
    ft = alpha * std::tanh(beta * (t_current - t_act_start));
  }

  // compute active optimal stress at current time
  double sigma_max_ft = sigma_max * ft;

  return sigma_max_ft;
}

double MAT::UTILS::MUSCLE::FiberStretch(
    const LINALG::Matrix<3, 3> &C, const LINALG::Matrix<3, 3> &M)
{
  // product C^T*M
  LINALG::Matrix<3, 3> transpCM(false);
  transpCM.MultiplyTN(C, M);  // C^TM = C^T*M

  // stretch in fibre direction lambdaM
  // lambdaM = sqrt(C:M) = sqrt(tr(C^T M)), see Holzapfel2000, p.14
  double lambdaM = std::sqrt(transpCM(0, 0) + transpCM(1, 1) + transpCM(2, 2));

  return lambdaM;
}

LINALG::Matrix<3, 3> MAT::UTILS::MUSCLE::DFiberStretch_DC(
    const double lambdaM, const LINALG::Matrix<3, 3> &C, const LINALG::Matrix<3, 3> &M)
{
  // derivative of lambdaM w.r.t. C
  LINALG::Matrix<3, 3> dlambdaMdC(M);
  dlambdaMdC.Scale(0.5 / lambdaM);

  return dlambdaMdC;
}

double MAT::UTILS::MUSCLE::ContractionVelocityBWEuler(
    const double lambdaM, const double lambdaMOld, const double timeStepSize)
{
  double dotLambdaM = (lambdaM - lambdaMOld) / timeStepSize;
  return dotLambdaM;
}