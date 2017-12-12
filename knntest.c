/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  knntest.c                                                           *
 *  -displays the recognition accuracy by knn test                      *
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
#include "errors.h"
#include "labels.h"
#include "datafile.h"

static char *usage[] = {
  "knntest - displays the recognition accuracy by knn test\n",
  "Required parameters:\n",
  "  -cin filename         input codebook file\n",
  "  -din filename         test data\n",
  "Optional parameters:\n",
  "  -knn N                use N nearest neighbours (default: 5)\n", 
  "  -buffer integer       buffered reading of data, integer lines at a time\n",
  "  -selfuncs name        select a set of functions\n",
  NULL};

int compute_knnaccuracy(struct teach_params *teach)
{
  long noc, i, total, stotal;
  struct data_entry *datatmp;
  struct winner_info *winners;
  WINNER_FUNCTION *find_winners = teach->winner;
  int datalabel;
  struct entries *data = teach->data;
  struct entries *codes = teach->codes;
  int knn = teach->knn;
  struct hitlist *hits, *correct, *totals;
  struct hit_entry *he;
  eptr p;

  if (knn < 1) 
    knn = 1;

  winners = calloc(knn, sizeof(struct winner_info));
  if (winners == NULL)
    return ERR_NOMEM;

  if ((hits = new_hitlist()) == NULL)
    {
      free(winners);
      return ERR_NOMEM;
    }

  if ((correct = new_hitlist()) == NULL)
    {
      free(winners);
      free_hitlist(hits);
      return ERR_NOMEM;
    }

  if ((totals = new_hitlist()) == NULL)
    {
      free(winners);
      free_hitlist(hits);
      free_hitlist(correct);
      return ERR_NOMEM;
    }

  stotal = 0;
  total = 0;

  ifverbose(3)
    fprintf(stderr, "computing accuracy\n");

  /* Scan all input entries */
  datatmp = rewind_entries(data, &p);

  /* Number of data vectors */
  noc = data->flags.totlen_known ? data->num_entries : 0;


  while (datatmp != NULL) {

    find_winners(codes, datatmp, winners, knn);

    /* If classification was correct */
    clear_hitlist(hits);

    for (i = 0; i < knn; i++)
      add_hit(hits, get_entry_label(winners[i].winner));

    datalabel = get_entry_label(datatmp);

    if (hits->head->label == datalabel)
      {
	/* Number of correct classifications */
	stotal++;
	
	/* Number of correct classifications in that class */
	add_hit(correct, datalabel);
      }
     
    /* Total number of entries in that class */
    add_hit(totals, datalabel);

    /* Total number of entries */
    total++;

    /* Take the next input entry */
    datatmp = next_entry(&p);

    ifverbose(1)
      if (noc)
	mprint(noc--);
  }

  ifverbose(1)
    {
      mprint((long) 0);
      fprintf(stderr, "\n");
    }

  fprintf(stdout, "\nRecognition accuracy:\n\n");
  for (he = totals->head; he != NULL; he = he->next) 
    {
      int res, tot;

      tot = he->freq;
      res = hitlist_label_freq(correct, he->label);
      
      fprintf(stdout, "%14s: ", find_conv_to_lab(he->label));
      fprintf(stdout, "%6.2f %%\n", 100.0 * (float) res / tot);
    }
  fprintf(stdout, "\nTotal accuracy: %6.2f %%\n\n",
	  100.0 * (float) stotal / total);

  free(winners);
  free_hitlist(hits);
  free_hitlist(correct);
  free_hitlist(totals);

  return 0;
}

int main(int argc, char **argv)
{
  int knn;
  long buffer;
  char *in_data_file;
  char *in_code_file;
  struct entries *data, *codes;
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
  knn = (int) oatoi(extract_parameter(argc, argv, KNN_NEIGHBORS, OPTION), 5);
  buffer = oatoi(extract_parameter(argc, argv, "-buffer", OPTION), 0);
  funcname = extract_parameter(argc, argv, "-selfuncs", OPTION);

  ifverbose(2)
    fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
  if ((data = open_entries(in_data_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file %s\n", in_data_file);
      exit(1);
    }

  ifverbose(2)
    fprintf(stderr, "Codebook entries are read from file %s\n", in_code_file);
  if ((codes = open_entries(in_code_file)) == NULL)
    {
      fprintf(stderr, "Can't open codes file %s\n", in_code_file);
      close_entries(data);
      exit(1);
    }

  if (data->dimension != codes->dimension) {
    fprintf(stderr, "Data and codebook vectors have different dimensions");
    close_entries(data);
    close_entries(codes);
    exit(1);
  }

  set_teach_params(&params, codes, data, buffer, funcname);
  params.winner = find_winner_knn;
  params.knn = knn;

  compute_knnaccuracy(&params);

  close_entries(data);
  close_entries(codes);

  return(0);
}
