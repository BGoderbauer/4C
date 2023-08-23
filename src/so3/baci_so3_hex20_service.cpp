/*----------------------------------------------------------------------*/
/*! \file
\brief 3D quadratic serendipity element
\level 1

*----------------------------------------------------------------------*/
#include "baci_discretization_fem_general_utils_fem_shapefunctions.H"
#include "baci_lib_node.H"
#include "baci_so3_hex20.H"


/*----------------------------------------------------------------------*
 |  return Center Coords in Reference System                            |
 *----------------------------------------------------------------------*/
const std::vector<double> DRT::ELEMENTS::So_hex20::soh20_ElementCenterRefeCoords()
{
  // update element geometry
  DRT::Node** nodes = Nodes();
  CORE::LINALG::Matrix<NUMNOD_SOH20, NUMDIM_SOH20> xrefe;  // material coord. of element
  for (int i = 0; i < NUMNOD_SOH20; ++i)
  {
    const double* x = nodes[i]->X();
    xrefe(i, 0) = x[0];
    xrefe(i, 1) = x[1];
    xrefe(i, 2) = x[2];
  }
  const DRT::Element::DiscretizationType distype = Shape();
  CORE::LINALG::Matrix<NUMNOD_SOH20, 1> funct;
  // Element midpoint at r=s=t=0.0
  CORE::DRT::UTILS::shape_function_3D(funct, 0.0, 0.0, 0.0, distype);
  CORE::LINALG::Matrix<1, NUMDIM_SOH20> midpoint;
  // midpoint.Multiply('T','N',1.0,funct,xrefe,0.0);
  midpoint.MultiplyTN(funct, xrefe);
  std::vector<double> centercoords(3);
  centercoords[0] = midpoint(0, 0);
  centercoords[1] = midpoint(0, 1);
  centercoords[2] = midpoint(0, 2);
  return centercoords;
}