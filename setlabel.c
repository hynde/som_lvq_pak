/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  setlabel.c                                                          *
 *  -sets the labels of entries by the majority voting                  *
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
  "setlabel - sets the labels of codebook entries by the majority voting\n",
  "Required parameters:\n",
  "  -cin filename         input codebook file\n",
  "  -din filename         input data\n",
  "  -cout filename        output codebook filename\n",
  "Optional parameters:\n",
  "  -knn N                use N nearest neighbours (default: 5)\n", 
  "  -buffer integer       buffered reading of data, integer lines at a time\n",
  "  -selfuncs name        select a set of functions\n",
  NULL};


struct entries *find_labels(struct teach_params *teach)
{
  long noc, i;
  struct data_entry *codetmp;
  WINNER_FUNCTION *find_winners = teach->winner;
  struct entries *data = teach->data;
  struct entries *codes = teach->codes;
  int knn = teach->knn;
  struct winner_info *winners;
  struct hitlist *hits;
  eptr p;

  if ((hits = new_hitlist()) == NULL)
    {
      fprintf(stderr, "can't create hitlist\n");
      return NULL;
    }

  winners = calloc(knn, sizeof(struct winner_info));

  if (knn < 1) 
    knn = 1;

  /* Scan all codebook entries */
  codetmp = rewind_entries(codes, &p);
  noc = codes->num_entries;

  while (codetmp != NULL) {

    clear_hitlist(hits);

    /* Find best matching data vectors */
    find_winners(data, codetmp, winners, knn);

    /* Find the majority of votes */
    for (i = 0; i < knn; i++)
      add_hit(hits, get_entry_label(winners[i].winner));

    set_entry_label(codetmp, hits->head->label);

    /* Take the next code entry */
    codetmp = next_entry(&p);

    ifverbose(1)
      mprint((long) noc--);
  }
  ifverbose(1)
    {
      mprint((long) 0);
      fprintf(stderr, "\n");
    }

  free_hitlist(hits);

  return(codes);
}

int main(int argc, char **argv)
{
  int knn;
  char *in_data_file;
  char *in_code_file;
  char *out_code_file;
  struct entries *data, *codes;
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
  out_code_file = extract_parameter(argc, argv, OUT_CODE_FILE, ALWAYS);
  knn = oatoi(extract_parameter(argc, argv, KNN_NEIGHBORS, OPTION), 5);
  buffer = oatoi(extract_parameter(argc, argv, "-buffer", OPTION), 0);
  funcname = extract_parameter(argc, argv, "-selfuncs", OPTION);

  ifverbose(2)
    fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
  if ((data = open_entries(in_data_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file '%s'\n", in_data_file);
      exit(1);
    }

  label_not_needed(1);

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

  set_teach_params(&params, codes, data, 0, funcname); 
  params.knn = knn;
  params.winner = find_winner_knn;
  codes = find_labels(&params);

  ifverbose(2)
    fprintf(stderr, "Codebook entries are saved to file %s\n", out_code_file);
  save_entries(codes, out_code_file);

  close_entries(data);
  close_entries(codes);

  return(0);
}
