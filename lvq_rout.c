/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  lvq_rout.c                                                          *
 *  -routines needed in some programs                                   *
 *    -training routines                                                *
 *    -classification routines                                          *
 *    -etc.                                                             *
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
#include <stdlib.h>
#include "lvq_pak.h"
#include "lvq_rout.h"
#include "datafile.h"

/* Check whether the vector 'code' (codebook vector) is correctly
   classified by knn-classification with respect to the codebook
   'data'.  Return 1 if correct, 0 if incorrect, -1 on error */

int correct_by_knn(struct entries *data, struct data_entry *code, int knn, 
		   WINNER_FUNCTION *find_knn)
{
  int corr = 0;
  int i;
  int codelabel;
  struct winner_info *winners;
  struct hitlist *hits;

  hits = new_hitlist();
  if (hits == NULL)
    return -1;

  if (knn < 1) knn = 1;
  winners = malloc(sizeof(struct winner_info) * knn);
  if (winners == NULL)
    {
      perror("correct_by_knn");
      free_hitlist(hits);
      return -1;
    }

  /* Find nearest neighbours. Note: data is codebook, code is sample */
  if (find_knn(data, code, winners, knn) != knn)
    {
      fprintf(stderr, "correct_by_knn: can't find winners\n");
      corr = -1;
      goto end;
    }

  for (i = 0; i < knn; i++) 
    add_hit(hits, get_entry_label(winners[i].winner));

  codelabel = get_entry_label(code);

  if (hits->head)
    if (hits->head->label == codelabel)
      corr = 1;
 end:
  free_hitlist(hits);
  free(winners);
  return(corr);
}


/* Pick a given number of entries from the beginning of the entry list */

struct entries *pick_codes(int num, struct entries *data)
{
  struct entries *datac;
  struct data_entry *prev, *loca, tmp;
  eptr p;

  datac = copy_entries(data);
  if (datac == NULL)
    {
      fprintf(stderr, "extract_codes: can't copy entries structure\n");
      return NULL;
    }
  /* Pick (at most) 'num' entries from beginning */

  if ((loca = rewind_entries(data, &p)) == NULL)
    {
      fprintf(stderr, "pick_codes: can't get data\n");
      close_entries(datac);
      return NULL;
    }
  prev = &tmp;
  tmp.next = NULL;
  while ((loca != NULL) && (num--)) {
    prev->next = copy_entry(data, loca);
    prev = prev->next;
    datac->num_entries++;
    loca = next_entry(&p);
  }

  datac->entries = tmp.next;
  datac->num_loaded = datac->num_entries;
  datac->flags.totlen_known = 1;

  return(datac);
}

/* Pick a given number of entries of given class from entry list */

struct data_entry *pick_known_codes(int num, struct entries *data, int label)
{
  struct data_entry *prev, *loca, tmp;
  eptr p;

  if ((loca = rewind_entries(data, &p)) == NULL)
    {
      fprintf(stderr, "pick_known_codes: can't get data\n");
      return NULL;
    }
  prev = &tmp;
  tmp.next = NULL;
  while ((loca != NULL) && (num)) {
    if (get_entry_label(loca) == label) {
      prev->next = copy_entry(data, loca);
      prev = prev->next;
      num--;
    }
    loca = next_entry(&p);
  }

  return(tmp.next);
}

/* Pick a given number of entries of each class from entry list. The
   numbers of entries are given in an array. The selected entries
   should fall inside class borders */

struct data_entry *pick_inside_codes(struct hitlist *classes, 
				     struct entries *data, int knn, 
				     WINNER_FUNCTION *find_knn)
{
  int still, datalabel;
  long total = 0;
  struct data_entry *prev, *loca, tmp;
  struct hit_entry *class;
  eptr p;

  /* Pick (at most) 'topick' entries from the beginning of each class
     so that they are also correctly classified by KNN */

  for (class = classes->head, total = 0; class != NULL; class = class->next) 
    total += class->freq;

  if ((loca = rewind_entries(data, &p)) == NULL)
    {
      fprintf(stderr, "pick_inside_codes: can't get data\n");
      return NULL;
    }
  prev = &tmp;
  tmp.next = NULL;
  still = 1;

  ifverbose(1)
    mprint((long) total);

  while ((total) && (loca != NULL)) {
    datalabel = get_entry_label(loca);
    class = find_hit(classes, datalabel);
    if (class)
      if (class->freq > 0) {
	/* test if it is correctly classified */
	if (correct_by_knn(data, loca, knn, find_knn))
	  { 
	    ifverbose(1)
	      mprint((long) total);
	    total--;
	    prev->next = copy_entry(data, loca);
	    if (prev->next == NULL)
	      {
		fprintf(stderr, "pick_inside_codes: can't copy entry\n");
		return tmp.next;
	      }
	    prev = prev->next;
	    
	    class->freq--;
	  }
      }
    loca = next_entry(&p);
  }
  
  ifverbose(1)
    {
      mprint((long) 0);
      fprintf(stderr, "\n");
    }

  return(tmp.next);
}

