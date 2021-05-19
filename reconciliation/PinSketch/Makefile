CXX = g++
RM = rm -rf 
MKDIR = mkdir -p

TOP := $(shell /bin/pwd)

BUILD = $(TOP)/build
SRCDIR = $(TOP)/src
DATAOLDDIR = ../../test-sets/100000-1000
DATADIR = ../../test-sets/1000000-1000
PARAMDIR = ../../PBS/params
PBS_BCH_PRARM_FILENAME_PREFIX = $(TOP)/perf/params_r${max_rounds}.csv
#
# Params used during revision (3 rounds, 5 diffs per group on average)
#
PRARM_FILENAME = params_r3_5_all.csv
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

CXXFLAGS = -O3   -Wall -I$(SRCDIR) -std=c++11
LDFLAGS = -lminisketch -lxxhash

TARGET = $(BUILD)/erlay_perf  $(BUILD)/erlay_wp_perf

all: $(TARGET)

$(BUILD)/erlay_perf: $(SRCDIR)/Erlay.cpp
	@$(MKDIR) $(BUILD)
	$(CXX) $(CXXFLAGS) $<  $(LDFLAGS) -o $@

$(BUILD)/erlay_wp_perf: $(SRCDIR)/Erlay_with_partition.cpp
	@$(MKDIR) $(BUILD)
	$(CXX) $(CXXFLAGS) $<  $(LDFLAGS) -o $@


$(BUILD)/pinsketch_experiments_diffs.sh: $(TOP)/cli_utils.py $(BUILD)/erlay_perf
	@$(MKDIR) $(BUILD)
	$< pinsketch -b ./erlay_perf -u 1000000 -d $(TEST_DIFFS) -e "$(DATADIR)/diff_estimates_with_tow_{d}_128_9012.txt" -i $(DATADIR)/$(LARGER_SETS) -o $@
	@chmod +x $@

$(BUILD)/pinsketch_wp_experiments_diffs.sh: $(TOP)/cli_utils.py $(BUILD)/erlay_wp_perf
	@$(MKDIR) $(BUILD)
	$< pinsketch-wp -b ./erlay_wp_perf -u 1000000 -d $(TEST_DIFFS) -e "$(DATADIR)/diff_estimates_with_tow_{d}_128_9012.txt" -i $(DATADIR)/$(LARGER_SETS) -p $(PARAMDIR)/$(PRARM_FILENAME) -o $@
	@chmod +x $@

scripts: $(BUILD)/pinsketch_experiments_diffs.sh $(BUILD)/pinsketch_wp_experiments_diffs.sh
	@echo "Generate scripts ..."
#
# This target runs a few experiments using the exact same configurations
# as we used before to verify that this version of codes (with relatively
# large modifications) can produce the same results and run at similar
# speed on the same machine
#
consistency_test: $(BUILD)/erlay_perf $(BUILD)/erlay_wp_perf
	(cd $(BUILD) && ./erlay_perf -n 1000 -d 10 -s 0 -u 100000 -e $(DATAOLDDIR)/diff_estimates_with_tow_10_128_9012.txt -i $(DATAOLDDIR)/$(SETS) -r 1.38)
	(cd $(BUILD) && ./erlay_perf -n 1000 -d 10000 -s 18420 -u 100000 -e $(DATAOLDDIR)/diff_estimates_with_tow_10000_128_9012.txt -i $(DATAOLDDIR)/$(SETS) -r 1.38)
	(cd $(BUILD) && ./erlay_wp_perf -n 1000 -d 10 -s 0 -u 100000 -e $(DATAOLDDIR)/diff_estimates_with_tow_10_128_9012.txt -i $(DATAOLDDIR)/$(SETS) -r 1.38 -R 3 -p $(PARAMDIR)/params_r3.csv -m 5.0)
	(cd $(BUILD) && ./erlay_wp_perf -n 1000 -d 10000 -s 18420 -u 100000 -e $(DATAOLDDIR)/diff_estimates_with_tow_10000_128_9012.txt -i $(DATAOLDDIR)/$(SETS) -r 1.38 -R 3 -p $(PARAMDIR)/params_r3.csv -m 5.0)


params:
	(cd $(TOP)/../PBS && make params)

perf: scripts params
	@echo "Benchmarking ..."
	(cd $(BUILD) && ./pinsketch_wp_experiments_diffs.sh)
	(cd $(BUILD) && ./pinsketch_experiments_diffs.sh)
	@echo "DONE!"

clean:
	$(RM) $(BUILD) 
