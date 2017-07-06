/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  extract.c                                                           *
 *  -extracts entries that belong to a given class                      *
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
#include "fileio.h"
#include "datafile.h"
#include "labels.h"

static char *usage[] = {
  "extract - extracts entries that belong to a given class\n",
  "Required parameters:\n",
  "  -din filename         input data\n",
  "  -cout filename        output codebook filename\n",
  "  -label string         label of class to extract\n",
  NULL};


/* Extract those vectors (entries) that belong to same class as given
   label */

struct entries *extract_codes(int label, struct entries *data)
{
  struct data_entry *prev, *loca, tmp;
  eptr p;

  struct entries *datac;
  
  datac = copy_entries(data);
  if (datac == NULL)
    {
      fprintf(stderr, "extract_codes: can't copy entries structure\n");
      return NULL;
    }

  /* Those entries are saved that have the same label as given */
  loca = rewind_entries(data, &p);
  prev = &tmp;
  tmp.next = NULL;
  while (loca != NULL) {
    if (get_entry_label(loca) == label) {
      prev->next = copy_entry(data, loca);
      prev = prev->next; 

      datac->num_entries++;

    }
    loca = next_entry(&p);
  }

  datac->entries = tmp.next;
  datac->num_loaded = datac->num_entries;
  datac->flags.totlen_known = 1;

  return(datac);
}

int main(int argc, char **argv)
{
  int label;
  char *in_data_file;
  char *out_code_file;
  char *label_s;
  struct entries *data, *codes;

  global_options(argc, argv);
  if (extract_parameter(argc, argv, "-help", OPTION2))
    {
      printhelp();
      exit(0);
    }
  in_data_file = extract_parameter(argc, argv, IN_DATA_FILE, ALWAYS);
  out_code_file = extract_parameter(argc, argv, OUT_CODE_FILE, ALWAYS);
  label_s = extract_parameter(argc, argv, LABEL, ALWAYS);

  ifverbose(2)
    fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
  if ((data = open_entries(in_data_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file '%s'\n", in_data_file);
      exit(1);
    }

  if ((codes = copy_entries(data)) == NULL)
    {
      fprintf(stderr, "Can't copy data-entries\n");
      close_entries(data);
      exit(1);
    }

  ifverbose(2)
    fprintf(stderr, "Codes %s are extracted\n", label_s);
  label = find_conv_to_ind(label_s);
  ifverbose(3)
    fprintf(stderr, "Index of that label is %d\n", label);

  if ((codes = extract_codes(label, data)) == NULL)
    {
      fprintf(stderr, "Extract failed\n");
      close_entries(data);
      exit(1);
    }
  
  ifverbose(2)
    fprintf(stderr, "Codebook entries are saved to file %s\n", out_code_file);
  save_entries(codes, out_code_file);
  invalidate_alphafile(out_code_file);
  close_entries(codes);
  close_entries(data);
      
  return(0);
}
