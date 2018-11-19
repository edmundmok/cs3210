# Source: https://devblogs.nvidia.com/separate-compilation-linking-cuda-device-code/
OBJECTS = main.o hash.o utils.o
TARGET = 0123Homework

CC = nvcc
ARCHFLAG = -arch=sm_30
CFLAGS = -x cu $(ARCHFLAG) -I. -dc
LDFLAGS = $(ARCHFLAG)

all: $(TARGET)

# Link object files into executable
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(TARGET)

# Automatic Variables: https://www.gnu.org/software/make/manual/make.html#Automatic-Variables
%.o: %.cu
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(TARGET)