/* Pick one entry from a given class. */

struct data_entry *force_pick_code(struct entries *data, int ind)
{
  struct data_entry *prev, *loca, tmp;
  eptr p;

  /* pick one entry from a given class */

  if ((loca = rewind_entries(data, &p)) == NULL)
    {
      fprintf(stderr, "force_pick_codes: can't get data\n");
      return NULL;
    }
  prev = &tmp;
  tmp.next = NULL;
  while (loca != NULL) {
    if (get_entry_label(loca) == ind) {
      prev->next = copy_entry(data, loca);
      prev = prev->next;
      break;
    }
    loca = next_entry(&p);
  }

  return(tmp.next);
}

struct mindists *alloc_mindists(void)
{
  struct mindists *md;

  if ((md = malloc(sizeof(struct mindists))) == NULL)
    return NULL;
  md->num_classes = 0;
  md->class = NULL;
  md->dists = NULL;
  md->devs = NULL;
  md->noe = NULL;
  if ((md->classes = new_hitlist()) == NULL)
    {
      free(md);
      return NULL;
    }
  return md;
}

void free_mindists(struct mindists *md)
{
  if (md)
    {
      if (md->classes)
	free_hitlist(md->classes);
      if (md->class)
	free(md->class);
      if (md->dists)
	free(md->dists);
      if (md->devs)
	free(md->devs);
      if (md->noe)
	free(md->noe);
      free(md);
    }
}

/* Compute the average shortest distances */

struct mindists *min_distances(struct entries *codes, DIST_FUNCTION *distance)
{

  long nol, i;
  int note, fou, dim;
  float *dists;
  int *class, *noe;
  float dissf, dist;
  struct data_entry *entr, *ensu, *d;
  eptr p, p2;
  struct hitlist *classes;
  struct hit_entry *h;
  struct mindists *md;

  if (distance == NULL)
    distance = vector_dist_euc;

  if ((md = alloc_mindists()) == NULL)
    return NULL;
  classes = md->classes;
  
  for (d = rewind_entries(codes, &p); d != NULL; d = next_entry(&p))
    add_hit(classes, get_entry_label(d));

  nol = classes->entries; /* number of classes */
  md->num_classes = nol;

  class = calloc(nol, sizeof(int));
  if ((md->class = class) == NULL)
    {
      free_mindists(md);
      return NULL;
    }
  noe = calloc(nol, sizeof(int));
  if ((md->noe = noe) == NULL)
    {
      free_mindists(md);
      return NULL;
    }
  dists = calloc(nol, sizeof(float));
  if ((md->dists = dists) == NULL)
    {
      free_mindists(md);
      return NULL;
    }
  
  dim = codes->dimension;

  for (i = 0, h = classes->head; i < nol; i++, h = h->next) 
    {
      dists[i] = 0.0;
      class[i] = h->label;
      noe[i] = h->freq;
      entr = rewind_entries(codes, &p);
      p2.parent = p.parent;
      note = 0;
      while (entr != NULL) 
	{
	  if (get_entry_label(entr) == class[i]) {
	    p2.current = p.current;
	    p2.index = p.index;
	    ensu = next_entry(&p2);
	    dissf = FLT_MAX;
	    fou = 0;
	    while (ensu != NULL)
	      {
		if (get_entry_label(ensu) == class[i])
		  {
		    fou = 1;
		    dist = distance(ensu, entr, dim);
		    if (dist < dissf)
		      dissf = dist;
		    
		  }
		ensu = next_entry(&p2);
	      }
	    if (fou) 
	      {
		dists[i] += dissf;
		note++;
	      }
	  }
	  entr = next_entry(&p);
	}
      if (note > 0)
	dists[i] /= note;
    }
  
  return(md);
}

/* Comparison routine for the distances (used in qsort) */

int compar(const void *a, const void *b)
{
  if (*(float *)a < *(float *)b)
    return(-1);
  if (*(float *)a > *(float *)b)
    return(1);
  return(0);
}

