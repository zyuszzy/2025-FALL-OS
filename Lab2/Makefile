# indicating that target "all" and "clean" are not files
.PHONY: all clean

# set some variables
CC = riscv64-linux-gnu-gcc
CFLAGS = -Wall -Wextra -Werror -O3 -Wpedantic -pthread -static

STUDENT_ID = 314551123
TARGET = sched_demo_$(STUDENT_ID)
SOURCE = sched_demo_$(STUDENT_ID).c
OBJS = $(SOURCE:.c=.o)

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)