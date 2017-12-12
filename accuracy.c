/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  accuracy.c                                                          *
 *  -computes the recognition accuracy by the nearest-neighbor rule     *
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
  "accuracy - computes the recognition accuracy by the nearest-neighbor rule\n",
  "Required parameters:\n",
  "  -cin filename         codebook file\n",
  "  -din filename         test data\n",
  "Optional parameters:\n",
  "  -cfout filename       output classification file\n",
  "  -buffer integer       buffered reading of data, integer lines at a time\n",
  "  -selfuncs name        select a set of functions\n",
  NULL};

int compute_accuracy(struct teach_params *teach, struct file_info *of)
{
  long total, stotal, noc;
  struct winner_info winner;
  struct hitlist *correct, *totals;
  struct hit_entry *he;
  FILE *ocf;
  int datalabel;
  struct entries *data = teach->data;
  struct entries *codes = teach->codes;
  WINNER_FUNCTION *find_winner = teach->winner;
  struct data_entry *datatmp;
  eptr p;

  ocf = of ? fi2fp(of) : NULL;

  if ((correct = new_hitlist()) == NULL)
    {
      return ERR_NOMEM;
    }

  if ((totals = new_hitlist()) == NULL)
    {
      free_hitlist(correct);
      return ERR_NOMEM;
    }

  stotal = 0;
  total = 0;

  if ((datatmp = rewind_entries(data, &p)) == NULL)
    {
      fprintf(stderr, "compute_accuracy: can't get data\n");
      goto end;
    }


  /* Number of data vectors */
  noc = data->flags.totlen_known ? data->num_entries : 0;

  /* Scan all input entries */
  while (datatmp != NULL) {

    find_winner(codes, datatmp, &winner, 1);
    
    /* If classification was correct */
    datalabel = get_entry_label(datatmp);
    if (get_entry_label(winner.winner) == datalabel) {
      /* Number of correct classifications */
      stotal++;

      /* Number of correct classifications in that class */
      add_hit(correct, datalabel);

      /* Write '1' to classification description file */
      if (ocf != NULL) fprintf(ocf,"1\n");

    } else {
      /* Write '0' to classification description file */
      if (ocf != NULL) fprintf(ocf,"0\n");
    }
     
    /* Total number of entries in that class */
    add_hit(totals, datalabel);

    /* Total number of entries */
    total++;

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

  fprintf(stdout, "\nRecognition accuracy:\n\n");
  for (he = totals->head; he != NULL; he = he->next) 
    {
      long res, tot;

      tot = he->freq;
      res = hitlist_label_freq(correct, he->label);
      
      fprintf(stdout, "%9s: %4ld entries ", find_conv_to_lab(he->label), tot);
      fprintf(stdout, "%6.2f %%\n", 100.0 * (float) res / tot);
    }
  fprintf(stdout, "\nTotal accuracy: %5ld entries %6.2f %%\n\n", total,
          100.0 * (float) stotal / total);
 end:
  free_hitlist(correct);
  free_hitlist(totals);
  return 0;
}

int main(int argc, char **argv)
{
  char *in_data_file;
  char *in_code_file;
  char *out_classification_file;
  struct entries *data = NULL, *codes = NULL;
  struct file_info *ocf=NULL;
  int error = 0;
  long buffer = 0;
  struct teach_params params;
  char *funcname = NULL;

  global_options(argc, argv);

  if (extract_parameter(argc, argv, "-help", OPTION2))
    {
      printhelp();
      exit(0);
    }

  in_data_file = extract_parameter(argc, argv, IN_DATA_FILE, ALWAYS);
  in_code_file = extract_parameter(argc, argv, IN_CODE_FILE, ALWAYS);
  buffer = oatoi(extract_parameter(argc, argv, "-buffer", OPTION), 0);

  out_classification_file = 
	extract_parameter(argc, argv, OUT_CLASSIFICATION_FILE, OPTION);
  funcname = extract_parameter(argc, argv, "-selfuncs", OPTION);

  ifverbose(2)
    fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
  if ((data = open_entries(in_data_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file '%s'\n", in_data_file);
      error = 1;
      goto end;
    }

  ifverbose(2)
    fprintf(stderr, "Codebook entries are read from file %s\n", in_code_file);

  if ((codes = open_entries(in_code_file)) == NULL)
    {
      fprintf(stderr, "Can't open code file '%s'\n", in_data_file);
      error = 1;
      goto end;
    }

  if (data->dimension != codes->dimension) {
    fprintf(stderr, "Data and codebook vectors have different dimensions");
    error = 1;
    goto end;
  }

  if (out_classification_file !=  NULL) {
    ifverbose(2)
      fprintf(stderr, "Classifications are saved to file %s\n",
              out_classification_file);

    if ((ocf = open_file(out_classification_file, "w")) == NULL) {
      fprintf(stderr, "Cannot open '%s' for output\n",out_classification_file);
      error = 1;
      goto end;
    }
  }

  set_teach_params(&params, codes, data, buffer, funcname);
  
  compute_accuracy(&params, ocf);

 end:
  
  if (ocf)
    close_file(ocf);
  if (data)
    close_entries(data);
  if (codes)
    close_entries(codes);

  return(error);
}