/* Compute the median of shortest distances */

struct mindists *med_distances(struct entries *codes, DIST_FUNCTION *distance)
{
  long i, nol;
  int not, mnoe, fou, dim;
  int *class, *noe;
  float *dists;
  float dissf, dist;
  float *meds;
  struct data_entry *entr, *ensu, *d;
  struct hitlist *classes;
  struct hit_entry *h;
  struct mindists *md;
  eptr p, p2;

  dim = codes->dimension;

  if (distance == NULL)
    distance = vector_dist_euc;

  if ((md = alloc_mindists()) == NULL)
    return NULL;
  classes = md->classes;
  
  /* find out number of labels in each class */
  for (d = rewind_entries(codes, &p); d != NULL; d = next_entry(&p))
    add_hit(classes, get_entry_label(d));

  nol = classes->entries; /* number of classes */
  md->num_classes = nol;

  class = calloc(nol, sizeof(int));
  if ((md->class = class) == NULL)
    {
      free_mindists(md);
      return NULL;
    }
  noe = calloc(nol, sizeof(int));
  if ((md->noe = noe) == NULL)
    {
      free_mindists(md);
      return NULL;
    }
  dists = calloc(nol, sizeof(float));
  if ((md->dists = dists) == NULL)
    {
      free_mindists(md);
      return NULL;
    }
#if 0
  devs = calloc(nol, sizeof(float));
  if ((md->devs = devs) == NULL)
    {
      free_mindists(md);
      return NULL;
    }
#endif

  for (i = 0; i < nol; i++)
    dists[i] = 0.0;

  /* Find the max number of entries in one class */
  mnoe = classes->head->freq;

  /* Allocate space for the distances (to find the median) */
  meds = (float *) oalloc(sizeof(float) * mnoe);
  
  for (i = 0, h = classes->head; i < nol; i++, h = h->next) 
    {
      dists[i] = 0.0;
      class[i] = h->label;
      noe[i] = h->freq;
      not = 0;
      entr = rewind_entries(codes, &p);
      p2.parent = p.parent;
      while (entr != NULL) {
	if (get_entry_label(entr) == class[i]) {
	  p2.current = p.current;
	  p2.index = p.index;
	  ensu = next_entry(&p2);
	  dissf = FLT_MAX;
	  fou = 0;
	  while (ensu != NULL) {
	    if (get_entry_label(ensu) == class[i])
	      {
		dist = 0.0;
		fou = 1;
		dist = distance(ensu, entr, dim);
		if (dist < dissf) {
		  dissf = dist;
		}
	      }
	    ensu = next_entry(&p2);
	  }
	  if (fou)
	    meds[not++] = dissf;
	}
	entr = next_entry(&p);
      }
      if (not > 0) {
	/* find the median */
	qsort((void *) meds, not, sizeof(float), compar);
	dists[i] = meds[not/2];
      }
    }
  
  ofree(meds);

  return(md);
}

/* Train by lvq1; the nearest codebook vector is modified.  If
   classification is correct, move it towards the input entry; if
   classification is incorrect, move it away from the input entry */

struct entries *lvq1_training(struct teach_params *teach)
{

  long le, total_length, length = teach->length;
  int label, dim;
  int numofe;
  float shortest;
  float talpha;
  struct data_entry *datatmp, *best;
  struct winner_info win;
  VECTOR_ADAPT *adapt_vector = teach->vector_adapt;
  WINNER_FUNCTION *find_winner = teach->winner;
  ALPHA_FUNC *get_alpha = teach->alpha_func;
  struct entries *data = teach->data;
  struct entries *codes = teach->codes;
  struct snapshot_info *snap = teach->snapshot;
  float alpha = teach->alpha;
  eptr p;

  total_length = length;
  
  dim = codes->dimension;

  if ((datatmp = rewind_entries(data, &p)) == NULL)
    {
      fprintf(stderr, "lvq1_training: can't get data\n");
      return NULL;
    }

  numofe = data->flags.totlen_known ? data->num_entries : 0;

