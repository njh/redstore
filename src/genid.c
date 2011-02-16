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
#include <string.h>

#include "redstore.h"
#include "redstore_config.h"

#define REDSTORE_ID_LEN  (9)
static unsigned long base_id = 0;


char* redstore_genid(void)
{
  const char chars[] = "0123456789abcdef";
  const int base = sizeof(chars) - 1;
  unsigned long quotient, remainder;
  int i=0, index=0;
  char *str = NULL;

  // FIXME: this won't work if redstore starts using fork()
  if (base_id == 0) {
#ifdef HAVE_SRANDOMDEV
    srandomdev();
#endif
    base_id = random();
  }

  str = calloc(1, REDSTORE_ID_LEN+1);
  if (!str)
    return NULL;

  quotient = base_id++;
  for (i=0; i<REDSTORE_ID_LEN; i++) {
    remainder = quotient % base;
    quotient = quotient / base;
    index = (index + i + remainder) % base;
    str[i] = chars[index];

    // Add a dash after the 4th character
    if (i == 3)
      str[++i] = '-';
  }

  return str;
}
