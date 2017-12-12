/************************************************************************
 *                                                                      *
 *  Program package 'lvq_pak':                                          *
 *                                                                      *
 *  lvq_run.c                                                           *
 *  - user interface to lvq_pak                                         *
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
#include <stdlib.h>
#ifndef max
#define max(x,y) ((x)<(y) ? (y):(x))
#endif

#ifdef MSDOS 
#include <sys\\stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "lvq_pak.h"
#include "datafile.h"
#include "labels.h"

/*---------------------------------------------------------------------------*/
#define R_OK 4
#define W_OK 2
#define F_OK 0

#define BEL 7
#define FLEN 80
#define CMD_LEN 160
#define SAVED_HISTORY	20
#define MAX_NUM_CLASSIFIERS 10

/*---------------------------------------------------------------------------*/

#define DEFAULT_LVQ1_ALPHA	0.03
#define DEFAULT_LVQ2_ALPHA	0.03
#define DEFAULT_LVQ3_ALPHA	0.03

enum e_lvq_init   { NONE=0, EVEN, PROP};
enum e_lvq_status { NOTHING=0, INIT, TRAIN, RETRAIN};



struct classifier {
  char	din[FLEN];
  int	notv;
  char	cout[FLEN];
  char	tdin[FLEN];
  int	noc;
  int	init_opt;
  int	lvq_status;
  long	rlen, totrlen;
  int	rt_lvq_type;
  long	rt_rlen;
  float	rt_alpha;
  float	rt_win;
  float rt_epsilon;
  float	accuracy;
  char	*history[SAVED_HISTORY];
  int	hist_i;
  int	train_hist_bgn;
  int	retrain_hist_bgn;
};

typedef struct classifier CLASSIFIER;


/*---------------------------------------------------------------------------*/
char
  *init_ext=	".ini",	
  *train_ext=	".cod",	
  *retrain_ext=	".lvq",	
  *class_ext=	".cfo",	
  *acc_ext=	".acc",
  *alpha_ext=	".lra",
  *init_alpha_ext=".lrs",
  *train_alpha_ext=".lrt",
  *log_ext=	".log";	

char *prog_dir;

#ifdef MSDOS 
char
  *type_cmd=	"type",
  *copy_cmd=	"copy";
#else /* UNIX */
char	
  *type_cmd=	"cat",
  *copy_cmd=	"cp";
#endif


char *sep = "\n\
==============================================================================\
\n";

char *introMsg0 = "\n\
\nThis program acts as a very simple interactive interface to the lvq_pak.\
\nTo run this program, you must have a training data file in the format\
\nexplained in the document. Preferably, you ought to have independent test\
\ndata to evaluate the performance, too. In addition, you must have an idea\
\nof how many codebook vectors you wish to use. This number depends on the";
char *introMsg1 = "\
\ndimensionality of the training data, on the number of classes you\
\nhave and on the amount of training data available.\
\nThe program suggests default values for most of the parameters. We suggest\
\nthat you use them initially. To select the default value in question, just\
\npress enter.\
\n";




/*---------------Routines related to history handling------------------------*/

void fix_init_history(CLASSIFIER *c) {c->train_hist_bgn = c->hist_i;}
     
void fix_train_history(CLASSIFIER *c) {c->retrain_hist_bgn = c->hist_i;}
     
void decrease_lvq_status(CLASSIFIER *c, int newstatus) 
{
  char l[CMD_LEN];
  int i;
  
  if (newstatus < c->lvq_status) {
    switch (newstatus) {
    case NOTHING:
      for (i=0; i<c->hist_i; i++) free(c->history[i]);
      c->hist_i = c->train_hist_bgn = c->retrain_hist_bgn = 0;
      sprintf(l,"%s%s",c->cout,alpha_ext); remove(l);
      break;
    case INIT:
      for (i=c->train_hist_bgn; i<c->hist_i; i++) free(c->history[i]);
      c->hist_i = c->train_hist_bgn;
      c->retrain_hist_bgn = 0;
      break;
    case TRAIN:
      for (i=c->retrain_hist_bgn; i<c->hist_i; i++) free(c->history[i]);
      c->hist_i = c->retrain_hist_bgn;
      break;
    default:
      fprintf(stdout,"\nERROR: Cannot decrease status!\n");
      break;
    }
    c->lvq_status = newstatus;
  }
}


char *dup_history(char *str)
{
  return( (char *)memcpy( (char *)malloc(max(CMD_LEN,strlen(str)+1)), 
			 str, 
			 strlen(str)+1 ));	
}


char *repl_history(char *inwhere, char *what, char *bywhat) 
     /* Replaces all occurrences of " what." by " bywhat." 
	in string "inwhere". Assumes there is enough space. */
{
  int i,j,len,lwhatlen,lbywhatlen;
  char lwhat[FLEN], lbywhat[FLEN], duplicate[CMD_LEN];
  
  len = strlen(inwhere);
  lwhat[0]=lbywhat[0]=' ';
  strcpy( lwhat+1, what );
  strcpy( lbywhat+1, bywhat );
  strcat( lwhat, ".");
  strcat( lbywhat, ".");
  lwhatlen = strlen(lwhat);
  lbywhatlen = strlen(lbywhat);
  strcpy(duplicate,inwhere);
  
  for (i=j=0; i<len;) {
    if (strncmp(&duplicate[i],lwhat,lwhatlen)==0) {
      memcpy( &inwhere[j], lbywhat, lbywhatlen);
      j += lbywhatlen;
      i += lwhatlen;
    } else  
      inwhere[j++] = duplicate[i++];
  }
  inwhere[j] = '\0';
  return(inwhere);
}


