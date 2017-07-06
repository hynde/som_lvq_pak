/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  labels.c                                                            *
 *  -displays the number of entries in each class                       *
 *                                                                      *
 *  Version 3.0                                                         *
 *  Date: 1 Mar 1995                                                    *
 *                                                                      *
 *  NOTE: This program package is copyrighted in the sense that it      *
 *  may be used for scientific purposes. The package as a whole, or     *
 *  parts thereof, cannot be included or used in any commercial         *
 *  application without written permission granted by its producents.   *
 *  No programs contained in this package may be copied for commercial  *
 *  distribution.                                                       *
 *                                                                      *
 *  All comments  concerning this program package may be sent to the    *
 *  e-mail address 'lvq@cochlea.hut.fi'.                                *
 *                                                                      *
 ************************************************************************/

#include <stdio.h>
#include "lvq_pak.h"
#include "datafile.h"
#include "labels.h"

static char *usage[] = {
  "showlabs - displays the number of entries in each class\n",
  "Required parameters:\n",
  "  -cin filename         input codebook file\n",
  "Optional parameters:\n",
  "  -buffer integer       buffered reading of data, integer lines at a time\n",
  NULL};

int labels(struct entries *data)
{
  struct data_entry *e;
  struct hit_entry *h;
  struct hitlist *classes;
  eptr p;

  if ((classes = new_hitlist()) == NULL)
    return -1; /* error */

  for (e = rewind_entries(data, &p); e != NULL; e = next_entry(&p))
    add_hit(classes, get_entry_label(e));

  for (h = classes->head; h != NULL; h = h->next)
    {
      fprintf(stdout, "In class %s are %ld units\n",
	      find_conv_to_lab(h->label), h->freq);
    }

  return 0;
}

int main(int argc, char **argv)
{
  char *in_code_file;
  struct entries *codes;
  long buffer;

  global_options(argc, argv);
  if (extract_parameter(argc, argv, "-help", OPTION2))
    {
      printhelp();
      exit(0);
    }
  in_code_file = extract_parameter(argc, argv, IN_CODE_FILE, ALWAYS);
  buffer = oatoi(extract_parameter(argc, argv, "-buffer", OPTION), 0);

  ifverbose(2)
    fprintf(stderr, "Code entries are read from file %s\n", in_code_file);
  if ((codes = open_entries(in_code_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file '%s'\n", in_code_file);
      exit(1);
    }

  set_buffer(codes, buffer);
  labels(codes);

  close_entries(codes);
  return(0);
}
