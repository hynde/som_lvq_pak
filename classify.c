/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  classify.c                                                          *
 *  -finds out the classifications against a given codebook             *
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
#include <float.h>
#include "lvq_pak.h"
#include "datafile.h"

static char *usage[] = {
  "classify - finds out the classifications against a given codebook\n",
  "Required parameters:\n",
  "  -cin filename         input codebook file\n",
  "  -din filename         input data\n",
  "  -dout filename        classified output data\n",
  "Optional parameters:\n",
  "  -cfout filename       output classification file\n",
  "  -buffer integer       buffered reading of data, integer lines at a time\n",
  "  -selfuncs name        select a set of functions\n",
  NULL};


int compute_classifications(struct teach_params *teach,
			    struct file_info *file)
{
  long noc;
  int label;
  struct data_entry *datatmp;
  FILE *ocf = fi2fp(file);
  struct entries *data = teach->data;
  struct entries *codes = teach->codes;
  WINNER_FUNCTION *find_winner = teach->winner;
  struct winner_info win;
  eptr p;

  if ((datatmp = rewind_entries(data, &p)) == NULL)
    {
      fprintf(stderr, "compute_classifications: can't get data\n");
      return 1;
    }

  /* Number of data vectors */
  noc = data->flags.totlen_known ? data->num_entries : 0;

  /* Scan all input entries */
  while (datatmp != NULL) {

    if (find_winner(codes, datatmp, &win, 1) == 0)
      {
	/* no winner found, all components of sample masked off */
	label = find_conv_to_ind("# empty datavector");
      }
    else
      {
	label = get_entry_label(win.winner); /* use only the first label */
	set_entry_label(datatmp, label);
      }

    /* Save to file if required */
    if (ocf != NULL) {
      fprintf(ocf, "%s\n", find_conv_to_lab(label));
    }

    /* Take the next input entry */
    datatmp = next_entry(&p);

    ifverbose(1)
      if (noc)
	mprint((long) noc--);
  }
  ifverbose(1)
    {
      mprint((long) 0);
      fprintf(stderr, "\n");
    }
  return 0;
}

int main(int argc, char **argv)
{
  char *in_data_file;
  char *out_data_file;
  char *out_classification_file;
  char *in_code_file;
  struct entries *data, *codes;
  struct file_info *ocf=NULL;
  struct teach_params params;
  long buffer;
  char *funcname = NULL;

  global_options(argc, argv);
  if (extract_parameter(argc, argv, "-help", OPTION2))
    {
      printhelp();
      exit(0);
    }

  in_data_file = extract_parameter(argc, argv, IN_DATA_FILE, ALWAYS);
  in_code_file = extract_parameter(argc, argv, IN_CODE_FILE, ALWAYS);
  out_classification_file = 
	extract_parameter(argc, argv, OUT_CLASSIFICATION_FILE, OPTION);
  out_data_file = extract_parameter(argc, argv, OUT_DATA_FILE, ALWAYS);
  buffer = oatoi(extract_parameter(argc, argv, "-buffer", OPTION), 0);
  funcname = extract_parameter(argc, argv, "-selfuncs", OPTION);

  label_not_needed(1);
  ifverbose(2)
    fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
  if ((data = open_entries(in_data_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file '%s'\n", in_data_file);
      exit(1);
    }

  label_not_needed(0);

  ifverbose(2)
    fprintf(stderr, "Codebook entries are read from file %s\n", in_code_file);
  if ((codes = open_entries(in_code_file)) == NULL)
    {
      fprintf(stderr, "Can't open codes file '%s'\n", in_code_file);
      close_entries(data);
      exit(1);
    }

  if (data->dimension != codes->dimension) {
    fprintf(stderr, "Data and codebook vectors have different dimensions");
    close_entries(data);
    close_entries(codes);
    exit(1);
  }
  
  if (out_classification_file != NULL) {
    ifverbose(2)
      fprintf(stderr, "Classifications are saved to file %s\n",
              out_classification_file);
    
    if ((ocf = open_file(out_classification_file,"w")) == NULL) {
      fprintf(stderr, "Cannot write to %s\n",out_classification_file);
      close_entries(data);
      close_entries(codes);
      exit(1);
    }
  }
  
  set_teach_params(&params, codes, data, buffer, funcname); 
  compute_classifications(&params, ocf);
  
  if (ocf)
    close_file(ocf);
  
  ifverbose(2)
    fprintf(stderr, "Output entries are saved to file %s\n", out_data_file);
  
  save_entries(data, out_data_file);

  close_entries(codes);
  close_entries(data);
  return(0);
}