void copy_history(CLASSIFIER *c1, CLASSIFIER *c2) 
     /* copies history of c2 to c1 and replaces filenames */
{
  int i;
  for (i=0; i<c2->hist_i; i++) c1->history[i] = 
    repl_history( dup_history(c2->history[i]), c2->cout, c1->cout);
}



/*----------------------Some assisting routines------------------------------*/

void systemd(char *cmd)
{
  fprintf(stdout,">>%s\n",cmd); 
  fflush(stdout);
  system( cmd );
}


void systemh(char *cmd, CLASSIFIER *c)
{
  systemd( cmd );
  c->history[c->hist_i++] = dup_history(cmd);
}


void getsb(char *line)
{
  if (!silent(-1))
    putchar(BEL);
  fgets(line, CMD_LEN, stdin);
  if (line[strlen(line)-1] == '\n')
    line[strlen(line)-1] = '\0';
}	

int faccess( char *name, int amode)
{
  int err;
  struct stat s;
  err = stat( name, &s );
  if (err == -1) return(-1); else
  switch (amode) {
	case R_OK:
	case F_OK:
		if (s.st_mode & S_IREAD) return(0); else return(-1);
	case W_OK:
		if (s.st_mode & S_IWRITE) return(0); else return(-1);
	default:
		return(-1);
  }
}


int check_filename( char *name, int amode)
{
  int ok = 1;
  char l[CMD_LEN];
  
  switch (amode) {
  case R_OK:
    ok = (faccess(name,amode) == 0);
    if (!ok) {
      sprintf(l,"\n Cannot read file %s\n",name);
      perror(l);
    }
    break;
  case W_OK:
    if ( faccess(name,F_OK) == 0) {
      fprintf(stdout,"\n File %s already exists",name);
      fprintf(stdout,"\n Enter y to overwrite: ");
      getsb(l);
      if (*l == 'y') {
	ok = (faccess(name,amode) == 0);
	if (!ok) {
	  sprintf(l,"\n Cannot write to file %s\n",name);
	  perror(l);
	} else {
	  ok = 2;
	}
      } else {
	ok=0;
      }
    } else {
      FILE *f;
      if ( (f=fopen(name,"w"))==NULL) {
	fprintf(stdout,"\n Cannot create file %s \n",name);
	ok=0;
      } else {
	fclose(f);
	remove(name);
	ok=1;
      }
    }
    break;
  default:
    fprintf(stdout,"\nERROR: Illegal file access mode %d!\n",amode);
    exit(-1);
  }
  return (ok);
}



void remove_classifier_files( char *cname )
{
  char l[CMD_LEN];
  sprintf(l,"%s%s",cname,init_ext);remove(l); 
  sprintf(l,"%s%s",cname,train_ext);remove(l);
  sprintf(l,"%s%s",cname,retrain_ext);remove(l); 
  sprintf(l,"%s%s",cname,class_ext);remove(l);
  sprintf(l,"%s%s",cname,acc_ext);remove(l); 
  sprintf(l,"%s%s",cname,alpha_ext);remove(l);
  sprintf(l,"%s%s",cname,init_alpha_ext);remove(l); 
  sprintf(l,"%s%s",cname,train_alpha_ext);remove(l); 
  sprintf(l,"%s%s",cname,log_ext);remove(l);
}



void copy_classifier_files( char *existing, char *new )
{
  char l[CMD_LEN];
  sprintf(l,"%s %s%s %s%s",copy_cmd, existing,init_ext, new,init_ext);
  system(l);
  sprintf(l,"%s %s%s %s%s",copy_cmd, existing,train_ext, new,train_ext);
  system(l);
  sprintf(l,"%s %s%s %s%s",copy_cmd, existing,class_ext, new,class_ext);
  system(l);
  sprintf(l,"%s %s%s %s%s",copy_cmd, existing,acc_ext, new,acc_ext);
  system(l);
  sprintf( l, "%s%s", existing,retrain_ext);
  if ( faccess(l, R_OK) == 0) {
	sprintf(l,"%s %s%s %s%s",copy_cmd, existing,retrain_ext, new,retrain_ext);
	system(l);
  }
  sprintf( l, "%s%s", existing,alpha_ext);
  if ( faccess(l, R_OK) == 0) {
	sprintf(l,"%s %s%s %s%s",copy_cmd, existing,alpha_ext, new,alpha_ext);
	system(l);
  }
  sprintf( l, "%s%s", existing,init_alpha_ext);
  if ( faccess(l, R_OK) == 0) {
	sprintf(l,"%s %s%s %s%s",copy_cmd, existing,init_alpha_ext, new,init_alpha_ext);
	system(l);
  }
  sprintf( l, "%s%s", existing,train_alpha_ext);
  if ( faccess(l, R_OK) == 0) {
	sprintf(l,"%s %s%s %s%s",copy_cmd, existing,train_alpha_ext, new,train_alpha_ext);
	system(l);
  }
}


/*---------------------------------------------------------------------------*/

