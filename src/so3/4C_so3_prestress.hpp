// This file is part of 4C multiphysics licensed under the
// GNU Lesser General Public License v3.0 or later.
//
// See the LICENSE.md file in the top-level for license information.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef FOUR_C_SO3_PRESTRESS_HPP
#define FOUR_C_SO3_PRESTRESS_HPP

#include "4C_config.hpp"

#include "4C_comm_parobject.hpp"
#include "4C_comm_parobjectfactory.hpp"
#include "4C_linalg_fixedsizematrix.hpp"
#include "4C_linalg_serialdensematrix.hpp"

FOUR_C_NAMESPACE_OPEN

namespace Discret
{
  namespace Elements
  {
    class PreStressType : public Core::Communication::ParObjectType
    {
     public:
      std::string name() const override { return "PreStressType"; }

      static PreStressType& instance() { return instance_; };

     private:
      static PreStressType instance_;
    };

    /*!
    \brief A class for handling the prestressing in finite deformations

    */
    class PreStress : public Core::Communication::ParObject
    {
     public:
      /*!
      \brief Standard Constructor
      */
      PreStress(const int numnode, const int ngp, const bool istet4 = false);

      /*!
      \brief Copy Constructor
      */
      PreStress(const Discret::Elements::PreStress& old);

      /*!
      \brief Return unique ParObject id

      every class implementing ParObject needs a unique id defined at the
      top of this file.
      */
      int unique_par_object_id() const override;
      /*!
      \brief Pack this class so it can be communicated

      \ref pack and \ref unpack are used to communicate this node

      */
      void pack(Core::Communication::PackBuffer& data) const override;

      /*!
      \brief Unpack data from a char vector into this class

      \ref pack and \ref unpack are used to communicate this node

      */
      void unpack(Core::Communication::UnpackBuffer& buffer) override;

      /// get history of deformation gradient
      inline Core::LinAlg::SerialDenseMatrix& f_history() const { return *fhist_; }

      /// get history of of reference configuration (inverse of Jacobian)
      inline Core::LinAlg::SerialDenseMatrix& j_history() const { return *inv_jhist_; }

      /// put a matrix to storage
      inline void matrixto_storage(const int gp, const Core::LinAlg::Matrix<3, 3>& Mat,
          Core::LinAlg::SerialDenseMatrix& gpMat) const
      {
        for (int i = 0; i < gpMat.numCols(); ++i) gpMat(gp, i) = Mat.data()[i];
        return;
      }

      /// put a matrix to storage
      inline void matrixto_storage(const int gp, const Core::LinAlg::Matrix<4, 3>& Mat,
          Core::LinAlg::SerialDenseMatrix& gpMat) const
      {
        for (int i = 0; i < gpMat.numCols(); ++i) gpMat(gp, i) = Mat.data()[i];
        return;
      }

      /// get matrix from storage
      inline void storageto_matrix(const int gp, Core::LinAlg::Matrix<3, 3>& Mat,
          const Core::LinAlg::SerialDenseMatrix& gpMat) const
      {
        for (int i = 0; i < gpMat.numCols(); ++i) Mat.data()[i] = gpMat(gp, i);
        return;
      }

      /// get matrix from storage
      inline void storageto_matrix(const int gp, Core::LinAlg::Matrix<4, 3>& Mat,
          const Core::LinAlg::SerialDenseMatrix& gpMat) const
      {
        for (int i = 0; i < gpMat.numCols(); ++i) Mat.data()[i] = gpMat(gp, i);
        return;
      }

      /// get indication whether class is initialized (important for restarts)
      bool& is_init() { return isinit_; }

     private:
      /// flagindicating whether material configuration has been initialized
      bool isinit_;

      /// no. nodal points of element
      int numnode_;

      /// history of deformation gradient
      std::shared_ptr<Core::LinAlg::SerialDenseMatrix> fhist_;

      /// updated Lagrange inverse of Jacobian
      std::shared_ptr<Core::LinAlg::SerialDenseMatrix> inv_jhist_;

      /// get number of gaussian points considered
      inline int num_gp() const { return fhist_->numRows(); }

      /// get no. of nodal points
      inline int num_node() const { return numnode_; }

    };  // class PreStress
  }     // namespace Elements
}  // namespace Discret


FOUR_C_NAMESPACE_CLOSE

#endif
