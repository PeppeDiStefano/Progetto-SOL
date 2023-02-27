# Compilatore da usare
CC			=  gcc
# Flags di compilazione
CFLAGS	    += -std=c99 -Wall -pedantic -g -D_DEFAULT_SOURCE
# Directory includes
INCL_DIR	= ./includes
# Flag inclusione 
INCLUDES	= -I ${INCL_DIR}
# Libreria per compilazione
LIBS        = -pthread

# OBJECTS DEPENDENCIES
FARM_OBJS =  codes/MasterWorker.o codes/Worker.o codes/Collector.o codes/threadpool.o codes/queue.o
GENERAFILE_OBJ = test/generafile.o

# EXE
FARM_EXE = farm
GENERAFILE_EXE = generafile

# TARGETS
TARGETS		= $(FARM_EXE) \
			  $(GENERAFILE_EXE) \

# PHONY
.PHONY: all test clean
.SUFFIXES: .c .h

# Regola di default
all		: $(TARGETS)

# Eseguibile farm
${FARM_EXE}: $(FARM_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(FARM_OBJS) $(LIBS)

# Eseguibile generafile
${GENERAFILE_EXE}: $(GENERAFILE_OBJ) 
	$(CC) $(CFLAGS) -o $@ $(GENERAFILE_OBJ)

test/generafile.o: test/generafile.c
	$(CC) $(CFLAGS) $< -c -fPIC -o $@

codes/%.o: codes/%.c $(INCL_DIR)/%.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -c -fPIC -o $@

# Esecuzione dello script per testare
test:
		bash test.sh

# Per rimuovere tutti gli eseguibili e gli oggetti
clean:
		rm -f -r farm
		rm -f -r generafile
		rm -f -r codes/*.o
		rm -f -r test/*.o
		rm -f -r collector
		rm -f -r file*
		rm -f -r testdir
		rm -f -r expected.txt