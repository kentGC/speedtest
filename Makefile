#Makefile for speedtest
OBJS=speedtest.c
TARGET=speedtest
LIB=-lpthread
DEFINE=-DDBUG
CC=$(COMPILE_CROSS)gcc


$(TARGET):$(OBJS)
	$(CC) $^ -o $@ $(LIB) $(DEFINE)


clean:
	rm $(TARGET)
	
