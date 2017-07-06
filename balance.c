/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  balance.c                                                           *
 *  -balances the number of entries in codebook by shortest distances   *
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
#include "lvq_rout.h"
#include "datafile.h"
#include "labels.h"

#define BAL 1.3

static char *usage[] = {
  "balance - balances the number of entries in codebook by shortest distances\n",
  "Required parameters:\n",
  "  -cin filename         input codebook file\n",
  "  -din filename         input data\n",
  "  -cout filename        output codebook filename\n",
  "Optional parameters:\n",
  "  -knn N                use N nearest neighbours\n", 
  "  -rand integer         seed for random number generator. 0 is current time\n",
  "  -selfuncs name        select a set of functions\n",
  NULL};

struct entries *balance_codes(struct teach_params *teach, char *outfile)
{
  long nod, nol, i;
  int label;
  int *noe, *diff, *class;
  int note, knn = teach->knn;
  float aver;
  float *dists;
  struct data_entry *entr, *tlab, tmp, *prev;
  struct entries *red_codes;
  struct entries *codes = teach->codes, *data = teach->data;
  struct mindists *md = NULL;
  DIST_FUNCTION *distance = teach->dist;
  struct hitlist *more;
  WINNER_FUNCTION *find_knn = teach->winner;
  eptr p;

  nol = number_of_labels();

  /* Compute the medians of the shortest distances from each entry to
     its nearest entry of the same class */

  ifverbose(2)
    fprintf(stderr, "Medians of the shortest distances are computed\n");
  md = med_distances(codes, distance);

  /* If serious imbalance exists, add entries to classes where
     distances are large, and remove them from classes where distances
     are small */

  nol = md->num_classes;
  noe = md->noe;
  dists = md->dists;
  class = md->class;

  diff = (int *) oalloc(sizeof(int) * nol);
  for (i = 0; i < nol; i++) {
    diff[i] = 0;
  }

  aver = 0.0;
  note = 0;
  for (i = 0; i < nol; i++) {
    if (noe[i] > 1) {
      aver += dists[i];
      note++;
    }
  }
  aver /= note;

  note = 0;
  ifverbose(2)
    fprintf(stderr, "Medians of different classes are compared\n");
  for (i = 0; i < nol; i++) {
    if ((aver > BAL * dists[i]) && (noe[i] > 1)) {
      diff[i]--;
      note++;
    }
    if (BAL * aver < dists[i]) {
      diff[i]++;
      note--;
    }
  }

  /* Force-pick one codebook vector for each missing class */
  for (i = 0; i < nol; i++) {
    if (noe[i] == 0) {
      entr = force_pick_code(data, i);

      tlab = codes->entries;
      while (tlab->next != NULL)
        tlab = tlab ->next;
      tlab->next = entr;
      codes->num_entries++;
      noe[i] = 1;
      note--;
    }
  }

  /* If there was net increase or decrease in number of entries */
  for (i = 0; i < nol; i++) {
    if ((aver > BAL * dists[i]) && ((noe[i]+diff[i]) > 1)) {
      if (note < 0) {
        diff[i]--;
        note++;
      }
    }
    if (BAL * aver < dists[i]) {
      if (note > 0) {
        diff[i]++;
        note--;
      }
    }
  }

  ifverbose(1)
    fprintf(stderr, "Some codebook vectors are removed\n");
  /* Now remove entries from those classes where diff shows negative */

  entr = codes->entries;
  tmp.next = entr;
  prev = &tmp;
  while (entr != NULL) {

    label = get_entry_label(entr);
    for (i = 0; i < nol; i++)
      if (class[i] == label)
	break;

    if (diff[i] < 0) {
      diff[i]++;
      tlab = entr;
      entr = entr->next;
      prev->next = entr;
      tlab->next = NULL;
      free_entry(tlab);
      codes->num_entries--;
    }
    else
      {
	prev = entr;
	entr = entr->next;
      }
  }

  codes->entries = tmp.next;

  ifverbose(1)
    fprintf(stderr, "Some new codebook vectors are picked\n");
  /* Pick the requested number of additional codes for each class */

  more = new_hitlist();
  for (i = 0; i < nol; i++)
    {
      while (diff[i] > 0)
	{
	  add_hit(more, class[i]); 
	  diff[i]--;
	}
    }
  
  entr = pick_inside_codes(more, data, knn, find_knn);
  free_hitlist(more);

  /* Add new entries to the list */
  tlab = codes->entries;
  while (tlab->next != NULL)
    tlab = tlab->next;
  tlab->next = entr;
  /* laske montako uutta */

  ifverbose(1)
    fprintf(stderr, "Codebook vectors are redistributed\n");

  rewind_entries(data, &p);
  nod = data->num_entries; /* number of data vectors */
  teach->length = nod;
  teach->alpha = 0.3;
  red_codes = olvq1_training(teach, NULL, outfile);
  if (red_codes == NULL)
    return NULL;

  /* Display the medians of the shortest distances */
  ifverbose(2)
    fprintf(stderr, "Medians of the shortest distances are computed\n");

  free_mindists(md); /* free old information */
  md = med_distances(red_codes, distance);
  nol = md->num_classes;
  noe = md->noe;
  dists = md->dists;
  class = md->class;

  for (i = 0; i < nol; i++) {
    if (verbose(1) > 0)
      fprintf(stdout, "In class %9s %3d units, min dist.: %.3f\n",
	     find_conv_to_lab(class[i]), noe[i], dists[i]);
  }

  if (md)
    free_mindists(md);

  return(red_codes);
}

int main(int argc, char **argv)
{
  char *in_data_file;
  char *in_code_file;
  char *out_code_file;
  struct entries *data, *codes;
  struct teach_params params;
  int randomize;
  long buffer = 0;
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
  params.knn = (int) oatoi(extract_parameter(argc, argv, KNN_NEIGHBORS, OPTION), 5);
  randomize = (int) oatoi(extract_parameter(argc, argv, RANDOM, OPTION), 0);
  buffer = oatoi(extract_parameter(argc, argv, "-buffer", OPTION), 0);
  funcname = extract_parameter(argc, argv, "-selfuncs", OPTION);

  ifverbose(2)
    fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
  data = open_entries(in_data_file);

  ifverbose(2)
    fprintf(stderr, "Codebook entries are read from file %s\n", in_code_file);
  codes = open_entries(in_code_file);

  if (data->dimension != codes->dimension) {
    fprintf(stderr, "Data and codes have different dimensions\n");
    close_entries(codes);
    close_entries(data);
    exit(1);
  }

  init_random(randomize);

  set_teach_params(&params, codes, data, buffer, funcname);
  params.winner = find_winner_knn;

  codes = balance_codes(&params, out_code_file);

  ifverbose(2)
    fprintf(stderr, "Codebook entries are saved to file %s\n", out_code_file);
  save_entries(codes, out_code_file);

  return(0);
}

