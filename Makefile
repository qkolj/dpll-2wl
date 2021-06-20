HEADERS := partial_valuation.h dpll.h
SOURCES := partial_valuation.cpp dpll.cpp main.cpp
OUTPUT := dpll-2wl

all: $(OUTPUT)

$(OUTPUT): $(HEADERS) $(SOURCES)
	g++ -o $@ $(SOURCES)
	
debug: partial_valuation.h partial_valuation.cpp dpll.h dpll.cpp main.cpp
	g++ -DDEBUG -o dpll $^

.PHONY: clean

clean:
	rm -f *.o $(OUTPUT)