  for (le = 0; le < length; le++, datatmp = next_entry(&p))
    {
      if (datatmp == NULL)
	{
	  datatmp = rewind_entries(data, &p);
	  if (datatmp == NULL)
	    {
	      fprintf(stderr, "lvq1_training: can't rewind data (%ld/%ld iterations)\n", 
		      le, length);
	      return NULL;
	    }
	}

      find_winner(codes, datatmp, &win, 1);
      shortest = win.diff;
      best = win.winner;
      label = get_entry_label(best);
      
      talpha = get_alpha(le, length, alpha);
      
      /* Was the classification correct? If classification was
         correct; move towards, else move away */

      if (label == get_entry_label(datatmp))
	adapt_vector(best, datatmp, dim, talpha);
      else 
	adapt_vector(best, datatmp, dim, -talpha);


    /* save snapshot when needed */
    if ((snap) && ((le % snap->interval) == 0) && (le > 0))
      {
        ifverbose(3)
	  fprintf(stderr, "Saving snapshot, %ld iterations\n", le);
        if (save_snapshot(teach, le))
          {
            fprintf(stderr, "snapshot failed\n");
          }
      }

      
      ifverbose(1) 
	mprint(length - le);
    }
  ifverbose(1) 
    fprintf(stderr, "\n");
  
  return(codes);
}

/* Train by olvq1, whereby optimized alpha values are used.  The
   nearest code vector is modified; If classification is correct, move
   it towards the input entry, if classification is incorrect, move it
   away from the input entry */

struct entries *olvq1_training(struct teach_params *teach, 
			       char *infile, char *outfile)
{
  long i, le, noc, length = teach->length;
  int label;
  int numofe;
  int potobe, dim;
  float shortest;
  float *talpha;
  struct data_entry *datatmp, *best;
  VECTOR_ADAPT *adapt = teach->vector_adapt;
  WINNER_FUNCTION *find_winner = teach->winner;
  /* ALPHA_FUNC *get_alpha = teach->alpha_func; */
  struct entries *data = teach->data;
  struct entries *codes = teach->codes;
  struct snapshot_info *snap = teach->snapshot;
  float alpha = teach->alpha;
  struct winner_info win;
  eptr p;

  dim = codes->dimension;

  rewind_entries(codes, &p); /* make sure codes are loaded */

  noc = codes->num_entries;

  /* Get the alpha values:                                      */
  /* There are several possibilities: the user may define them; */
  /* if not, then they may be read from file;                   */
  /* if no file exists, then default values are used.           */ 
  talpha = (float *) oalloc(sizeof(float) * noc);
  if (alpha == 0.0) {
    if (!alpha_read(talpha, noc, infile)) {
      alpha = 0.3;
      for (i = 0; i < noc; i++) {
	talpha[i] = alpha;
      }
    }
  }
  else {
    for (i = 0; i < noc; i++) {
      talpha[i] = alpha;
    }
  }

  if ((datatmp = rewind_entries(data, &p)) == NULL)
    {
      fprintf(stderr, "olvq1_training: can't get data\n");
      return NULL;
    }

  numofe = data->flags.totlen_known ? data->num_entries : 0;

  for (le = 0; le < length; le++, datatmp = next_entry(&p))
    {
      if (datatmp == NULL)
	{
	  datatmp = rewind_entries(data, &p);
	  if (datatmp == NULL)
	    {
	      fprintf(stderr, "olvq1_training: can't rewind data (%ld/%ld iterations)\n", 
		      le, length);
	      return NULL;
	    }
	}
      
      find_winner(codes, datatmp, &win, 1);
      shortest = win.diff;
      best = win.winner;
      label = get_entry_label(best);
      potobe = win.index;
      
      /* Individual alphas for every codebook vector; */
      /* Was the classification correct?              */
      if (label == get_entry_label(datatmp)) {
	/* If classification was correct, move towards */

	adapt(best, datatmp, dim, talpha[potobe]);

	talpha[potobe] = talpha[potobe] / (1 + talpha[potobe]);
      }
      else {
	/* If classification was incorrect, move away */

	adapt(best, datatmp, dim, -talpha[potobe]);

	talpha[potobe] = talpha[potobe] / (1 - talpha[potobe]);
	if (talpha[potobe] > alpha)
	  talpha[potobe] = alpha;
      }

    /* save snapshot when needed */
    if ((snap) && ((le % snap->interval) == 0) && (le > 0))
      {
        ifverbose(3)
	  fprintf(stderr, "Saving snapshot, %ld iterations\n", le);
        if (save_snapshot(teach, le))
          {
            fprintf(stderr, "snapshot failed\n");
          }
      }

      
      ifverbose(1)
	mprint(length - le);
    }
  ifverbose(1)
    fprintf(stderr, "\n");
  
  /* Store the alphas */
  alpha_write(talpha, noc, outfile);
  
  return(codes);
}

