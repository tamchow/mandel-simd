RELEASE_FLAGS = -std=c11 -O3 -fopenmp -m64 -march=native -mavx -msahf -mcx16
DEBUG_FLAGS = -std=c11 -Wall -Wextra -g -O0 -fopenmp -m64
LDFLAGS = -lm

all : release debug

release : ./linux/x64/bin/release/mandel

debug : ./linux/x64/bin/debug/mandel

remake : clean all

./linux/x64/bin/release/mandel : mandel.c ./linux/x64/obj/release/palette.o ./linux/x64/obj/release/imageOutput.o ./linux/x64/obj/release/targa.o
	$(CC) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS)

./linux/x64/obj/release/palette.o : palette.c
	$(CC) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS) -c

./linux/x64/obj/release/imageOutput.o : imageOutput.c
	$(CC) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS) -c

./linux/x64/obj/release/targa.o : targa.c
	$(CC) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS) -c

./linux/x64/bin/debug/mandel : mandel.c ./linux/x64/obj/debug/palette.o ./linux/x64/obj/debug/imageOutput.o ./linux/x64/obj/debug/targa.o
	$(CC) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS)

./linux/x64/obj/debug/palette.o : palette.c
	$(CC) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS) -c

./linux/x64/obj/debug/imageOutput.o : imageOutput.c
	$(CC) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS) -c

./linux/x64/obj/debug/targa.o : targa.c
	$(CC) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS) -c

clean :
	$(RM) ./linux/x64/bin/release/mandel  ./linux/x64/obj/release/palette.o ./linux/x64/obj/release/imageOutput.o ./linux/x64/obj/release/targa.o \
	./linux/x64/bin/debug/mandel ./linux/x64/obj/debug/palette.o ./linux/x64/obj/debug/imageOutput.o ./linux/x64/obj/debug/targa.o

.PHONY : all release debug remake clean
