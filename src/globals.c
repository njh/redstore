/*
    RedStore - a lightweight RDF triplestore powered by Redland
    Copyright (C) 2010-2011 Nicholas J Humfrey <njh@aelius.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include "redstore.h"


int quiet = 0;                  // Only display error messages
int verbose = 0;                // Increase number of logging messages
int running = 1;                // True while still running
int exit_code = EXIT_SUCCESS;   // Exit code for when RedStore exits
unsigned long query_count = 0;
unsigned long import_count = 0;
unsigned long request_count = 0;
const char *storage_name = NULL;
const char *storage_type = NULL;
char *public_storage_options = NULL;

librdf_world *world = NULL;
librdf_model *model = NULL;
librdf_storage *storage = NULL;

raptor_stringbuffer *error_buffer = NULL;
