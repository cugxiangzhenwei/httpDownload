CC = g++
CFLAGS = -g -Wall -W
OBJS = httpDownload.o httpCommon.o progress_bar.o
httpdown:$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o httpdown -lpthread

httpCommon.o:httpCommon.cpp
	$(CC) $(CFLAGS) -c httpCommon.cpp

httpDownload.o: httpDownload.cpp
	$(CC) $(CFLAGS) -c httpDownload.cpp

progress_bar.o: progress_bar.cpp progress_bar.h
	$(CC) $(CFLAGS) -c progress_bar.cpp

clean:
	rm httpdown $(OBJS)

dist:
	tar -jcvf ./httpDownload_1.0.1.tar.bz2 *.cpp *.h  Makefile

distclean:
	rm ./httpDownload_1.0.1.tar.bz2 ./httpdown $(OBJS)