void estimate_needed_codevectors
  (char *in_code_file, int *noc, int *notv)
{
  struct data_entry *e;
  struct hit_entry *h;
  struct hitlist *classes;
  eptr p;
  int nol = 0, sum = 0, retval;
  struct entries *codes;
  
  if ((codes = open_entries(in_code_file)) == NULL)
    {
      fprintf(stderr, "Can't open data file '%s'\n", in_code_file);
      exit(1);
    }

  if ((classes = new_hitlist()) == NULL)
    return; /* error */

  for (e = rewind_entries(codes, &p); e != NULL; e = next_entry(&p))
    add_hit(classes, get_entry_label(e));

  fprintf(stdout,"\n\n The dimensionality of the training data in file %s is %d.",
	  in_code_file, codes->dimension);
  for (h = classes->head; h != NULL; h = h->next)
    {
      fprintf(stdout, "In class %s are %ld units\n",
	      find_conv_to_lab(h->label), h->freq);
      sum += h->freq;
      nol++;
    }

  fprintf(stdout," The total number of training vectors is %d.\n\n",sum);
  retval = 0.4 * nol*( nol-1 + codes->dimension/2);
  if (retval > sum) retval = sum;
  close_entries(codes);
  
  *noc = retval;
  *notv = sum;
}




/*--------Read parameters from user for the LVQ_PAK programs-----------------*/

void default_classifier(CLASSIFIER *c)
{
  c->din[0] = c->cout[0] = c->tdin[0] = '\0';
  c->noc = c->notv = 0;
  c->init_opt = EVEN;
  c->lvq_status = NOTHING;
  c->rt_lvq_type = 1;
  c->rt_alpha = 0.0;
  c->rt_win = 0.3;
  c->rt_epsilon = 0.1;
  c->rlen = c->totrlen = c->rt_rlen = 0;
  c->accuracy = 0.0;
  c->hist_i = c->train_hist_bgn = c->retrain_hist_bgn = 0;
}



void read_classifier_file(CLASSIFIER *c)
{
  char l[CMD_LEN]; int ok;
  do {
    fprintf(stdout,"\n*Enter the name of the file to which the codebook vectors");
    fprintf(stdout,"\n*will be stored (without .cod extension): ");
    getsb(l); 
    if (*l == '\0') ok=0; else {
      strcpy( c->cout, l);
      strcat(l, train_ext);
      ok = check_filename( l, W_OK);
    }
  } while (ok==0);
  
  /* remove old files */
  if (ok==2) remove_classifier_files( c->cout );
}



void read_classifier_parameters(CLASSIFIER *c)
{
  char l[CMD_LEN];
  int ok;
  int i;
  
  fprintf(stdout,sep);
  fprintf(stdout,"Enter now the parameters and associated filenames for this LVQ-classifier.");
  
  do {
    if (c->din[0]) fprintf(stdout,"\n*Enter training data file (%s): ", c->din);
    else fprintf(stdout,"\n*Enter training data file: ");
    getsb(l); 
    if (*l != 0) {
      /* if a new training file was given, all previous
	 training must be rerun */
      decrease_lvq_status(c,NOTHING);
      c->noc = 0; /* must be recomputed */
      sscanf(l, "%s", c->din);
    }
    ok = check_filename( c->din, R_OK);
  } while (!ok);
  
  if (c->noc == 0) {
    fprintf(stdout,"\n Reading input data..."); fflush(stdout);
    estimate_needed_codevectors(c->din, &c->noc, &c->notv);
  }
  
  fprintf(stdout,"*Enter the desired total number of codevectors which will be\n");
  fprintf(stdout,"*divided among classes (default: %d): ",c->noc);
  getsb(l); 
  if (*l != 0) {
    sscanf(l, "%d", &c->noc);
    decrease_lvq_status(c,NOTHING);
    c->rlen = 0; /* must be recomputed */
  }
  
  fprintf(stdout,"\nNext, you have to choose how to initialize the codevectors.\n");
  fprintf(stdout,"The options are: \n");
  fprintf(stdout,"	1: Equal allocation of codevectors to each class.\n");
  fprintf(stdout,"	2: Proportional to the amount of training data for each class.\n");
  fprintf(stdout,"We recommend that you use option 1.\n");
  fprintf(stdout,"*Enter your choice (default is %d): ",c->init_opt);
  getsb(l); 
  if (*l != 0) {
    sscanf(l, "%d", &i);
    if (i != c->init_opt) {
      c->init_opt = i;
      decrease_lvq_status(c,NOTHING);
    }
  }

  
  fprintf(stdout,"\nYou must now specify how many training iterations are used. We suggest ");
  fprintf(stdout,"\na number that is about 40 times the number of codebook vectors.");
  if (c->rlen == 0) {
    c->rlen = 40*c->noc;
    decrease_lvq_status(c,INIT);
  }
  fprintf(stdout,"\n*Enter the number of training iterations (%ld): ",c->rlen);
  getsb(l);
  if (*l != 0) {
    long nrlen;
    sscanf(l,"%ld", &nrlen);

    /* has the classifier once been trained (by olvq1)
       even retrained ? */
    if (c->lvq_status >= TRAIN) {

      /* Can we continue previously done training? */
      if (nrlen > c->totrlen) {

	fprintf(stdout,"You entered a number larger than used previously. In this case we can");
	fprintf(stdout,"\ncontinue previous training because olvq1 saves its final state to a file.");
	c->rlen = nrlen - c->totrlen;
	
	/* Restore the state after previous run(s) of 'olvq1'.
	   This must be done because LVQ{1,2,3} destroys the 
	   file "classifier_name.lra" */
	sprintf(l,"%s%s",c->cout,train_alpha_ext);
	if (faccess(l,F_OK)==0) {
	  sprintf(l,"%s %s%s %s%s",copy_cmd, c->cout,
		  train_alpha_ext,c->cout,alpha_ext);
	  system(l);
	}

	/* remove retraining history */
	decrease_lvq_status(c,TRAIN); 
	c->lvq_status = INIT;
      } else {
	/* Cannot continue previous olvq1-training (less
	   training cycles requested than done at previous run).
	   Start olvq1 from the state after initialization. */
	decrease_lvq_status(c,INIT);
	c->rlen = nrlen;
	c->totrlen = 0;
	
	/* must destroy the current state of olvq1 */
	sprintf(l,"%s%s",c->cout,alpha_ext);remove(l); 
	sprintf(l,"%s%s",c->cout,train_alpha_ext);remove(l); 
	sprintf(l,"%s%s",c->cout,train_ext);remove(l);
	
	/* restore the state after previous (possible) 
	   run(s) of 'balance' */
	sprintf(l,"%s%s",c->cout,init_alpha_ext);
	if (faccess(l,F_OK)==0) {
	  sprintf(l,"%s %s%s %s%s",copy_cmd, c->cout,
		  init_alpha_ext,c->cout,alpha_ext);
	  system(l);
	}
      }
    } else {
      /* The classifier hasn't yet been trained. */
      c->rlen = nrlen;
      c->totrlen = 0;
      decrease_lvq_status(c,INIT);
    }
  }
  
  
  do {
    fprintf(stdout,"\n*Enter the test data file");
    if (c->tdin[0]) fprintf(stdout," (%s): ",c->tdin); else fprintf(stdout,": ");
    getsb(l); 
    if (*l != 0) {
      sscanf(l, "%s", c->tdin);
      c->accuracy = 0.0;
    }
    ok = check_filename( c->tdin, R_OK);
  } while (!ok);
}



