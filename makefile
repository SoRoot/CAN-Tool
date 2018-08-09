#============================================
#		Unity Test Framework
#		Author: Lukas Ungerland
#============================================


ifeq ($(OSTYPE),cygwin)
	CLEANUP=rm -f
	MKDIR=mkdir -p
	TARGET_EXTENSION=out
else ifeq ($(OS),Windows_NT)
	CLEANUP=del /F /Q
	MKDIR=mkdir
	TARGET_EXTENSION=exe
else
	CLEANUP=rm -f
	MKDIR=mkdir -p
	TARGET_EXTENSION=out
endif

PATHU = Unity/src/
PATHS = src/
PATHT = test/
PATHB = build/
PATHD = build/depends/
PATHO = build/objs/
PATHR = build/results/

BUILD_PATHS = $(PATHB) $(PATHD) $(PATHO) $(PATHR)

SRCT = $(wildcard $(PATHT)*.c)
$(info $(SRCT))

COMPILE=gcc -c
LINK=gcc
DEPEND=gcc -MM -MG -MF
CFLAGS=-I. -I$(PATHU) -I$(PATHS) -DTEST

RESULTS = $(patsubst $(PATHT)test%.c,$(PATHR)test%.txt,$(SRCT))
$(info "hallo: "$(RESULTS))





test: $(BUILD_PATHS) $(RESULTS)
		@echo $(RESULTS)
		@echo "-----------------------\nIGNORES:\n------------------------"
		@echo `grep -s IGNORE $(PATHR)*.txt`
		@echo "-----------------------\nFAILURES:\n-----------------------"
		@echo `grep -s FAIL $(PATHR)*.txt`
		@echo "\nDONE"

$(PATHR)%.txt: $(PATHB)%.$(TARGET_EXTENSION)
		-./$< > $@ 2>&1

$(PATHB)test%.$(TARGET_EXTENSION): $(PATHO)test%.o $(PATHO)%.o $(PATHU)unity.o $(PATHD)test%.d
		$(LINK) -o $@ $^


$(PATHO)%.o:: $(PATHT)%.c
		$(COMPILE) $(CFLAGS) $< -o $@

$(PATHO)%.o:: $(PATHS)%.c
		$(COMPILE) $(CFLAGS) $< -o $@

$(PATHO)%.o:: $(PATHU)%.c $(PATHU)%.h
		$(COMPILE) $(CFLAGS) $< -o $@ 


$(PATHD)%.d:: $(PATHT)%.c
		$(DEPEND) $@ $<


$(PATHB):
		$(MKDIR) $(PATHB)

$(PATHD):
		$(MKDIR) $(PATHD)

$(PATHO):
		$(MKDIR) $(PATHO)

$(PATHR):
		$(MKDIR) $(PATHR)


clean:
		$(CLEANUP) $(PATHO)*.o
		$(CLEANUP) $(PATHB)*.$(TARGET_EXTENSION)
		$(CLEANUP) $(PATHR)*.txt


.PRECIOUS: $(PATHB)Test%.$(TARGET_EXTENSION)
.PRECIOUS: $(PATHD)%.d
.PRECIOUS: $(PATHO)%.o
.PRECIOUS: $(PATHR)%.txt

.PHONY: clean
.PHONY: test
