CC = gcc
CFLAGS = -I. -INRC -lm #flags ici à voir si pertients
TARGET = main_programme
SOURCES = main.c image.c nrc/nrio.c nrc/nralloc.c nrc/nrarith.c

$(TARGET): $(SOURCES)
	$(CC) -o $(TARGET) $(SOURCES) $(CFLAGS)

run: 
	./$(TARGET)
clean:
	rm -f $(TARGET).exe