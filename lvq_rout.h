#ifndef _LVQ_ROUT_H
#define _LVQ_ROUT_H

struct mindists {
  int num_classes;
  struct hitlist *classes;
  int *class;    /* class label */
  int *noe;      /* number of entries in class */
  float *dists;  /* distance inside class */
  float *devs;   /* deviations */
};

struct mindists *alloc_mindists(void);
void free_mindists(struct mindists *md);

int correct_by_knn(struct entries *data, struct data_entry *code, int knn, 
		   WINNER_FUNCTION *find_knn);

/* struct data_entry *extract_codes(int ind, struct entries *data); */
struct entries *pick_codes(int num, struct entries *data);
struct data_entry *pick_known_codes(int num, struct entries *data, int index);
struct data_entry *pick_inside_codes(struct hitlist *classes, struct entries *data, int knn, WINNER_FUNCTION *win);
struct data_entry *force_pick_code(struct entries *data, int ind);
struct mindists *min_distances(struct entries *codes, DIST_FUNCTION *);
struct mindists *med_distances(struct entries *codes, DIST_FUNCTION *);
struct mindists *deviations(struct entries *codes, struct mindists *md);

struct entries *olvq1_training(struct teach_params *teach, char *in, char *out);
struct entries *lvq1_training(struct teach_params *teach);
struct entries *lvq2_training(struct teach_params *teach, float winlen);
struct entries *lvq3_training(struct teach_params *teach, float epsilon, float winlen);

#endif /* _LVQ_ROUT_H */

