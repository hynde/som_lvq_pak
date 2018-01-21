/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  mcnemar.c                                                           *
 *	Runs the McNemar's test to compare the statistical              *
 *	significance of the difference between the classification       *
 *	accuracies of two classifiers, that have been tested            *
 *	using the SAME data (the samples used to test the               *
 *	classifiers are thus, of course, *NOT* independent).            *
 *                                                                      *
 *	See, for example, the following reference:                      *
 *                                                                      *
 *	J. S. Milton, Jesse C. Arnold, "Probability and statistics in   *
 *	the engineering and computing sciences", McGraw-Hill, New York, *
 *	St. Louis, San Francisco,..., 2nd printing, 1987,               *
 *	pp. 543-544.                                                    *
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

int tbl[2][2] = { {0, 0}, {0, 0} };


static double alpha[4]  = {0.05, 0.025, 0.01, 0.005},
              chi_sq[4] = {3.84, 5.02,  6.63, 7.88};

char *usage = "\n\
Usage: mcnemar classification_file1 classification_file2\n\
 You must first run \"accuracy\" with option \"-cfout classification_file\"\n\
 to create the files containing classification information.\n";


int main(int argc, char **argv)
{
  FILE *cf1, *cf2;
  struct file_info *cfi1 = NULL, *cfi2 = NULL;
  int i1,i2,c1,c2,cnt,i;
  double testv, tmp;
  int error = 1;

  if (argc != 3) {
    fputs(usage, stderr);
    exit(1);
  }
  if ((cfi1 = open_file(argv[1],"r")) == NULL) {
    fprintf(stderr, "\nCannot open %s\n",argv[1]);
    goto cleanup;
  }
  cf1 = fi2fp(cfi1);

  if ( (cfi2 = open_file(argv[2],"r")) == NULL) {
    fprintf(stderr, "\nCannot open %s\n",argv[2]);
    goto cleanup;
  }
  cf2 = fi2fp(cfi2);
  
  for (;;) {
    i1 = fscanf(cf1,"%d", &c1);
    i2 = fscanf(cf2,"%d", &c2);

    if (i1 != i2) {
      fprintf(stderr, "\nERROR: Unequal numbers of classifications in files.\n");
      goto cleanup;
    }
    if (i1 != 1) break;

    if ( (c1!=0 && c1!=1) || (c2!=0 && c2!=1)) {
      fprintf(stderr, "\nFiles contain other than 0's and 1's.\n");
      goto cleanup;
    }

    c1 = 1 - c1;
    c2 = 1 - c2;
    tbl[c1][c2]++;
  }
  
  cnt = tbl[0][1] + tbl[1][0];
  if (cnt) {
    fprintf(stderr, "\nStatistics of the results of the two classifiers:");
    fprintf(stderr, "\n             1st correct,  1st errors");
    fprintf(stderr, "\n2nd correct:      %6d       %6d",   tbl[0][0],tbl[1][0]);
    fprintf(stderr, "\n2nd errors:       %6d       %6d\n", tbl[0][1],tbl[1][1]);
    tmp = tbl[0][1] - tbl[1][0];
    testv = tmp*tmp;
    testv /= cnt;
    fprintf(stderr, "\nTest statistics (%.3f)", testv);
    
    for (i=3; i>=0; i--) if (testv > chi_sq[i]) break;
    
    if (i>=0) {
      fprintf(stderr, " is significant at risk level %.3f\n", alpha[i]);
      fprintf(stderr, "The classifiers are significantly different!\n");
    } else {
      fprintf(stderr, " is not significant!\n");
      fprintf(stderr, "The classifiers are not significantly different!\n");
    }
    
  }
  else
    fprintf(stderr, "\nRecognition result files are equal!\n");

  error = 0;
 cleanup:

  if (cfi1)
    close_file(cfi1);
  if (cfi2)
    close_file(cfi2);

  return(error);
}


