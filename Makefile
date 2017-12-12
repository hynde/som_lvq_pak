
#
#CC=cc
#CFLAGS=-O
#LDFLAGS=-s
#LDLIBS=-lm
#LD=$(CC)

## NetBSD-1.0 (tested on Amiga)
##
#CC=gcc
#CFLAGS=-O2
#LDFLAGS=-s
#LDLIBS=-lm
#LD=$(CC)

# DEC Alpha/OSF
#
#CC=cc
#CFLAGS=-O2
#LDFLAGS=-s
#LDLIBS=-lm
#LD=$(CC)

## Amiga/AmigaDOS
##  Tested on an Amiga 2000 w/ 68030+68882 with gcc and dmake, OS 3.1
#CC=gcc
#CFLAGS=-O2 -m68020 -m68881
#LDFLAGS=-s
#LDLIBS=-lm
#LD=$(CC)

## HPUX
##
#CC=c89
#CFLAGS=-O
#LDFLAGS=-s
#LDLIBS=-lm
#LD=$(CC)

## Linux
##
CC=gcc
CFLAGS=-O3 -Wall
LDFLAGS=
LDLIBS=-lm
LD=$(CC)

## SGI
##
#CC=cc
#CFLAGS=-O2 -mips1
#LDFLAGS=-s
#LDLIBS=-lm
#LD=$(CC)

TESTFILES_SOM=ex.dat ex_fts.dat ex_ndy.dat ex_fdy.dat
TESTFILES_LVQ=ex1.dat ex2.dat
OBJS_COMMON=lvq_pak.o fileio.o labels.o datafile.o version.o
OBJS_SOM=som_rout.o $(OBJS_COMMON)
OBJS_LVQ=lvq_rout.o $(OBJS_COMMON)
UMATOBJS=umat.o map.o median.o header.o
OTHERFILES=header.ps
PROGRAMS_SOM=vcal mapinit vsom qerror randinit lininit visual sammon planes vfind umat
PROGRAMS_LVQ=accuracy knntest pick setlabel lvqtrain lvq1 lvq2 lvq3 olvq1 eveninit \
	propinit showlabs mindist mcnemar sammon cmatr elimin balance \
	stddev classify extract lvq_run

all:	som lvq
lvq:	$(PROGRAMS_LVQ)
som:	$(PROGRAMS_SOM)

# SOM programs
vsom:	vsom.o $(OBJS_SOM)
	$(LD) $(LDFLAGS) -o $@ vsom.o $(OBJS_SOM) $(LDLIBS)

vsomtest:	vsomtest.o $(OBJS_SOM)
	$(LD) $(LDFLAGS) -o $@ vsomtest.o $(OBJS_SOM) $(LDLIBS)

qerror:	qerror.o $(OBJS_SOM)
	$(LD) $(LDFLAGS) -o $@ qerror.o $(OBJS_SOM) $(LDLIBS)

mapinit:	mapinit.o $(OBJS_SOM)
	$(LD) $(LDFLAGS) -o $@ mapinit.o $(OBJS_SOM) $(LDLIBS)

randinit: mapinit
	rm -f $@
	ln mapinit $@

lininit: mapinit
	rm -f $@
	ln mapinit $@

vcal:	vcal.o $(OBJS_SOM)
	$(LD) $(LDFLAGS) -o $@ vcal.o $(OBJS_SOM) $(LDLIBS)

visual:	visual.o $(OBJS_SOM)
	$(LD) $(LDFLAGS) -o $@ visual.o $(OBJS_SOM) $(LDLIBS)

sammon:	sammon.o $(OBJS_SOM)
	$(LD) $(LDFLAGS) -o $@ sammon.o $(OBJS_SOM) $(LDLIBS)

planes: planes.o $(OBJS_SOM)
	$(LD) $(LDFLAGS) -o $@ planes.o $(OBJS_SOM) $(LDLIBS)

vfind:	vfind.o $(OBJS_SOM)
	$(LD) $(LDFLAGS) -o $@ vfind.o $(OBJS_SOM) $(LDLIBS)

