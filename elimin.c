/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  elimin.c                                                            *
 *   - eliminates those entries that are incorrectly classified by knn  *
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
#include "labels.h"
#include "lvq_rout.h"

#define KNN 10

static char *usage[] = {
  "elimin - eliminates those entries that are incorrectly classified by knn\n",
  "Required parameters:\n",
  "  -din filename         input data\n",
  "  -cout filename        output codebook filename\n",
  "Optional parameters:\n",
  "  -knn N                use N nearest neighbours (default: 5)\n", 
  NULL};


struct entries *eliminate_codes(int knn, struct entries *data, WINNER_FUNCTION *find_knn)
{
  long j, noe, correct, incorrect;
  int datalabel;
  struct data_entry tmp, *loca, *prev;
  struct entries *datac;
  struct winner_info *win;
  eptr p;

  if (knn > KNN) {
    fprintf(stderr, "Can use only %d neighbors", KNN);
    knn = KNN;
  }

  if ((datac = copy_entries(data)) == NULL)
    {
      return NULL;
    }
      
  if ((win = calloc(knn, sizeof(struct winner_info))) == NULL)
    {
      close_entries(datac);
      return NULL;
    }

  /* Count the number of entries in data set */

  /* Those entries are removed from datac that are not
     correctly classified using the knn-classification */
  loca = rewind_entries(data, &p);

  noe = data->flags.totlen_known ? data->num_entries : 0;

  prev = &tmp;
  tmp.next = NULL;
  while (loca != NULL) {
    correct = 0; 
    incorrect = 0;

    if (find_knn(data, loca, win, knn) != knn)
      {
	/* did not find winners */
      }
    else
      {
	datalabel = get_entry_label(loca);

	/* Count the correctly classified items against
	   the incorrectly classified ones */
	for (j = 0; j < knn; j++) {
	  if (get_entry_label(win[j].winner) == datalabel) 
	    correct++;
	  else
	    incorrect++;
	}
	
	/* The entry is saved only if there are more correct hits
	   than there are incorrect ones  (one hit is for sure because
	   the sample is compared to itself also)                            */
	if (correct > incorrect) {
	  prev->next = copy_entry(datac, loca);
	  prev = prev->next;
	  datac->num_entries++;
	  datac->num_loaded++;
	}
      }
    /* Consider the next entry */
    loca = next_entry(&p);

    ifverbose(1)
      if (noe)
	mprint((long) --noe);

  }

  ifverbose(1)
    {
      mprint(0);
      fprintf(stderr, "\n");
    }
  
  datac->entries = tmp.next;
  
  return(datac);
}

int main(int argc, char **argv)
{
  int knn;
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
  knn = (int) oatoi(extract_parameter(argc, argv, KNN_NEIGHBORS, OPTION), 5);

  ifverbose(2)
    fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
  if ((data = open_entries(in_data_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file '%s'\n", in_data_file);
      exit(1);
    }

  ifverbose(2)
    fprintf(stderr, "Extra codes are eliminated\n");
  codes = eliminate_codes(knn, data, find_winner_knn);
  close_entries(data);
  if (codes == NULL)
    {
      fprintf(stderr, "Elimination failed!\n");
      exit(1);
    }
      
  ifverbose(2)
    fprintf(stderr, "Codebook entries are saved to file %s\n", out_code_file);
  save_entries(codes, out_code_file);
  invalidate_alphafile(out_code_file);

  return(0);
}