void read_retrain_parameters(CLASSIFIER *c, int add)
{
  char l[CMD_LEN];
  int i;
  
  fprintf(stdout,"\nChoose the type of LVQ to be used for fine-tuning.\n");
  fprintf(stdout,"	1: LVQ1\n");
  fprintf(stdout,"	2: LVQ2.1\n");
  fprintf(stdout,"	3: LVQ3\n");
  fprintf(stdout,"*Enter your choice (%d): ",c->rt_lvq_type);
  getsb(l); 
  if (*l != 0) {
    sscanf(l, "%d", &i);
    if (i!=1 && i!=2 && i!=3) i=1;
    if (i != c->rt_lvq_type) {
      c->rt_alpha = 0.0; /* if changed LVQ-type set new default */
      c->rt_lvq_type = i;
      if (!add) decrease_lvq_status(c,TRAIN);
    }
  }
  
  /* set default alphas */
  if (c->rt_alpha == 0.0)
    if (c->rt_lvq_type == 1) c->rt_alpha = DEFAULT_LVQ1_ALPHA; else
      if (c->rt_lvq_type == 2) c->rt_alpha = DEFAULT_LVQ2_ALPHA; else
        if (c->rt_lvq_type == 3) c->rt_alpha = DEFAULT_LVQ3_ALPHA; else
	  c->rt_alpha = 0.02;
  
  fprintf(stdout,"\n*Enter the initial value for alpha (%g): ",c->rt_alpha);
  getsb(l);
  if (*l != 0) {
    sscanf(l,"%f", &c->rt_alpha);
    if (!add) decrease_lvq_status(c,TRAIN);
  }
  
  
  fprintf(stdout,"\nYou must now specify how many training iterations are used.");
  fprintf(stdout,"\nWe suggest a number that is at least five times the number of ");
  fprintf(stdout,"\ntraining vectors in your file %s.",c->din);
  if (c->rt_rlen == 0) {
    c->rt_rlen = 5*c->notv;
    if (!add) decrease_lvq_status(c,TRAIN);
  }
  fprintf(stdout,"\n*Enter the number of training iterations (%ld): ",c->rt_rlen);
  getsb(l);
  if (*l != 0) {
    sscanf(l,"%ld", &c->rt_rlen);
    if (!add) decrease_lvq_status(c,TRAIN);
  }
  
  
  if (c->rt_lvq_type == 2 || c->rt_lvq_type == 3) {
    fprintf(stdout,"\nSpecify the width of the window in which the adaptation takes place.");
    fprintf(stdout,"\n*Enter the width (%g): ",c->rt_win);
    getsb(l);
    if (*l != 0) {
      sscanf(l,"%f", &c->rt_win);
      if (!add) decrease_lvq_status(c,TRAIN);
    }
  }

  if (c->rt_lvq_type == 3) {
    fprintf(stdout,"\n*Enter the stabilizing factor (epsilon) (%g): ",c->rt_epsilon);
    getsb(l);
    if (*l != 0) {
      sscanf(l,"%f", &c->rt_epsilon);
      if (!add) decrease_lvq_status(c,TRAIN);
    }
  }
}




/*---------------Read and write classifiers to/from file---------------------*/

