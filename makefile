CC = gcc
CFLAGS = -I. -INRC -lm #flags ici à voir si pertients
EXECUTABLE = main_programme
SOURCES = main.c image.c nrc/nrio.c nrc/nralloc.c nrc/nrarith.c
SOURCESTEST = test.c image.c nrc/nrio.c nrc/nralloc.c nrc/nrarith.c

$(EXECUTABLE): $(SOURCESTEST) 
	$(CC) -o $(EXECUTABLE) $(SOURCESTEST) $(CFLAGS)

run: 
	./$(EXECUTABLE)
clean:
	rm -f $(EXECUTABLE)