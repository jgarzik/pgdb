
#include "pgdb-internal.h"

void pgdb_options_set_create_if_missing(
    pgdb_options_t* opt, bool yn)
{
	opt->create_missing = yn;
}

void pgdb_options_set_error_if_exists(
    pgdb_options_t* opt, bool yn)
{
	opt->error_if_exists = yn;
}

