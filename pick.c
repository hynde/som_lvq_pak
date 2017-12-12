/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  pick.c                                                              *
 *  -picks a given number of entries from list                          *
 *                                                                      *
 *  Version 3.0                                                         *
 *  Date: 1 mar 1995                                                    *
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
#include "lvq_rout.h"
#include "datafile.h"

static char *usage[] = {
  " pick - picks a given number of entries from list\n",
  "Required parameters:\n",
  "  -din filename         data file\n",
  "  -cout filename        output codebook file\n",
  "  -noc integer          number of codes to pick\n",
  NULL};

int main(int argc, char **argv)
{
  int num;
  char *in_data_file;
  char *out_code_file;
  struct entries *data, *codes;

  global_options(argc, argv);
  if (extract_parameter(argc, argv, "-help", OPTION2))
    {
      printhelp();
      exit(0);
    }
  in_data_file = extract_parameter(argc, argv, IN_DATA_FILE, ALWAYS);
  out_code_file = extract_parameter(argc, argv, OUT_CODE_FILE, ALWAYS);
  num = oatoi(extract_parameter(argc, argv, NUMBER_OF_CODES, ALWAYS), 1);

  ifverbose(2)
    fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
  if ((data = open_entries(in_data_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file '%s' for reading\n", in_data_file);
      exit(1);
    }
  
  codes = pick_codes(num, data);

  close_entries(data);
  ifverbose(2)
    fprintf(stderr, "Codebook entries are saved to file %s\n", out_code_file);
  save_entries(codes, out_code_file);
  close_entries(codes);
  invalidate_alphafile(out_code_file);

  return(0);
}
