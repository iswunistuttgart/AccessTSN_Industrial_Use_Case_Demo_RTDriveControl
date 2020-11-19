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

LIBS=-pthread -lrt

_OBJ = packet_handler.o axisshm_handler.o time_calc.o axis_sim.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

all: demo_tsnsender

demo_tsnsender: demo_tsnsender.c obj/packet_handler.o obj/axisshm_handler.o obj/time_calc.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

demo_tsndrive: demo_tsndrive.c obj/packet_handler.o obj/axis_sim.o obj/time_calc.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

recv_test: tests/recv_test.c obj/packet_handler.o obj/time_calc.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

posupdate_test: tests/posupdate_test.c obj/axis_sim.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

drive_test: tests/demo_drive_test.c obj/axis_sim.o obj/time_calc.o obj/axisshm_handler.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core demoapps_common/*~ demo_tsnsender demo_tsndrive tests/recv_test tests7drive_test
