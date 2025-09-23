CC = gcc
CFLAGS = -I. -INRC -lm #flags ici à voir si pertients
EXECUTABLE = main_programme
SOURCES = main.c image.c nrc/nrio.c nrc/nralloc.c nrc/nrarith.c
SOURCESTEST = test.c image.c nrc/nrio.c nrc/nralloc.c nrc/nrarith.c

$(EXECUTABLE): $(SOURCES) 
	$(CC) -o $(EXECUTABLE) $(SOURCES) $(CFLAGS)

comparison: comparison_demo.c image.c nrc/nrio.c nrc/nralloc.c nrc/nrarith.c
	$(CC) -o comparison_demo comparison_demo.c image.c nrc/nrio.c nrc/nralloc.c nrc/nrarith.c $(CFLAGS)

run: 
	./$(EXECUTABLE)

demo:
	./comparison_demo.exe

clean:
	rm -f $(EXECUTABLE) comparison_demo.exe