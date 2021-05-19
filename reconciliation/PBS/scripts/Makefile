CXX = g++
RM = gio trash -f 

CXXFLAGS = -O3   -Wall 
LDFLAGS = -lminisketch -lxxhash

TARGET = ITRecon ITReconAgainstGraphene PBS

all: $(TARGET)

ITRecon: ITRExperiments.cpp
	-$(CXX) $(CXXFLAGS) $<  $(LDFLAGS) -o $@

PBS: ITRExperiments_v2.cpp 
	-$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) 

ITReconAgainstGraphene: ITRExperiments_vs_Graphene.cpp
	-$(CXX) $(CXXFLAGS) $<  $(LDFLAGS) -o $@

clean:
	$(RM) $(TARGET)