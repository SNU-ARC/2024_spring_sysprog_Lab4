#---------------------------------------------------------------------------------------------------
# Shell Lab                                   Fall 2023                          System Programming
#
# Makefile for regression testing
#
# GNU make documentation: https://www.gnu.org/software/make/manual/make.html
#
# Typically, the only thing you need to modify in this Makefile is the list of source files.
#

#--- variable declarations

# C compiler and compilation flags
CC=gcc
CFLAGS=-Wall -O2 -g

SOURCES=$(wildcard my*.c)
TARGETS=$(SOURCES:.c=)

# test harness
DRIVER=./sdriver.pl
TSHNAME=csapsh
TSH=../${TSHNAME}
TSHREF=../reference/${TSHNAME}
TSHARGS="-p"

# regression traces
TRACEDIR=../traces
TRACES=$(sort $(wildcard $(TRACEDIR)/trace??.txt))
TESTS=$(TRACES:$(TRACEDIR)/trace%.txt=test%)
RTESTS=$(TRACES:$(TRACEDIR)/trace%.txt=rtest%)


#--- rules

all: tools

tools: $(TARGETS)

my%: my%.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS)

mrproper: clean

tests: $(TESTS)
	@echo "Tests completed."

rtests: $(RTESTS)
	@echo "Regression tests completed."

test%: tools
	$(DRIVER) -s $(TSH) -a $(TSHARGS) -t $(TRACEDIR)/$(@:test%=trace%.txt)

rtest%: tools
	$(DRIVER) -s $(TSHREF) -a $(TSHARGS) -t $(TRACEDIR)/$(@:rtest%=trace%.txt)


