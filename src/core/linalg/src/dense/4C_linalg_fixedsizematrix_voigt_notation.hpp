/*! \file
\level 1
\brief Voigt notation definition and utilities
*/

#ifndef FOUR_C_LINALG_FIXEDSIZEMATRIX_VOIGT_NOTATION_HPP
#define FOUR_C_LINALG_FIXEDSIZEMATRIX_VOIGT_NOTATION_HPP

#include "4C_config.hpp"

#include "4C_linalg_fixedsizematrix.hpp"

#include <array>

FOUR_C_NAMESPACE_OPEN

namespace CORE::LINALG::VOIGT
{
  /*! Voigt notation types
   *
   * This enum can be used whenever the distinction between stress- and strain-like Voigt notation
   * is important. The classical Voigt notation is only meaningful for symmetric tensors.
   */
  enum class NotationType
  {
    /// stress-like Voigt notation
    ///
    /// A vector in stress-like Voigt notation, contains the off-diagonal values only once.
    stress,
    /// strain-like Voigt notation
    ///
    /// A vector in strain-like Voigt notation, contains the sum of corresponding off-diagonal
    /// values.
    strain
  };

  /*--------------------------------------------------------------------------*/
  /** \brief Utility routines for the perturbed Voigt tensor notation
   *
   * \tparam type specific NotationType this class operates on
   */
  template <NotationType type>
  class VoigtUtils
  {
   public:
    /// instantiation is forbidden
    VoigtUtils() = delete;

    /** \brief compute power of a symmetric 3x3 matrix in perturbed Voigt notation
     *
     *  \f[
     *  [vtensorpow]_{AB} = [vtensor^{pow}]_{AB}
     *  \f]
     *
     *  \param[in] pow          positive integer exponent
     *  \param[in] vtensor      input tensor in Voigt <type> notation
     *  \param[in] vtensor_pow  result, i.e. input tensor to the given power
     */
    static void PowerOfSymmetricTensor(unsigned pow, const CORE::LINALG::Matrix<6, 1>& strain,
        CORE::LINALG::Matrix<6, 1>& strain_pow);

    /** \brief Compute the inverse tensor in perturbed Voigt notation
     *
     *  \param[in]  vtensor      tensor in Voigt <type> notation
     *  \param[out] vtensor_inv  inverse tensor in Voigt <type> notation
     */
    static void InverseTensor(
        const CORE::LINALG::Matrix<6, 1>& tens, CORE::LINALG::Matrix<6, 1>& tens_inv);

    /**
     * \brief Compute the determinant of a matrix in Voigt <type> notation
     *
     * @param vtensor Tensor in Voigt <type> notation
     */
    static inline double Determinant(const CORE::LINALG::Matrix<6, 1>& vtensor)
    {
      return TripleEntryProduct(vtensor, 0, 1, 2) + 2 * TripleEntryProduct(vtensor, 3, 4, 5) -
             TripleEntryProduct(vtensor, 1, 5, 5) - TripleEntryProduct(vtensor, 2, 3, 3) -
             TripleEntryProduct(vtensor, 0, 4, 4);
    }

    /** \brief Compute the three principal invariants of a matrix in Voigt <type> notation
     *
     * @param[out] prinv the three principal invariants
     * @param vtensor tensor in Voigt <type> notation
     */
    static inline void InvariantsPrincipal(
        CORE::LINALG::Matrix<3, 1>& prinv, const CORE::LINALG::Matrix<6, 1>& vtensor)
    {
      // 1st invariant, trace tens
      prinv(0) = vtensor(0) + vtensor(1) + vtensor(2);
      // 2nd invariant, 0.5( (trace(tens))^2 - trace(tens^2))
      prinv(1) = 0.5 * (prinv(0) * prinv(0) - vtensor(0) * vtensor(0) - vtensor(1) * vtensor(1) -
                           vtensor(2) * vtensor(2)) -
                 vtensor(3) * vtensor(3) * UnscaleFactor(3) * UnscaleFactor(3) -
                 vtensor(4) * vtensor(4) * UnscaleFactor(4) * UnscaleFactor(4) -
                 vtensor(5) * vtensor(5) * UnscaleFactor(5) * UnscaleFactor(5);
      // 3rd invariant, determinant tens
      prinv(2) = Determinant(vtensor);
    }

