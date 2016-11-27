C=gcc
CFLAGS=-g -Wall -O2
LDFLAGS=-L/usr/lib/
#LIBS=-L/usr/lib/ -lmysqlclient #-lpthread
#INCLUDES= -I./mysql-connector-c-6.0.2-linux-rhel5
TARGET_SENDER = netstateSender
all:$(TARGET_SENDER)

$(TARGET_SENDER): utils.o
	$(CC) -o $@ $(INCLUDES) $(CFLAGS) utils.o main.c $(LIBS)

utils.o: utils.c
	$(CC) $(CFLAGS) $(INCLUDES) -c utils.c

clean:
	rm -f *.o $(TARGET_SENDER)
