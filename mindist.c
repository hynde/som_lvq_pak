/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  mindist.c                                                           *
 *  - displays the medians of the shortest distances in each class      *
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
#include <math.h>
#include "lvq_pak.h"
#include "lvq_rout.h"
#include "datafile.h"

#define BAL 1.3

static char *usage[] = {
  "mindist - displays the medians of the shortest distances in each class\n",
  "Required parameters:\n",
  "  -cin filename         codebook file\n",
  "  -din filename         data file\n",
  "Optional parameters:\n",
  "  -buffer integer       buffered reading of data, integer lines at a time\n",
  NULL};

int main(int argc, char **argv)
{
  int nol, i, buffer;
  char *in_code_file;
  char *in_data_file;
  struct entries *codes;
  struct entries *data;
  struct mindists *md = NULL;

  global_options(argc, argv);
  if (extract_parameter(argc, argv, "-help", OPTION2))
    {
      printhelp();
      exit(0);
    }
  in_code_file = extract_parameter(argc, argv, IN_CODE_FILE, ALWAYS);
  in_data_file = extract_parameter(argc, argv, IN_DATA_FILE, OPTION);
  buffer = oatoi(extract_parameter(argc, argv, "-buffer", OPTION), 0);

  ifverbose(2)
    fprintf(stderr, "Codebook entries are read from file %s\n", in_code_file);
  if ((codes = open_entries(in_code_file)) == NULL)
    {
      fprintf(stderr, "Can't read code file '%s'\n", in_code_file);
      exit(1);
    }

  data = NULL;
  if (in_data_file != (char *) NULL) {
    ifverbose(2)
      fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
    if ((data = open_entries(in_data_file)) == NULL)
      {
	fprintf(stderr, "Can't read code file '%s'\n", in_code_file);
	close_entries(codes);
	exit(1);
      }
    set_buffer(data, buffer);
  }

  ifverbose(2)
    fprintf(stderr, "The medians of the shortest distances are computed\n");
  md = med_distances(codes, NULL);
#if 1
  if (data) {
    ifverbose(2)
      fprintf(stderr, "The standard deviations are computed\n");
    if (deviations(data, md) == NULL)
      {
	fprintf(stderr, "can't calculate deviations\n");
      }
  }
#endif

  nol = md->num_classes;
  for (i = 0; i < nol; i++) {
    fprintf(stdout, "In class %9s %3d units, min dist.: %6.3f",
          find_conv_to_lab(md->class[i]), md->noe[i], md->dists[i]);
#if 1
    if (md->devs != NULL) {
      fprintf(stdout, ", stand. dev.: %6.3f \n",md->devs[i]);
    }
    else
#endif
      fprintf(stdout, "\n");
  }

  if (codes)
    close_entries(codes);
  if (data)
    close_entries(data);
  if (md)
    free_mindists(md);

  return(0);
}


