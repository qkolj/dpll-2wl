HEADERS := partial_valuation.h dpll.h
SOURCES := partial_valuation.cpp dpll.cpp main.cpp
OUTPUT := dpll-2wl

all: $(OUTPUT)

$(OUTPUT): $(HEADERS) $(SOURCES)
	g++ -o $@ $(SOURCES)
	
debug: $(HEADERS) $(SOURCES)
	g++ -DDEBUG -o $(OUTPUT)-debug $(SOURCES)

.PHONY: clean

clean:
	rm -f *.o $(OUTPUT)
