CC		=  gcc
CFLAGS          += -std=c99 -Wall

BINDIR		= ./bin
INCDIR		= ./include
LIBDIR		= ./lib
SRCDIR		= ./src
TESTDIR		= ./testfile
SCRIPTDIR	= ./script

INCL_I		= -I $(INCDIR)
INCL_L		= -L $(LIBDIR)
FLAGS           = -g -O3 -pthread

TARGET		= $(BINDIR)/prog
CTEST		= $(TESTDIR)/config.txt


.PHONY: all test clean

#Regola creazione file .o
$(SRCDIR)/%.o:	$(SRCDIR)/%.c
	$(CC) -fPIC $(CFLAGS) $(INCL_I) $(FLAGS) -c -o $@ $<

all	: $(TARGET)

#Creazione eseguibile
$(BINDIR)/prog: $(SRCDIR)/main.c $(LIBDIR)/libsupp.so
	$(CC) $(CFLAGS) $(INCL_I) $(FLAGS) -o $@ $< $(INCL_L) -lsupp

#Creazione libreria .so
$(LIBDIR)/libsupp.so: $(SRCDIR)/supp.o $(INCDIR)/supp.h
	$(CC) -shared -o $@ $<

#TEST
test	: $(TARGET)
	@LD_LIBRARY_PATH=$(LIBDIR) $(TARGET) $(CTEST) > $(SCRIPTDIR)/s.log & \
	PROCESS_TO_KILL="$$!";\
	(sleep 25 && kill -HUP $$PROCESS_TO_KILL);
	@echo "test END"
	@(sleep 5)
	@cd ./script; \
	./analisi.sh s.log

#Pulizia
clean		:
	-rm -f $(TARGET)
	-find . \( -name "*~" -o -name "*.o" \) -exec rm -f {} \;
	-rm -f $(LIBDIR)/libsupp.so
	-rm -f $(SCRIPTDIR)/s.log