void input_classifier(FILE *f, CLASSIFIER *c)
{
  int i;
  char l[CMD_LEN];
  
  fgets(l,CMD_LEN,f);sscanf(l,"%s", c->din);
  fgets(l,CMD_LEN,f);sscanf(l,"%d", &c->notv);
  fgets(l,CMD_LEN,f);sscanf(l,"%s", c->tdin);
  fgets(l,CMD_LEN,f);sscanf(l,"%s", c->cout);
  fgets(l,CMD_LEN,f);sscanf(l,"%d", &c->noc);
  fgets(l,CMD_LEN,f);sscanf(l,"%d", &c->init_opt);
  fgets(l,CMD_LEN,f);sscanf(l,"%ld",&c->totrlen);
  fgets(l,CMD_LEN,f);sscanf(l,"%ld",&c->rlen);
  fgets(l,CMD_LEN,f);sscanf(l,"%d", &c->lvq_status);
  
  fgets(l,CMD_LEN,f);sscanf(l,"%d", &c->rt_lvq_type);
  fgets(l,CMD_LEN,f);sscanf(l,"%ld", &c->rt_rlen);
  fgets(l,CMD_LEN,f);sscanf(l,"%f", &c->rt_alpha);
  fgets(l,CMD_LEN,f);sscanf(l,"%f", &c->rt_win);
  fgets(l,CMD_LEN,f);sscanf(l,"%f", &c->rt_epsilon);
  
  fgets(l,CMD_LEN,f);sscanf(l,"%f", &c->accuracy);
  
  fgets(l,CMD_LEN,f); 
  fgets(l,CMD_LEN,f); sscanf(l,"%d %d %d", 
			     &c->hist_i, &c->train_hist_bgn, &c->retrain_hist_bgn);
  for (i=0; i < c->hist_i; i++) {
    fgets(l,CMD_LEN,f); 
    l[strlen(l)-1]='\0';
    c->history[i] = dup_history(l);
  }
}



void print_classifier(FILE *f, CLASSIFIER *c)
{
  int i;
  if (f==stdout) {
    fprintf(f,sep);
    fprintf(f,"\n");
  }
  fprintf(f,"%s\t Training data file\n", c->din);
  fprintf(f,"%d\t Number of training vectors\n", c->notv);
  fprintf(f,"%s\t Testing data file\n", c->tdin);
  fprintf(f,"%s\t Codebook vector files\n", c->cout);
  fprintf(f,"%d\t Number of codebook vectors\n", c->noc);
  fprintf(f,"%d\t Initializing option\n", c->init_opt);
  fprintf(f,"%ld\t Training cycles used\n", c->totrlen);
  if (f!=stdout) {
    fprintf(f,"%ld\t Training cycles used in latest teaching\n", c->rlen);
    fprintf(f,"%d\t Current status\n", c->lvq_status);
  }
  
  if ( f!=stdout || c->lvq_status == RETRAIN ) {
    fprintf(f,"%d\t retrain LVQ-type\n", c->rt_lvq_type);
    fprintf(f,"%ld\t Training cycles used\n", c->rt_rlen);
    fprintf(f,"%g\t Initial alpha\n", c->rt_alpha);
    if (f!=stdout || c->rt_lvq_type == 2 || c->rt_lvq_type == 3)
      fprintf(f,"%g\t Window width\n", c->rt_win);
    if (f!=stdout || c->rt_lvq_type == 3)
      fprintf(f,"%g\t Epsilon\n", c->rt_epsilon);
  }
  fprintf(f,"%g\t Accuracy\n", c->accuracy);
  
  fprintf(f,"Recent history:\n");
  if (f!=stdout) fprintf(f,"%d %d %d\n", 
			 c->hist_i, c->train_hist_bgn, c->retrain_hist_bgn);
  for (i=0; i < c->hist_i; i++) fprintf(f,"%s\n", c->history[i]);
}





int retrieve_classifiers(int argc, char **argv, CLASSIFIER c[])
{
  int i,ci;
  char l[CMD_LEN];
  FILE *f;
  
  for (ci=0,i=1; i<argc; i++) {
    if (argv[i][0] != '-') {
      sprintf(l,"%s%s", argv[i],log_ext );
      f = fopen(l,"r");
      if (f==NULL) {
        fprintf(stdout,"\nERROR: cannot find classifier %s!\n",l);
      } else {
        fprintf(stdout,"\nReading classifier %s.",l);
        input_classifier(f,&c[ci++]);
        fclose(f);
      }
    }
    else
      i++;
  }
  return(ci);
}


/*-----------------------Run the LVQ_PAK programs----------------------------*/