/* Train by lvq2.  Two nearest codebook vectors are modified under
   specified conditions */

struct entries *lvq2_training(struct teach_params *teach, float winlen)
{

  long le, total_length, numofe;
  int label, nlabel, datalabel, dim;
  float shortest, nshortest;
  float talpha;
  struct data_entry *datatmp;
  struct data_entry *best, *nbest, *ntmp;
  VECTOR_ADAPT *adapt = teach->vector_adapt;
  WINNER_FUNCTION *find_winners = teach->winner;
  ALPHA_FUNC *get_alpha = teach->alpha_func;
  struct entries *data = teach->data;
  struct entries *codes = teach->codes;
  long length = teach->length;
  float alpha = teach->alpha;
  struct snapshot_info *snap = teach->snapshot;
  struct winner_info win[2];
  eptr p;

  dim = codes->dimension;

  total_length = length;

  if ((datatmp = rewind_entries(data, &p)) == NULL)
    {
      fprintf(stderr, "lvq2_training: can't get data\n");
      return NULL;
    }
  numofe = data->flags.totlen_known ? data->num_entries : 0;

  for (le = 0; le < length; le++, datatmp = next_entry(&p))
    {
      if (datatmp == NULL)
	{
	  datatmp = rewind_entries(data, &p);
	  if (datatmp == NULL)
	    {
	      fprintf(stderr, "lvq2_training: can't rewind data (%ld/%ld iterations)\n", 
		      le, length);
	      return NULL;
	    }
	}
      
      /* True alpha is decreasing linearly during the training */
      talpha = get_alpha(le, length, alpha);
      
      /* find two best mathing units */
      find_winners(codes, datatmp, win, 2);
      
      shortest = win[0].diff;
      best = win[0].winner;
      label = get_entry_label(best);
      
      nshortest = win[1].diff;
      nbest = win[1].winner;
      nlabel = get_entry_label(nbest);
      
      datalabel = get_entry_label(datatmp);
      /* Corrections are made only if the two nearest codebook vectors
	 belong to different classes, one of them correct, and if the
	 input entry is located inside a window between the nearest codebook
	 vectors           */
      if (label != nlabel) {
	if ((label == datalabel) || (nlabel == datalabel)) {
	  
	  /* If the ration of distances to the two nearest codebook vectors
	     satisfies a condition */
	  if ((shortest/nshortest) > ((1-winlen)/(1+winlen))) {
	    /* If the second best is correct swap the entries */
	    if (nlabel == datalabel) {
	      ntmp = best;
	      best = nbest;
	      nbest = ntmp;
	    }
	    
	    /* Move the entries */
	    adapt(best, datatmp, dim, talpha);
	    adapt(nbest, datatmp, dim, -talpha);
	  }
	}
      }
      
      /* save snapshot when needed */
      if ((snap) && ((le % snap->interval) == 0) && (le > 0))
	{
	  ifverbose(3)
	    fprintf(stderr, "Saving snapshot, %ld iterations\n", le);
	  if (save_snapshot(teach, le))
	    {
	      fprintf(stderr, "snapshot failed\n");
	    }
	}
      
      ifverbose(1)
	mprint(length - le);
    }
  ifverbose(1)
    fprintf(stderr, "\n");

  return(codes);
}

/* Train by lvq3. Two nearest codebook vectors are modified under
   specified conditions */

