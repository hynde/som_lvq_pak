/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  cmatr.c                                                             *
 *  -computes the confusion matrix by the nearest-neighbor rule         *
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
#include "lvq_rout.h"
#include "labels.h"

static char *usage[] = {
  "cmatr - computes the confusion matrix by the nearest-neighbor rule\n",
  "Required parameters:\n",
  "  -cin filename         input codebook file\n",
  "  -din filename         input data\n",
  "Optional parameters:\n",
  "  -cfout filename       output classification file\n",
  "  -buffer integer       buffered reading of data, integer lines at a time\n",
  "  -selfuncs name        select a set of functions\n",
  NULL};

int compute_cmatr(struct teach_params *teach, FILE *ocf)
{
  long i, j;
  int label, datalabel;
  long total, stotal, noc;
  struct data_entry *dtmp;
  WINNER_FUNCTION *find_winner = teach->winner;
  struct entries *data = teach->data;
  struct entries *codes = teach->codes;
  struct winner_info win;
  struct hitlist *correct, *totals, *confuzion;
  struct hit_entry *he, *he2;
  eptr p;

  if ((correct = new_hitlist()) == NULL)
    {
      return -1;
    }

  if ((totals = new_hitlist()) == NULL)
    {
      free_hitlist(correct);
      return -1;
    }

  if ((confuzion = new_hitlist()) == NULL)
    {
      free_hitlist(totals);
      free_hitlist(correct);
      return -1;
    }
      
  stotal = 0;
  total = 0;

  dtmp = rewind_entries(data, &p);
  noc = data->flags.totlen_known ? data->num_entries : 0;

  /* Scan all input entries */
  while (dtmp != NULL) {

    datalabel = get_entry_label(dtmp);

    if (find_winner(codes, dtmp, &win, 1) == 0)
      {
	/* invalid data vector */
      }
    else
      {
	label = get_entry_label(win.winner);
	
	if (label == datalabel) {
	  /* Number of correct classifications */
	  stotal++;
	  
	  /* Number of correct classifications in that class */
	  add_hit(correct, datalabel);
	  
	  /* Write '1' to classification description file */
	  if (ocf != NULL) fprintf(ocf,"1\n");
	  
	} 
	else {
	  /* Write '0' to classification description file */
	  if (ocf != NULL) fprintf(ocf,"0\n");
	}
	
	/* increment confusion matrix */
	add_hit(confuzion, datalabel * 65536 + label);
	
	/* Total number of entries in that class */
	add_hit(totals, datalabel);
	
	/* Total number of entries */
	total++;
      }

    /* Take the next input entry */
    dtmp = next_entry(&p);

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
  
  for (he = totals->head; he != NULL; he = he->next) {
    fprintf(stdout, "%9s: %4ld entries ", find_conv_to_lab(he->label), 
	    he->freq);
    fprintf(stdout, "%6.2f %%\n", 
	    100.0 * (float) hitlist_label_freq(correct, he->label) / he->freq);
  }
  fprintf(stdout, "\nTotal accuracy: %5ld entries %6.2f %%\n\n", total,
          100.0 * (float) stotal / total);

  {
    char *chp;
    
    fprintf(stdout, "Confusion matrix:\n\n");
    
    fprintf(stdout, "          ");
    for (he = totals->head; he != NULL; he = he->next) {
      chp = find_conv_to_lab(he->label);
      fprintf(stdout, " %4s", chp);
  }
    fprintf(stdout, "\n\n");

    /* tmatr = cmatr; */
    for (i = 0, he = totals->head; he != NULL; i++, he = he->next) {
      fprintf(stdout, "%9s: ", find_conv_to_lab(he->label));
      for (j = 0, he2 = totals->head; he2 != NULL; j++, he2 = he2->next) {
	fprintf(stdout, "%4ld ", 
		hitlist_label_freq(confuzion, he->label * 65536 + he2->label));
      }
      fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
  }

  free_hitlist(totals);
  free_hitlist(correct);
  free_hitlist(confuzion);
  return 0;
}

int main(int argc, char **argv)
{
  char *in_data_file;
  char *in_code_file;
  char *out_classification_file;
  struct entries *data, *codes;
  struct file_info *ocf=NULL;
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
  out_classification_file = 
	extract_parameter(argc, argv, OUT_CLASSIFICATION_FILE, OPTION);
  buffer = oatoi(extract_parameter(argc, argv, "-buffer", OPTION), 0);

  ifverbose(2)
    fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
  if ((data = open_entries(in_data_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file '%s'\n", in_data_file);
      exit(1);
    }

  ifverbose(2)
    fprintf(stderr, "Codebook entries are read from file %s\n", in_code_file);
  if ((codes = open_entries(in_code_file)) == NULL)
    {
      fprintf(stderr, "Can't open code file '%s'\n", in_code_file);
      close_entries(data);
      exit(1);
    }

  if (data->dimension != codes->dimension) {
    fprintf(stderr, "Data and codebook vectors have different dimensions");
    close_entries(data);
    close_entries(codes);
    exit(1);
  }

  if (out_classification_file != (char *) NULL) {
    ifverbose(2)
      fprintf(stderr, "Classifications are saved to file %s\n",
              out_classification_file);

    if ( (ocf = open_file(out_classification_file, "w")) == NULL) {
      fprintf(stderr, "\nCannot write to %s\n",out_classification_file);
      close_entries(data);
      close_entries(codes);
      exit(-1);
    }
  }

  set_teach_params(&params, codes, data, buffer, funcname);
  
  compute_cmatr(&params, ocf ? fi2fp(ocf) : NULL);
  if (ocf!=NULL)
    close_file(ocf);
  close_entries(data);
  close_entries(codes);

  return(0);
}
