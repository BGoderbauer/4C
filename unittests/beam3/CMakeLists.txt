# This file is part of 4C multiphysics licensed under the
# GNU Lesser General Public License v3.0 or later.
#
# See the LICENSE.md file in the top-level for license information.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

set(TESTNAME unittests_beam3)

set(SOURCE_LIST
    #cmake - format : sortable
    4C_beam3_euler_bernoulli_test.cpp 4C_beam3_kirchhoff_test.cpp 4C_beam3_reissner_test.cpp
    )

four_c_add_google_test_executable(${TESTNAME} SOURCE ${SOURCE_LIST})
