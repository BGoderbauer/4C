include(FindPackageHandleStandardArgs)

# using the general BACI python venv
set(ENV{VIRTUAL_ENV} "${BACI_VENV_DIR}")
set(Python_FIND_VIRTUALENV ONLY)
find_package(Python COMPONENTS Interpreter Development)

## We are likely to find Sphinx near the Python interpreter
find_program(
  SPHINX_EXECUTABLE
  NAMES "${BACI_VENV_DIR}/bin/sphinx-multibuild"
  DOC "Sphinx documentation generator"
  )

mark_as_advanced(SPHINX_EXECUTABLE)

find_package_handle_standard_args(Sphinx DEFAULT_MSG SPHINX_EXECUTABLE)