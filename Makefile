# SPDX-License-Identifier: (MIT)
#
# Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
# Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
#

INC = -I.
CC=gcc
CFLAGS=$(INC) -g

ODIR=obj
LDIR=../lib

LIBS=-lrt

_OBJ = demo_tsnsender.o packet_handler.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

all: demo_tsnsender

demo_tsnsender: obj/demo_tsnsender.o obj/packet_handler.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ demo_tsnsender