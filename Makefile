CFLAGS = -std=c11 -Wall -Wextra -O3 -Ofast -fopenmp

mandel.x86 : mandel.c mandel_color.o mandel_palette.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

mandel_color.o : color.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

mandel_palette.o : palette.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean :
	$(RM) mandel.x86 mandel_color.o mandel_palette.o

.PHONY : clean
