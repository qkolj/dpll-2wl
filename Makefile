SRC_DIR := src

HEADERS := $(wildcard $(SRC_DIR)/*.h)
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OUTPUT := dpll-2wl
OUTPUT_DB := dpll-2wl-debug

.PHONY: all
all: $(OUTPUT) $(OUTPUT_DB)

$(OUTPUT): $(HEADERS) $(SOURCES)
	g++ -Wall -o $@ $(SOURCES)

$(OUTPUT_DB): $(HEADERS) $(SOURCES)
	g++ -DDEBUG -Wall -o $@ $(SOURCES)

.PHONY: clean
clean:
	rm -f *.o $(OUTPUT) $(OUTPUT_DB)