    /** \brief Compute the product of a tensor in perturbed Voigt notation
     *  and a vector
     *
     *  \f$ [vecres]_{A} = vtensor_{AB} vec^{B} \f$
     *
     *  \param[in]  vtensor      tensor in Voigt <type> notation
     *  \param[out] vtensor_inv  inverser tensor in Voigt <type> notation
     */
    static void MultiplyTensorVector(const CORE::LINALG::Matrix<6, 1>& strain,
        const CORE::LINALG::Matrix<3, 1>& vec, CORE::LINALG::Matrix<3, 1>& vec_res);

    /** \brief Compute the symmetric outer product of two vectors
     *
     *  \f$ [abba]_{AB} = [veca]_{A} [vecb]_{B} + [veca]_{B} [vecb]_{A} \f$
     *
     *  \param[in]  vec_a  first vector
     *  \param[in]  vec_b  second vector
     *  \param[out] ab_ba  symmetric outer product of the two input vectors
     *                     in the Voigt <type> notation
     */
    static void SymmetricOuterProduct(const CORE::LINALG::Matrix<3, 1>& vec_a,
        const CORE::LINALG::Matrix<3, 1>& vec_b, CORE::LINALG::Matrix<6, 1>& ab_ba);

    /*!
     * Converts a <type>-like tensor to stress-like Voigt notation
     * @param vtensor_in tensor in <type>-like Voigt notation
     * @param vtensor_out tensor in stress-like Voigt notation
     */
    static void ToStressLike(
        const CORE::LINALG::Matrix<6, 1>& vtensor_in, CORE::LINALG::Matrix<6, 1>& vtensor_out);

    /*!
     * Converts a <type>-like tensor to strain-like Voigt notation
     * @param vtensor_in tensor in <type>-like Voigt notation
     * @param vtensor_out tensor in strain-like Voigt notation
     */
    static void ToStrainLike(
        const CORE::LINALG::Matrix<6, 1>& vtensor_in, CORE::LINALG::Matrix<6, 1>& vtensor_out);

    /*!
     * Converts a <type>-like tensor in Voigt notation to a matrix
     * @param vtensor_in tensor in <type>-like Voigt notation
     * @param tensor_out tensor as a matrix
     */
    static void VectorToMatrix(
        const CORE::LINALG::Matrix<6, 1>& vtensor_in, CORE::LINALG::Matrix<3, 3>& tensor_out);


    /*! Copy matrix contents to type-like Voigt notation
     *
     * Matrix [A_00 A_01 A_02; A_10 A_11 A_12; A_20 A_21 A_22] is converted to the Voigt vector
     * [A_00; A_11; A_22; 0.5*k*(A_01 + A_10); 0.5*k*(A_12 + A_21); 0.5*k*(A_02 + A_20)]
     *
     * where k is the scale factor for type-like Voigt notation.
     *
     * @param tensor_in the matrix to copy from
     * @param[out] vtensor_out target tensor in <type>-like Voigt notation
     */
    template <typename T>
    static void MatrixToVector(
        const CORE::LINALG::Matrix<3, 3, T>& tensor_in, CORE::LINALG::Matrix<6, 1, T>& vtensor_out);

    /// access scaling factors
    static inline double ScaleFactor(unsigned i) { return scale_fac_[i]; };

    /// access unscaling factors
    static inline double UnscaleFactor(unsigned i) { return unscale_fac_[i]; };

   private:
    /** \brief scale off diagonal values
     *
     *  \note This function changes the values only if the strain notation is used.
     *
     *  \param[out] tensor  scale the off-diagonal values of this tensor
     */
    static void ScaleOffDiagonalVals(CORE::LINALG::Matrix<6, 1>& strain);

