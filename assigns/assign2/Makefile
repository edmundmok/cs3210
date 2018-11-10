# Source: https://devblogs.nvidia.com/separate-compilation-linking-cuda-device-code/
OBJECTS = main.o hash.o
MAIN = 0123Homework

CC = nvcc
ARCHFLAG = -arch=sm_30
CFLAGS = -x cu $(ARCHFLAG) -I. -dc
LDFLAGS = $(ARCHFLAG)

# Link object files into executable
all: $(OBJECTS)
        $(CC) $(LDFLAGS) $(OBJECTS) -o $(MAIN)

# Automatic Variables: https://www.gnu.org/software/make/manual/make.html#Automatic-Variables
%.o: %.cu
        $(CC) $(CFLAGS) $< -o $@

clean:
        rm -f *.o $(MAIN)