# Umat
umat:	$(UMATOBJS) $(OBJS_SOM)
	$(LD) $(LDFLAGS) -o $@ $(UMATOBJS) $(OBJS_SOM) $(LDLIBS)

map.o umat.o:	umat.h fileio.h datafile.o lvq_pak.h labels.h
median.o:	umat.h

# LVQ programs
accuracy:	accuracy.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ accuracy.o $(OBJS_LVQ) $(LDLIBS)

knntest:	knntest.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

pick:	pick.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

setlabel:	setlabel.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

classify:	classify.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

showlabs:	showlabs.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

extract:	extract.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

mindist:	mindist.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

stddev:	stddev.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

balance:	balance.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

mcnemar:	mcnemar.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

#sammon:	sammon.o $(OBJS_LVQ)
#	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

cmatr:	cmatr.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

elimin:	elimin.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

lvqtrain:	lvqtrain.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

lvq1:	lvqtrain
	rm -f $@
	ln lvqtrain $@

olvq1:	lvqtrain
	rm -f $@
	ln lvqtrain $@

lvq2:	lvqtrain
	rm -f $@
	ln lvqtrain $@

lvq3:	lvqtrain
	rm -f $@
	ln lvqtrain $@

eveninit:	eveninit.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

propinit:	eveninit
	rm -f $@
	ln eveninit $@

lvq_run:	lvq_run.o $(OBJS_LVQ)
	$(LD) $(LDFLAGS) -o $@ $@.o $(OBJS_LVQ) $(LDLIBS)

version.o: version.h

# for testing
#TRAND=-rand 1
#ALPHA_TYPE=-alpha_type inverse_t
#BUFFER=-buffer 500

somexample: $(PROGRAMS_SOM)
	./randinit -din ex.dat -cout ex.cod -xdim 12 -ydim 8 -topol hexa \
  -neigh bubble -rand 123
	./vsom  -din ex.dat -cin ex.cod  -cout ex.cod -rlen 1000 \
  -alpha 0.05 -radius 10 $(TRAND) $(ALPHA_TYPE) $(BUFFER)
	./vsom     -din ex.dat -cin ex.cod  -cout ex.cod -rlen 10000 \
  -alpha 0.02 -radius 3 $(TRAND) $(ALPHA_TYPE) $(BUFFER)
	./qerror   -din     ex.dat -cin ex.cod
	./vcal     -din ex_fts.dat -cin ex.cod -cout ex.cod
	./visual   -din ex_ndy.dat -cin ex.cod -dout ex.nvs
	./visual   -din ex_fdy.dat -cin ex.cod -dout ex.fvs

lvqexample: $(PROGRAMS_LVQ)
	./eveninit -din ex1.dat  -cout ex1e.cod -noc 200
	./mindist  -cin ex1e.cod
	./balance  -din ex1.dat  -cin ex1e.cod  -cout ex1b.cod
	./olvq1    -din ex1.dat  -cin ex1b.cod  -cout ex1o.cod -rlen 5000
	./accuracy -din ex2.dat  -cin ex1o.cod

fileio.o:	fileio.h
datafile.o:	lvq_pak.h datafile.h fileio.h
labels.o:	labels.h lvq_pak.h
lvq_pak.o:	lvq_pak.h datafile.h fileio.h labels.h
lvq_rout.o:	lvq_rout.h lvq_pak.h datafile.h fileio.h
som_rout.o:	som_rout.h lvq_pak.h datafile.h fileio.h labels.h

accuracy.o knntest.o pick.o setlabel.o lvqtrain.o eveninit.o \
  propinit.o showlabs.o mindist.o mcnemar.o sammon.o cmatr.o \
	elimin.o balance.o stddev.o classify.o  \
	lvq_run.o:	lvq_pak.h fileio.h datafile.h labels.h lvq_rout.h

vcal.o mapinit.o vsom.o qerror.o visual.o sammon.o:\
	lvq_pak.h datafile.h fileio.h labels.h som_rout.h

clean:
	rm -f *.o $(PROGRAMS_SOM) $(PROGRAMS_LVQ)