void init_classifier (CLASSIFIER *c)
{
  char l[CMD_LEN];
  
  if (c->lvq_status < INIT) {
    
    fprintf(stdout,"\nRunning initialization: %d\n", (int) c->init_opt);
    
    switch(c->init_opt) {
    case EVEN:
      sprintf(l,"%seveninit -noc %d -din %s -cout %s%s -knn 5",
	      prog_dir, c->noc, c->din, c->cout, init_ext);
      systemh(l,c);
      break;
    case PROP:
      sprintf(l,"%spropinit -noc %d -din %s -cout %s%s -knn 5",
	      prog_dir, c->noc, c->din, c->cout, init_ext);
      systemh(l,c);
      break;
    default:
      fprintf(stdout,"\nIllegal initializing option %d\n",c->init_opt);
      exit(-1);
    }
    
    /* check that the initialized code file was created */
    sprintf( l, "%s%s", c->cout, init_ext);
    if ( faccess(l, R_OK) != 0) {
      fprintf(stdout,"\nUnsuccesful initialization!\n");
      exit(-1);
    }
    
    fprintf(stdout,"\nNow you have the possibility to modify the number of codevectors");
    fprintf(stdout,"\nso that the minimum distances between the codevectors within each");
    fprintf(stdout,"\nclass will be balanced. The current situation is as follows:\n");
    
    /* display distances between codevectors */
    sprintf(l,"%smindist -cin %s%s", prog_dir, c->cout, init_ext);
    systemd(l);
    
    do {
      fprintf(stdout,"\nDo you want to run an iteration of balancing? y/n (default=n) ");
      getsb(l); 
      if (*l != 'y') break;
      
      sprintf(l,"%sbalance -din %s -cin %s%s -cout %s%s -knn 5",
	      prog_dir, c->din, c->cout, init_ext, c->cout, init_ext);
      systemh(l,c);
    } while (1);
    
    c->lvq_status = INIT;
    fix_init_history(c);
    
    /* If balance was used, it created a file containing learning
       rates for each of the codevectors. It is stored for further
       use (rerun training but keep initialization). */
    sprintf( l, "%s%s", c->cout, alpha_ext);
    if ( faccess(l, F_OK) == 0) {
      sprintf(l,"%s %s%s %s%s",copy_cmd, c->cout, alpha_ext,
	      c->cout, init_alpha_ext);
      system(l);
    }
  }
}




void train_classifier (CLASSIFIER *c)
{
  char l[CMD_LEN], *input_ext;
  
  if (c->lvq_status < TRAIN) {
    
    /* check that initialization or previous olvq1 has been done */
    sprintf(l,"%s%s",c->cout,train_ext);
    if (faccess(l, R_OK)==0) {
      input_ext = train_ext;
    } else {
      sprintf(l,"%s%s",c->cout,init_ext);
      if (faccess(l, R_OK)==0) {
	input_ext = init_ext;
      } else {
	fprintf(stdout,"\nERROR: No initialization has been done for the classifier!\n");
	exit(-1);
      }
    }
    
    /* run the olvq1 training */
    fprintf(stdout,"\nStarting olvq1 training:\n");
    sprintf(l,"%solvq1 -din %s -cin %s%s -cout %s%s -rlen %ld",
	    prog_dir, c->din, c->cout, input_ext, 
	    c->cout, train_ext, c->rlen);
    systemh(l,c);
    c->totrlen += c->rlen;
    c->rlen = c->totrlen;
    
    c->lvq_status = TRAIN;
    fix_train_history(c);
  
    /* check that the trained code file was created */
    sprintf( l, "%s%s", c->cout, train_ext);
    if ( faccess(l, R_OK) != 0) {
      fprintf(stdout,"\nUnsuccesful training!\n");
      exit(-1);
    }
    
    /* olvq1 created a file containing learning rates
       for each of the codevectors. It is stored for further
       use in case where you have done retraining by LVQ? (that
       destroys the ".lra"-file), and you'd like to continue some
       more olvq1-training. */
    sprintf(l,"%s %s%s %s%s",copy_cmd, c->cout,
	    alpha_ext, c->cout, train_alpha_ext);
    system(l);
  }
}




void retrain_classifier (CLASSIFIER *c)
{
  char l[CMD_LEN], *ext;
  
  /* check that olvq1 or previous lvq has been done */
  sprintf(l,"%s%s",c->cout,retrain_ext);
  if (faccess(l, R_OK)==0) {
    ext = retrain_ext;
  } else {
    sprintf(l,"%s%s",c->cout,train_ext);
    if (faccess(l, R_OK)==0) {
      ext = train_ext;
    } else {
      fprintf(stdout,"\nERROR: No training done for the classifier!\n");
      exit(-1);
    }
  }
  
  /* run the lvq-training */
  fprintf(stdout,"\nStarting training:\n");
  switch(c->rt_lvq_type) {
  case 1:
    sprintf(l,"%slvq1 -din %s -cin %s%s -cout %s%s -alpha %g -rlen %ld",
	    prog_dir, c->din, c->cout, ext, 
	    c->cout, retrain_ext, c->rt_alpha, c->rt_rlen);
    systemh(l,c);
    break;
  case 2:
    sprintf(l,"%slvq2 -din %s -cin %s%s -cout %s%s -alpha %g -rlen %ld -win %g",
	    prog_dir, c->din, c->cout, ext, 
	    c->cout, retrain_ext, 
	    c->rt_alpha, c->rt_rlen, c->rt_win);
    systemh(l,c);
    break;
  case 3:
    sprintf(l,"%slvq3 -din %s -cin %s%s -cout %s%s -alpha %g -rlen %ld -win %g -epsilon %g",
	    prog_dir, c->din, c->cout, ext, 
	    c->cout, retrain_ext, 
	    c->rt_alpha, c->rt_rlen, c->rt_win, c->rt_epsilon);
    systemh(l,c);
    break;
  default:
    fprintf(stdout,"\nIllegal lvq-type %d\n",c->rt_lvq_type);
    exit(-1);
  }
  
  /* check that the trained code file was created */
  sprintf( l, "%s%s", c->cout, retrain_ext);
  if ( faccess(l, R_OK) != 0) {
    fprintf(stdout,"\nUnsuccesful training!\n");
    exit(-1);
  }
  
  c->lvq_status = RETRAIN;
}