    /** \brief unscale off diagonal values
     *
     *  \note This function changes the values only if the strain notation is used.
     *
     *  \param[out] tensor  unscale the off-diagonal values of this tensor
     */
    static void UnscaleOffDiagonalVals(CORE::LINALG::Matrix<6, 1>& strain);


    /** \brief unscale factors for the perturbed Voigt strain notation
     *
     *  These factors are meaningful if the strain convention is followed. */
    static constexpr std::array<double, 6> unscale_fac_ =
        type == NotationType::strain ? std::array{1.0, 1.0, 1.0, 0.5, 0.5, 0.5}
                                     : std::array{1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

    /** \brief scale factors for the perturbed Voigt stress notation
     *
     *  These factors are meaningful if the strain convention is followed. */
    static constexpr std::array<double, 6> scale_fac_ =
        type == NotationType::strain ? std::array{1.0, 1.0, 1.0, 2.0, 2.0, 2.0}
                                     : std::array{1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

    /**
     * \brief Compute the product of three matrix entries
     *
     * The entries are correctly scaled depending on the VoigtNotation type of the tensor.
     * @param vtensor the tensor in voigt notaion
     * @param i first entry's Voigt index
     * @param j second entry's Voigt inde
     * @param k third entry's Voigt index
     * @return product of the three entries
     */
    static inline double TripleEntryProduct(
        const CORE::LINALG::Matrix<6, 1>& vtensor, unsigned i, unsigned j, unsigned k)
    {
      return vtensor(i) * UnscaleFactor(i) * vtensor(j) * UnscaleFactor(j) * vtensor(k) *
             UnscaleFactor(k);
    }
  };


  /// convert non-symmetric 2-tensor to 9x1 vector
  ///          A_00 A_01 A_02
  /// Matrix   A_10 A_11 A_12  is converted to the vector below:
  ///          A_20 A_21 A_22
  ///
  /// Vector   V_0 = A_00; V_1 = A_11; V_2 = A_22; V_3 = A_01; V_4 = A_12; V_5 = A_02; V_6 = A_10;
  /// V_7 = A_21; V_8 = A_20
  void Matrix3x3to9x1(CORE::LINALG::Matrix<3, 3> const& in, CORE::LINALG::Matrix<9, 1>& out);

  /// convert 9x1 vector to non-symmetric 2-tensor
  ///
  /// Vector   V_0 = A_00; V_1 = A_11; V_2 = A_22; V_3 = A_01; V_4 = A_12; V_5 = A_02; V_6 = A_10;
  /// V_7 = A_21; V_8 = A_20
  ///
  /// is converted to
  ///
  ///          A_00 A_01 A_02
  /// Matrix   A_10 A_11 A_12
  ///          A_20 A_21 A_22
  void Matrix9x1to3x3(CORE::LINALG::Matrix<9, 1> const& in, CORE::LINALG::Matrix<3, 3>& out);

  /**
   * \brief Identity matrix in stress/strain-like Voigt notation
   * @param id (out) : 2nd order identity tensor in stress/strain-like Voigt notation
   */
  inline void IdentityMatrix(CORE::LINALG::Matrix<6, 1>& id)
  {
    id.Clear();
    for (unsigned i = 0; i < 3; ++i) id(i) = 1.0;
  }

  /*!
   * \brief Constructs a 4th order identity matrix with rows in <rows_notation>-type Voigt
   * notation and columns in <cols_notation>-type Voigt notation
   * @tparam rows_notation Voigt notation used for the rows
   * @tparam cols_notation  Voigt notation used for the columns
   * @param id (out) : Voigt-Matrix
   */
  template <NotationType rows_notation, NotationType cols_notation>
  void FourthOrderIdentityMatrix(CORE::LINALG::Matrix<6, 6>& id);

  /// collection of index mappings from matrix to Voigt-notation or vice versa
  struct IndexMappings
  {
   public:
    /**
     * from 6-Voigt index to corresponding 2-tensor row
     * @param i the index of a 6x1 vector in Voigt notation
     * @return row index of the corresponding 3x3 matrix
     */
    static inline int Voigt6ToRow(unsigned int i)
    {
      assertRangeVoigtIndex(i);
      static constexpr int VOIGT6ROW[6] = {0, 1, 2, 0, 1, 2};
      return VOIGT6ROW[i];
    };

    /// from 6-Voigt index to corresponding 2-tensor col index
    static inline int Voigt6ToCol(unsigned int i)
    {
      assertRangeVoigtIndex(i);
      static constexpr int VOIGT6COL[6] = {0, 1, 2, 1, 2, 0};
      return VOIGT6COL[i];
    };

    /// from symmetric 2-tensor index pair to 6-Voigt index
    static inline int SymToVoigt6(unsigned int row, unsigned int col)
    {
      assertRangeMatrixIndex(row, col);
      static constexpr int VOIGT3X3SYM[3][3] = {{0, 3, 5}, {3, 1, 4}, {5, 4, 2}};
      return VOIGT3X3SYM[row][col];
    };

    /// from non-symmetric 2-tensor index pair to 9-Voigt index
    static inline int NonSymToVoigt9(unsigned int row, unsigned int col)
    {
      assertRangeMatrixIndex(row, col);
      static constexpr int VOIGT3X3NONSYM[3][3] = {{0, 3, 5}, {6, 1, 4}, {8, 7, 2}};
      return VOIGT3X3NONSYM[row][col];
    };

    /// from symmetric 6x6 Voigt notation matrix indices (e.g. constitutive tensor) to one of the
    /// four indices of a four tensor
    static inline int Voigt6x6To4Tensor(
        unsigned int voigt_row, unsigned int voigt_col, unsigned int target_index)
    {
      assertRangeVoigtIndex(voigt_row);
      assertRangeVoigtIndex(voigt_col);
      FOUR_C_ASSERT(target_index < 4, "target index for fourth order tensor out of range");
      static constexpr int FOURTH[6][6][4] = {
          {{0, 0, 0, 0}, {0, 0, 1, 1}, {0, 0, 2, 2}, {0, 0, 0, 1}, {0, 0, 1, 2}, {0, 0, 0, 2}},
          {{1, 1, 0, 0}, {1, 1, 1, 1}, {1, 1, 2, 2}, {1, 1, 0, 1}, {1, 1, 1, 2}, {1, 1, 0, 2}},
          {{2, 2, 0, 0}, {2, 2, 1, 1}, {2, 2, 2, 2}, {2, 2, 0, 1}, {2, 2, 1, 2}, {2, 2, 0, 2}},
          {{0, 1, 0, 0}, {0, 1, 1, 1}, {0, 1, 2, 2}, {0, 1, 0, 1}, {0, 1, 1, 2}, {0, 1, 0, 2}},
          {{1, 2, 0, 0}, {1, 2, 1, 1}, {1, 2, 2, 2}, {1, 2, 0, 1}, {1, 2, 1, 2}, {1, 2, 0, 2}},
          {{0, 2, 0, 0}, {0, 2, 1, 1}, {0, 2, 2, 2}, {0, 2, 0, 1}, {0, 2, 1, 2}, {0, 2, 0, 2}}};
      return FOURTH[voigt_row][voigt_col][target_index];
    };

    /// instancing of this class is forbidden
    IndexMappings() = delete;

   private:
    static inline void assertRangeMatrixIndex(unsigned int row, unsigned int col)
    {
      FOUR_C_ASSERT(row < 3, "given row index out of range [0,2]");
      FOUR_C_ASSERT(col < 3, "given col index out of range [0,2]");
    }

    static inline void assertRangeVoigtIndex(unsigned int index)
    {
      FOUR_C_ASSERT(index < 6, "given index out of range [0,5]");
    }
  };

  //! typedefs for improved readability
  //! @{
  using Stresses = VoigtUtils<CORE::LINALG::VOIGT::NotationType::stress>;
  using Strains = VoigtUtils<CORE::LINALG::VOIGT::NotationType::strain>;
  //! @}

}  // namespace CORE::LINALG::VOIGT


FOUR_C_NAMESPACE_CLOSE

#endif