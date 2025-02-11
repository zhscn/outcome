#ifndef OUTCOME_LIBRARY_WRAPPER_HH
#define OUTCOME_LIBRARY_WRAPPER_HH

#include "vendor/outcome-experimental.hpp"

#if defined(OUTCOME_ENABLE_STD_ERROR_CODE)
#include "vendor/std_error_code.hpp"
#endif

#if defined(OUTCOME_ENABLE_EXCEPTION)
#include "vendor/system_code_from_exception.hpp"
#endif

#endif // OUTCOME_LIBRARY_WRAPPER_HH