void test_classifier (CLASSIFIER *c)
{
  char l[CMD_LEN];
  char acc_name[FLEN];
  FILE *facc;
  char *ext;
  
  /* check that olvq1 or previous lvq has been done */
  sprintf(l,"%s%s",c->cout,retrain_ext);
  if (faccess(l, R_OK)==0) {
    ext = retrain_ext;
  } else {
    sprintf(l,"%s%s",c->cout,train_ext);
    if (faccess(l, R_OK)==0) {
      ext = train_ext;
    } else {
      fprintf(stdout,"\nERROR: No training done for the classifier!\n");
      exit(-1);
    }
  }
  
  fprintf(stdout,sep);
  fprintf(stdout,"Starting testing:\n"); 
  fflush(stdout);
  
  /* redirect output of "accuracy" to a file */
  sprintf(acc_name, "%s%s",c->cout, acc_ext);
  remove(acc_name);
  sprintf(l,"%saccuracy -din %s -cin %s%s -cfout %s%s > %s", 
	  prog_dir, c->tdin, c->cout, ext, c->cout, class_ext, acc_name);
  systemd(l);
  
  /* type the file */
  sprintf(l,"%s %s", type_cmd, acc_name);
  system(l);
  
  /* read the final recognition accuracy from the file */
  facc = fopen(acc_name,"r");
  do fscanf(facc,"%s",l); while (strcmp(l,"Total"));
  fscanf(facc,"%s",l);
  fscanf(facc,"%s",l);
  fscanf(facc,"%s",l);
  fscanf(facc,"%f", &(c->accuracy) );
  fclose(facc);
}



void compare_classifiers (CLASSIFIER *c1, CLASSIFIER *c2) 
{
  char cif1[FLEN], cif2[FLEN], l[CMD_LEN];
  
  
  /* check that both classifiers have same test files
     and classification information files have been created
     for both of them */
  if ( strcmp(c1->tdin,c2->tdin) != 0 ) {
    fprintf(stdout,"\nClassifiers have been tested with different files %s and %s!\n",
	    c1->tdin,c2->tdin);
    return;
  }
  sprintf(cif1,"%s%s",c1->cout,class_ext);
  if ( faccess(cif1,R_OK) != 0 ) {
    fprintf(stdout,"\nCannot read classification information file %s!\n",
	    cif1);
    test_classifier(c1);
  }
  sprintf(cif2,"%s%s",c2->cout,class_ext);
  if ( faccess(cif2,R_OK) != 0 ) {
    fprintf(stdout,"\nCannot read classification information file %s!\n",
	    cif2);
    test_classifier(c2);
  }
  
  /* run McNemar's test */
  fprintf(stdout,sep);
  sprintf(l,"%smcnemar %s %s",prog_dir, cif1,cif2);
  systemd(l);
}


/*---------------------------------------------------------------------------*/


