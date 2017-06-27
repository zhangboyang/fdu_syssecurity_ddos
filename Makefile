TARGET := ddos
CFLAGS := -g -Wall 
LDFLAGS :=

CC := gcc
CCLD := $(CC)

CFILES := $(wildcard *.c)
COBJS := $(notdir $(CFILES:.c=.o))


$(TARGET): $(COBJS)
	$(CCLD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -MMD -MP $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o *.d $(TARGET)

-include $(COBJS:.o=.d)
