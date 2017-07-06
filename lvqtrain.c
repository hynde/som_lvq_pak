/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *                                                                      *
 * lvqtrain                                                             *
 *  - train a codebook with one of the following algorithms:            *
 *  lvq1                                                                *
 *   - Learning Vector Quantization Type 1                              *
 *  lvq2                                                                *
 *   - Learning Vector Quantization Type 2                              *
 *  lvq3                                                                *
 *   - Learning Vector Quantization Type 3                              *
 *  olvq1                                                               *
 *   - Optimized-learning-rate LVQ1                                     *
 *                                                                      *
 *  Version 3.1                                                         *
 *  Date: 7 Apr 1995                                                    *
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
#include <math.h>
#include <string.h>
#include "lvq_pak.h"
#include "lvq_rout.h"
#include "datafile.h"

static char *usage[] = {
  "lvqtrain/lvq1/lvq2/lvq3/olvq1 - teach codebook with one of the lvq algorithms\n",
  " Training algorithm is determined from program name and can be overridden\n",
  " with the -type option.\n",
  "Required parameters:\n",
  "  -cin filename         initial codebook file\n",
  "  -din filename         teaching data\n",
  "  -cout filename        output codebook filename\n",
  "  -rlen integer         running length of teaching\n",
  "  -alpha float          initial alpha value (optional with olvq1)\n",
  "  -win float            (lvq2, lvq3) window width\n", 
  "  -epsilon float        (lvq3) training epsilon\n", 
  "Optional parameters:\n",
  "  -type lvqtype         select which lvq algoritm to use: lvq1, lvq2,\n",
  "                        lvq3 or olvq1\n",
  "  -rand integer         seed for random number generator. 0 is current time\n",
  "  -buffer integer       buffered reading of data, integer lines at a time\n",
  "  -alpha_type type      type of alpha decrease, linear (def) or inverse_t.\n",
  
  "  -snapfile filename    snapshot filename\n",
  "  -snapinterval integer interval between snapshots\n",
  "  -selfuncs name        select a set of functions\n",
  NULL};


#define LVQ1  1
#define LVQ2  2
#define LVQ3  3
#define OLVQ1 4

struct typelist lvq_types[] = {
  {LVQ1, "lvq1", lvq1_training},
  {LVQ2, "lvq2", lvq2_training},
  {LVQ3, "lvq3", lvq3_training},
  {OLVQ1, "olvq1", olvq1_training},
  {0, NULL, NULL}};

int main(int argc, char **argv)
{
  char *in_data_file, *in_code_file, *out_code_file;
  char *progname, *typename, *alpha_s, *rand_s;
  char *snapshot_file;
  struct typelist *lvqt, *type_tmp;
  struct entries *data, *codes, *codes2;
  int lvqtype, randomize;
  long buffer, snapshot_interval;
  float	winlen = 0.0, epsilon = 0.0;
  struct teach_params params;
  struct snapshot_info *snap = NULL;
  int snap_type;
  char *funcname = NULL;

  global_options(argc, argv);
  if (extract_parameter(argc, argv, "-help", OPTION2))
    {
      printhelp();
      exit(0);
    }

  /* get program name */
  progname = getprogname();

  typename = extract_parameter(argc, argv, "-type", OPTION);
  if (typename)
    progname = typename;

  /* select which lvq algorithm to use */
  lvqt = get_type_by_str(lvq_types, progname);
  if ((lvqtype = lvqt->id) == 0)
    {
      fprintf(stderr, "Unknown LVQ type %s\n", progname);
      exit(1);
    }
  
  in_data_file = extract_parameter(argc, argv, IN_DATA_FILE, ALWAYS);
  in_code_file = extract_parameter(argc, argv, IN_CODE_FILE, ALWAYS);
  out_code_file = extract_parameter(argc, argv, OUT_CODE_FILE, ALWAYS);
  params.length = oatoi(extract_parameter(argc, argv, RUNNING_LENGTH, ALWAYS),
                        1);
  rand_s = extract_parameter(argc, argv, RANDOM, OPTION);
  randomize = oatoi(rand_s, 0);
  buffer = oatoi(extract_parameter(argc, argv, "-buffer", OPTION), 0);
  alpha_s = extract_parameter(argc, argv, "-alpha_type", OPTION);

  snapshot_file = extract_parameter(argc, argv, "-snapfile", OPTION);
  snapshot_interval = 
    oatoi(extract_parameter(argc, argv, "-snapinterval", OPTION), 0);

  snap_type =
    get_id_by_str(snapshot_list, 
                  extract_parameter(argc, argv, "-snaptype", OPTION));

  if (snapshot_interval)
    {
      if (snapshot_file == NULL)
        {
          snapshot_file = out_code_file;
          fprintf(stderr, "snapshot file not specified, using '%s'", snapshot_file);
        }
      snap = get_snapshot(snapshot_file, snapshot_interval, snap_type);
      if (snap == NULL)
        exit(1);
    }

  switch(lvqtype) 
    {
    case OLVQ1:
      params.alpha = oatof(extract_parameter(argc, argv, TRAINING_ALPHA, OPTION), 0.0);
      break;
    case LVQ2:
      params.alpha = atof(extract_parameter(argc, argv, TRAINING_ALPHA, ALWAYS));
      winlen = atof(extract_parameter(argc, argv, WINDOW_WIDTH, ALWAYS));
      break;
    case LVQ3:
      params.alpha = atof(extract_parameter(argc, argv, TRAINING_ALPHA, ALWAYS));
      epsilon = atof(extract_parameter(argc, argv, TRAINING_EPSILON, ALWAYS));
      winlen = atof(extract_parameter(argc, argv, WINDOW_WIDTH, ALWAYS));
      break;
    case LVQ1:
    default:
      params.alpha = atof(extract_parameter(argc, argv, TRAINING_ALPHA, ALWAYS));
      break;
    }

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
      fprintf(stderr, "Can't open code file '%s'\n", in_data_file);
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

  init_random(randomize);
  if (rand_s)
    data->flags.random_order = 1;

  /* alpha type */

  if (alpha_s)
    {
      type_tmp = get_type_by_str(alpha_list, alpha_s);
      if (type_tmp->data == NULL)
        {
          fprintf(stderr, "Unknown alpha type %s\n", alpha_s);
	  close_entries(data);
	  close_entries(codes);
	  exit(1);
        }
    }
  else
    type_tmp = get_type_by_id(alpha_list, ALPHA_LINEAR);

  params.alpha_type = type_tmp->id;
  params.alpha_func = type_tmp->data;
  params.snapshot = snap;

  switch (lvqtype)
    {
    case LVQ1:
      codes2 = lvq1_training(&params);
      break;
    case OLVQ1:
      codes2 = olvq1_training(&params, in_code_file,
			      out_code_file);
      break;
    case LVQ2:
      params.winner = find_winner_knn;
      codes2 = lvq2_training(&params, winlen);
      break;
    case LVQ3:
      params.winner = find_winner_knn;
      codes2 = lvq3_training(&params, epsilon, winlen);
      break;
    default:
      fprintf(stderr, "Unknown LVQ type?!?!?\n");
      close_entries(data);
      close_entries(codes);
      exit(1);
      break;
    }
  if (codes2 == NULL)
    {
      fprintf(stderr, "Teaching failed\n");
      close_entries(data);
      close_entries(codes);
      exit(1);
    }

  ifverbose(2)
    fprintf(stdout, "Codebook entries are saved to file %s\n", out_code_file);
  save_entries(codes, out_code_file);
  invalidate_alphafile(out_code_file);

  close_entries(data);
  close_entries(codes);

  return(0);
}
