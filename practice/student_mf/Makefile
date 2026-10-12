CC = gcc
CFLAGS = -Wall -Wextra -g
SRC = student.c main.c
OBJS = $(patsubst %.c,%.o,$(SRC))
DEP = $(patsubst %,.%.d,$(OBJS))

student: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

-include $(DEP)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MP -MF .$@.d

.PHONY: clean
clean:
	rm -f $(OBJS) $(DEP) student