struct entries *lvq3_training(struct teach_params *teach,
			      float epsilon, float winlen)
{
  long le, numofe, total_length;
  int label, nlabel, datalabel, dim;
  float shortest, nshortest;
  float talpha;
  struct data_entry *datatmp, *best, *nbest, *ntmp;
  VECTOR_ADAPT *adapt = teach->vector_adapt;
  WINNER_FUNCTION *find_winners = teach->winner;
  ALPHA_FUNC *get_alpha = teach->alpha_func;
  struct entries *data = teach->data;
  struct entries *codes = teach->codes;
  struct snapshot_info *snap = teach->snapshot;
  long length = teach->length;
  float alpha = teach->alpha;
  struct winner_info win[2];
  eptr p;

  dim = codes->dimension;
  
  total_length = length;

  if ((datatmp = rewind_entries(data, &p)) == NULL)
    {
      fprintf(stderr, "lvq3_training: can't get data\n");
      return NULL;
    }
  numofe = data->flags.totlen_known ? data->num_entries : 0;
  
  for (le = 0; le < length; le++, datatmp = next_entry(&p))
    {
      if (datatmp == NULL)
	{
	  datatmp = rewind_entries(data, &p);
	  if (datatmp == NULL)
	    {
	      fprintf(stderr, "lvq3_training: can't rewind data (%ld/%ld iterations)\n", 
		      le, length);
	      return NULL;
	    }
	}
      
      /* True alpha is decreasing linearly during the training */
      talpha = get_alpha(le, length, alpha);
      
      /* find two best mathing units */
      find_winners(codes, datatmp, win, 2);
      
      shortest = win[0].diff;
      best = win[0].winner;
      label = get_entry_label(best);
      
      nshortest = win[1].diff;
      nbest = win[1].winner;
      nlabel = get_entry_label(nbest);
      
      datalabel = get_entry_label(datatmp);
      /* Corrections are made if the two nearest codebook vectors
	 belong to different classes, one of them correct, and if the
	 input entry is located inside a window between the nearest codebook
	 vectors OR the two nearest codebook vectors both belong to the
	 correct class */
      if (label != nlabel) {
	if ((label == datalabel) || (nlabel == datalabel)) {
	  
	  /* If the ration of distances to the two nearest codebook vectors
	     satisfies a condition */
	  if ((shortest/nshortest) > ((1-winlen)/(1+winlen))) {
	    /* If the second best is correct swap the entries */
	    if (nlabel == datalabel) {
	      ntmp = best;
	      best = nbest;
	      nbest = ntmp;
	    }
	    
	    /* Move the entries */
	    adapt(best, datatmp, dim, talpha);
	    adapt(nbest, datatmp, dim, -talpha);
	  }
	}
      }
      else {
	if (label == datalabel) {
	  /* Move the entries, both toward */
	  adapt(best, datatmp, dim, talpha * epsilon);
	  adapt(nbest, datatmp, dim, talpha * epsilon);
	}
      }

      /* save snapshot when needed */
      if ((snap) && ((le % snap->interval) == 0) && (le > 0))
	{
	  ifverbose(3)
	    fprintf(stderr, "Saving snapshot, %ld iterations\n", le);
	  if (save_snapshot(teach, le))
	    {
	      fprintf(stderr, "snapshot failed\n");
	    }
	}
      
      ifverbose(1)
	mprint(length - le);
    }
  ifverbose(1)
    fprintf(stderr, "\n");

  return(codes);
}

float devdist( float *v1, float *v2, int dim)
{
  float diff, d;
  d = 0.0;
  while (dim-- > 0) {
    diff = *v1++ - *v2++;
    d += diff*diff;
  }
  return(d);
}

struct mindists *deviations(struct entries *codes, struct mindists *md)
{
  int i, j;
  int nol, dim, *noe, label;
  int *count;
  float *devs;
  float *avers;
  struct data_entry *entr;
  eptr p;

  dim = codes->dimension;
  nol = md->num_classes;
  noe = md->noe;

  count = (int *) oalloc(sizeof(int) * nol);

  devs = calloc(nol, sizeof(float));
  if ((md->devs = devs) == NULL)
    {
      return NULL;
    }

  avers = (float *) oalloc(sizeof(float) * nol * dim);
  for (i = 0; i < nol; i++) {
    devs[i] = 0.0;
    for (j = 0; j < dim; j++) {
      avers[i*dim+j] = 0.0;
    }
    count[i] = 0;
  }

  /* Compute averages */

  entr = rewind_entries(codes, &p);
  while (entr != NULL) {
    label = get_entry_label(entr);

    for (i = 0; i < nol; i++)
      if (label == md->class[i])
	break;
    
    for (j = 0; j < dim; j++)
      if (!((entr->mask != NULL) && (entr->mask[j] != 0)))
	{
	  avers[i*dim+j] += entr->points[j];
	}
    /* count[i]++; */

    entr = next_entry(&p);
  }
  
  for (i = 0; i < nol; i++) {
    for (j = 0; j < dim; j++) {
      avers[i*dim+j] /= noe[i];
    }
  }

  /* Compute deviations */
  entr = rewind_entries(codes, &p);
  while (entr != NULL) {
    label = get_entry_label(entr);

    for (i = 0; i < nol; i++)
      if (label == md->class[i])
	break;

    devs[i] += devdist(entr->points,
          &(avers[i*dim]), dim);
    entr = next_entry(&p);
  }
  for (i = 0; i < nol; i++) {
    devs[i] = sqrt(devs[i]/noe[i]);
  }

  return(md);
}


