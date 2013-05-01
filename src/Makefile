# This Makefile produces the upc_io.o file

BUPC = /usr/local/bin
CC = ${BUPC}/upcc -translator=/usr/local/bupc_translator/targ

INCLUDE_DIR = -I../include -I/usr/local/include
PUPC_DIR = ../../pupc-io
CFLAGS = ${INCLUDE_DIR} -I${PUPC_DIR} -I${PUPC_DIR}/ADIO -c -network=smp

LINK_DIR =
LFLAGS = ${INCLUDE_DIR} ${LINK_DIR} -network=smp

DEFINITION =
NP = 4
OPTIONS = -T${NP} -DDEBUG -g
OBJS = mpi.o upc_mpi.o mpi_info.o mpi_utils.o mpi_io.o

all: ${OBJS}

mpi.o: ../include/upc_mpi.h ../include/mpi.h mpi.c
	${CC} mpi.c ${OPTIONS} ${DEFINITION} $(CFLAGS) 

upc_mpi.o: ../include/upc_mpi.h upc_mpi.c
	${CC} ${OPTIONS} ${DEFINITION} $(CFLAGS) upc_mpi.c


mpi_info.o: ../include/mpi_info.h ../include/mpi.h mpi_info.c
	${CC} ${OPTIONS} ${DEFINITION} $(CFLAGS) mpi_info.c

mpi_utils.o: ../include/mpi_utils.h ../include/mpi.h mpi_utils.c
	${CC} ${OPTIONS} ${DEFINITION} $(CFLAGS) mpi_utils.c

mpi_io.o: ../include/mpi_io.h ../include/mpi.h mpi_io.c
	${CC} ${OPTIONS} ${DEFINITION} $(CFLAGS) mpi_io.c

clean: 
	rm -f core *.o *~ 
