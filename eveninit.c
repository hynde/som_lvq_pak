/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  eveninit.c                                                          *
 *  -initializes the codebook entries evenly to all classes (eveninit)  *
 *   or with their numbers proportional to the data file (propinit)     *
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
#include <string.h>
#include "lvq_pak.h"
#include "lvq_rout.h"
#include "datafile.h"

static char *usage[] = {
  "eveninit/propinit - initializes the codebook. Eveninit initializes\n",
  " entries evenly to all classes, propinit with their numbers proportional\n",
  " to the data file. Initialzation type is determined from program name\n",
  " (eveninit or propinit) or is selected with the -type option.\n",
  "Required parameters:\n",
  "  -din filename         input data\n",
  "  -cout filename        output codebook filename\n",
  "  -noc integer          number of codebook vectors\n",
  "Optional parameters:\n",
  "  -init type            initialization type, eveninit or propinit. Overrides\n",
  "                        the type determined from the program name\n",
  "  -knn N                use N nearest neighbours\n", 
  "  -rand integer         seed for random number generator. 0 is current time\n",
  NULL};

struct entries *init_codes(long number_of_codes, struct entries *data,
			   int knn, int prop, WINNER_FUNCTION *find_knn)
{
  long nol, tot, nic, emp, nom;
  struct data_entry *entr, *entr2, *temp, *d;
  struct entries *codebook;
  float frac, err;
  struct hitlist *classes;
  struct hit_entry *class;
  eptr p;

  if ((codebook = copy_entries(data)) == NULL)
    return NULL;

  codebook->topol = TOPOL_LVQ;

  if ((classes = new_hitlist()) == NULL)
    return NULL;

  /* count number of classes in data */

  for (d = rewind_entries(data, &p); d != NULL; d = next_entry(&p))
    add_hit(classes, get_entry_label(d));

  nol = classes->entries;
  tot = data->num_entries;

  if (nol > number_of_codes) {
    fprintf(stderr, "There are more different classes than requested codes");
  }

  /* how many entries for each class */
  nic = number_of_codes / nol;

  ifverbose(2)
    fprintf(stderr, "The codebook vectors for each class are picked\n");

  /* pick even number of codebook vectors for each class */
  for (class = classes->head; class != NULL; class = class->next)
    if (prop)
      {
	class->freq = class->freq * (float) number_of_codes / tot;
	if (class->freq < 1) 
	  class->freq = 1;
      }
    else
      class->freq = nic;

  entr = pick_inside_codes(classes, data, knn, find_knn);

  /* check if all required codebook vectors were found */
  emp = 0;
  for (class = classes->head, emp = 0; class != NULL; class = class->next)
    if (class->freq == 0)
      emp++;

  ifverbose(2)
    fprintf(stderr, "For %ld classes all found\n", emp);

  temp = entr;
  nom = 0;
  while (temp != NULL) {
    nom++;
    temp = temp->next;
  }
  ifverbose(2)
    fprintf(stderr, "Found %ld vectors in first pass\n", nom);

  if (nom < number_of_codes) {
    frac = 0.0;
    err = 0.0;
    if (emp != 0)
      frac = (number_of_codes - nom) / (float) emp;

    for (class = classes->head; class != NULL; class = class->next)
      {
	if (class->freq == 0) 
	  {
	    class->freq = (int) (frac + err);
	    err = frac + err - class->freq;
	  }
	else 
	  class->freq = 0;
      }
    
    /* pick more codes from those classes where you got all */
    entr2 = pick_inside_codes(classes, data, knn, find_knn);
    
    temp = entr;
    if (temp != NULL) {
      while (temp->next != NULL) {
        temp = temp->next;
      }
      temp->next = entr2;
    }
    else {
      entr = entr2;
    }
  }

  free_hitlist(classes);
  codebook->entries = entr;

  temp = entr;
  while (temp != NULL) {
    codebook->num_entries++;
    temp = temp->next;
  }
  codebook->num_loaded = codebook->num_entries;
  codebook->flags.totlen_known = 1;

  return(codebook);
}

int main(int argc, char **argv)
{
  long number_of_codes;
  int knn;
  int randomize;
  char *in_data_file;
  char *out_code_file;
  char *progname, *pname;
  int prop = -1;
  struct entries *data, *codes;

  global_options(argc, argv);
  if (extract_parameter(argc, argv, "-help", OPTION2))
    {
      printhelp();
      exit(0);
    }

  /* get program name */
  progname = getprogname();

  if (strcasecmp(progname, "propinit") == 0)
    prop = 1;
  else if (strcasecmp(progname, "eveninit") == 0)
    prop = 0;

  pname = extract_parameter(argc, argv, "-type", (prop < 0) ? ALWAYS : OPTION); 
  if (pname)
    {
      if (strcasecmp(pname, "propinit") == 0)
	prop = 1;
      else if (strcasecmp(pname, "eveninit") == 0)
	prop = 0;
    }

  if (prop < 0)
    {
      fprintf(stderr, "unknown init type\n");
      exit(1);
    }
  
  in_data_file = extract_parameter(argc, argv, IN_DATA_FILE, ALWAYS);
  out_code_file = extract_parameter(argc, argv, OUT_CODE_FILE, ALWAYS);
  number_of_codes = (int) oatoi(extract_parameter(argc, argv,
				NUMBER_OF_CODES, ALWAYS), 1);
  knn = (int) oatoi(extract_parameter(argc, argv, KNN_NEIGHBORS, OPTION), 5);
  randomize = (int) oatoi(extract_parameter(argc, argv, RANDOM, OPTION), 0);

  ifverbose(2)
    fprintf(stderr, "Input entries are read from file %s\n", in_data_file);
  if ((data = open_entries(in_data_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file '%s'\n", in_data_file);
      exit(1);
    }

  init_random(randomize);

  codes = init_codes(number_of_codes, data, knn, prop, find_winner_knn);
  if (codes == NULL)
    {
      fprintf(stderr, "Failed to initialize codes\n");
      exit(1);
    }

  ifverbose(2)
    fprintf(stderr, "Codebook entries are saved to file %s\n", out_code_file);

  save_entries(codes, out_code_file);
  invalidate_alphafile(out_code_file);

  close_entries(codes);

  return(0);
}
