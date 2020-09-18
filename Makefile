# SPDX-License-Identifier: (MIT)
#
# Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
# Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
#

INC = -I. -I./demoapps_common
CC=gcc
CFLAGS=$(INC) -g

ODIR=obj
LDIR=../lib

LIBS=-lrt

_OBJ = demo_tsnsender.o packet_handler.o axisshm_handler.o time_calc.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

all: demo_tsnsender

demo_tsnsender: obj/demo_tsnsender.o obj/packet_handler.o obj/axisshm_handler.o obj/time_calc.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INC)/*~ demo_tsnsender