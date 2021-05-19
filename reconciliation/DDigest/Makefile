CXX = g++
RM = gio trash -f
MKDIR = mkdir -p

CXXFLAGS = -O3 -Wall -std=c++11 -I$(SRCDIR)

TOP := $(shell /bin/pwd)

BUILD = $(TOP)/build
SRCDIR = $(TOP)/src
DATAOLDDIR = ../../test-sets/100000-1000
DATADIR = ../../test-sets/1000000-1000
#
# Sets & diffs used during revision
#
LARGER_SETS = sets_1000000_1000_32_1574465740.txt
TEST_DIFFS = "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000 16000 30000 50000 100000"
#
# Sets & diffs used in the original submission
#
SETS = sets_100000_1000_32_1574465740.txt
TEST_DIFFS_OLD = "10 20 30 60 100 200 400 700 1000 1400 2500 4000 8000 10000"

TARGET = $(BUILD)/ddigest_perf

all: $(TARGET)

$(BUILD)/ddigest_perf: $(SRCDIR)/KeyDigest.cpp $(SRCDIR)/iblt.cpp $(SRCDIR)/murmurhash3.cpp $(SRCDIR)/utilstrencodings.cpp
	@$(MKDIR) $(BUILD)
	$(CXX) $^ $(CXXFLAGS) -o $@

$(BUILD)/ddigest_experiments_diffs.sh: $(TOP)/cli_utils.py $(BUILD)/ddigest_perf
	@$(MKDIR) $(BUILD)
	@echo "Generating bash file ddigest_experiments_diffs.sh ..."
	$< -b ./ddigest_perf -u 1000000 -d $(TEST_DIFFS) -e "$(DATADIR)/diff_estimates_with_tow_{d}_128_9012.txt" -i $(DATADIR)/$(LARGER_SETS) -o $@
	@chmod +x $@

scripts: $(BUILD)/ddigest_experiments_diffs.sh
	@echo "Generating scripts for DDigest ..."

perf: $(BUILD)/ddigest_experiments_diffs.sh
	@echo "Benchmarking D.Digest ..."
	(cd $(BUILD) && ./ddigest_experiments_diffs.sh)
	@echo "DONE!"
	
clean:
	$(RM) $(BUILD)