int main(int argc, char **argv)
{
  int i,j, status;
  char l[CMD_LEN];
  int opt;
  int nocl;
  static CLASSIFIER c[MAX_NUM_CLASSIFIERS];
  char *cpo;

#ifdef MSDOS
  prog_dir = ostrdup(argv[0]);
  cpo = strrchr(prog_dir, '\\');
  if (cpo != NULL) {
    *(cpo+1) = (char) NULL;
  }
  else {
    prog_dir = ostrdup(".\\");
  }
#else
  prog_dir = ostrdup(argv[0]);
  cpo = strrchr(prog_dir, '/');
  if (cpo != NULL) {
    *(cpo+1) = '\0';
  }
  else {
    prog_dir = ostrdup("./");
  }
#endif

  silent((int) oatoi(extract_parameter(argc, argv, SILENT, OPTION), 0));

  fprintf(stdout,introMsg0);
  fprintf(stdout,introMsg1);
  fprintf(stdout,"\nPress enter to continue.");
  getsb(l);
  
  if (argc > 1) {
    nocl = retrieve_classifiers(argc,argv,c); 
  } else {
    nocl = 0;
  }
  
  
  do {
    fprintf(stdout,sep);
    if (nocl==0) 
      fprintf(stdout,"You don't have any classifiers yet. Start by option 1.");
    else if (nocl==1)
      fprintf(stdout,"You have now 1 classifier. Do you want to:");
    else
      fprintf(stdout,"You have now %d classifiers. Do you want to:",nocl);
    fprintf(stdout,"\n 0 -> Quit and save current classifiers.");
    fprintf(stdout,"\n 1 -> Create a completely new classifier from scratch. ");
    fprintf(stdout,"\n 2 -> Create a new classifier by copying the parameters of an old one.");
    fprintf(stdout,"\n      Use option 3 thereafter to modify the new classifier.");
    fprintf(stdout,"\n 3 -> Modify the parameters of a classifier and train it. You can modify as");
    fprintf(stdout,"\n      many or as few parameters as you wish. However, if you have done repeated");
    fprintf(stdout,"\n      fine-tuning, only the parameters of the latest one are in memory and");
    fprintf(stdout,"\n      modifiable. All previous repeated cycles of fine-tuning are then replaced");
    fprintf(stdout,"\n      by this new cycle with modified parameters.");
    fprintf(stdout,"\n 4 -> Fine-tune a classifier by using LVQ1, LVQ2.1, or LVQ3.");
    fprintf(stdout,"\n      You can repeat this step as many times as you wish.");
    fprintf(stdout,"\n 5 -> Delete a classifier.");
    fprintf(stdout,"\n 6 -> View the parameters of a classifier.");
    fprintf(stdout,"\n 7 -> Compare whether two classifiers tested with the same data have any");
    fprintf(stdout,"\n      statistically significant difference.");
    fprintf(stdout,"\n Enter your choice --> ");
    opt=0; getsb(l); sscanf(l, "%d", &opt);
    
    
    switch (opt) {
      
    case 1:
      default_classifier(&c[nocl]);
      read_classifier_parameters(&c[nocl]);
      read_classifier_file(&c[nocl]);
      init_classifier(&c[nocl]); 
      train_classifier(&c[nocl]); 
      test_classifier(&c[nocl]);
      nocl++;
      break;
      
    case 2:
      if (nocl<1) {
	fprintf(stdout,"\nNo classifiers to be copied.");
	break;
      } else if (nocl==1) 
	i=1; 
      else {
	fprintf(stdout,"\nEnter the classifier to be copied [1..%d]: ",nocl);
	getsb(l); sscanf(l, "%d", &i);
      }
      memcpy(&c[nocl],&c[i-1],sizeof(CLASSIFIER));
      read_classifier_file(&c[nocl]);
      copy_history(&c[nocl],&c[i-1]);
      
      /* duplicate all the associated files, too */
      copy_classifier_files( c[i-1].cout, c[nocl].cout );
      nocl++;
      break;
      
    case 3:
      if (nocl<1) {
	fprintf(stdout,"\nNo classifiers to be replaced.");
	break;
      } else if (nocl==1) 
	i=1; 
      else {
	fprintf(stdout,"\nEnter the classifier to be replaced [1..%d]: ",nocl);
	getsb(l); sscanf(l, "%d", &i);
      }
      status = c[i-1].lvq_status;
      read_classifier_parameters(&c[i-1]);
      init_classifier(&c[i-1]); 
      train_classifier(&c[i-1]); 
      if (status==RETRAIN) {
	sprintf(l,"%s%s",c[i-1].cout,retrain_ext);
	remove(l);
	decrease_lvq_status(&c[i-1], TRAIN);
	fprintf(stdout,"\nThe previous classifier was fine-tuned."); 
	fprintf(stdout,"\nFine-tune this one, too? [y/n] (default=n) ");
	getsb(l); 
	if (*l == 'y') {
	  read_retrain_parameters(&c[i-1], 0);
	  retrain_classifier(&c[i-1]);
	}
      }
      test_classifier(&c[i-1]);
      break;
      
    case 4:
      if (nocl<1) {
	fprintf(stdout,"\nNo classifiers to be retrained.");
	break;
      } else if (nocl==1) 
	i=1; 
      else {
	fprintf(stdout,"\nEnter the classifier [1..%d]: ",nocl);
	getsb(l); sscanf(l, "%d", &i);
      }
      read_retrain_parameters(&c[i-1], 1);
      retrain_classifier(&c[i-1]);
      test_classifier(&c[i-1]);
      break;
      
    case 5:
      if (nocl<1) {
	fprintf(stdout,"\nNo classifiers to be deleted.");
	break;
      } else if (nocl==1) 
	i=1; 
      else {
	fprintf(stdout,"\nEnter the classifier to be deleted [1..%d]: ",nocl);
	getsb(l); sscanf(l, "%d", &i);
      }
      remove_classifier_files( c[i-1].cout );
      for (j=i; j<nocl; j++) memcpy(&c[j-1],&c[j],sizeof(CLASSIFIER));
      nocl--;
      break;
      
    case 6:
      if (nocl<1) {
	fprintf(stdout,"\nNo classifiers to be viewed.");
	break;
      } else if (nocl==1) 
	i=1; 
      else {
	fprintf(stdout,"\nEnter the classifier [1..%d]: ",nocl);
	getsb(l); sscanf(l, "%d", &i);
      }
      print_classifier(stdout,&c[i-1]);
      break;
      
    case 7:
      if (nocl<1) {
	fprintf(stdout,"\nCannot compare less than two classifiers!\n");
	break;
      } else if (nocl==2) {
	i=1; j=2;
      } else {
	fprintf(stdout,"\nEnter the 1st classifier [1..%d]: ",nocl);
	getsb(l); sscanf(l, "%d", &i);
	fprintf(stdout,"Enter the 2nd classifier [1..%d]: ",nocl);
	getsb(l); sscanf(l, "%d", &j);
      }
      compare_classifiers( &c[i-1], &c[j-1] );
      break;
      
    case 0:
      if (nocl) {
	FILE *f;
	fprintf(stdout,"\nAs the result of this session of lvq_run,");
	fprintf(stdout,"\nthe following classifiers remain on disk:");
	for (i=0; i<nocl; i++) {
	  sprintf(l,"%s%s",c[i].cout,log_ext);
	  f=fopen(l,"w");
	  if (f!=NULL) {
	    fprintf(stdout,"\n   %s",c[i].cout);
	    print_classifier(f,&c[i]);
	    fclose(f);
	  } else {
	    fprintf(stdout,"\nERROR: Couldn't write to %s.\n",l);
	  }
	}
	fprintf(stdout,"\n\nYou can read in the stored classifiers by starting lvq_run as follows:");
	fprintf(stdout,"\n>> lvq_run classifier1 classifier2 ... classifier10");
	fprintf(stdout,"\nDo not enter any extensions to classifier filenames, just the baseforms.");
	
      }
      
    default:
      break;
    }
    
  } while (opt);
  
  fprintf(stdout,"\n\n");
  
}



