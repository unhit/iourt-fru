CC = g++
CCFLAGS = -c -Wall -O2 -fno-strict-aliasing -pipe

all: ip2loc-csv-parser ip2loc-server 

IOSers.o: IOSers.cc IOSers.h
	${CC} ${CCFLAGS} IOSers.cc -o IOSers.o

IP2LocCSVParser.o: IP2LocCSVParser.cc IOSers.h IP2LocCSVParser.h
	${CC} ${CCFLAGS} IP2LocCSVParser.cc -o IP2LocCSVParser.o

IP2LocLookup.o: IP2LocLookup.cc IP2LocLookup.h Location.h IOSers.h
	${CC} ${CCFLAGS} IP2LocLookup.cc -o IP2LocLookup.o

IP2LocServer.o: IP2LocServer.cc IP2LocServer.h IP2LocLookup.h Location.h
	${CC} ${CCFLAGS} IP2LocServer.cc -o IP2LocServer.o

Location.o: Location.cc Location.h IP2LocLookup.h
	${CC} ${CCFLAGS} Location.cc -o Location.o

ip2loc-csv-parser.o: ip2loc-csv-parser.cc IP2LocCSVParser.h
	${CC} ${CCFLAGS} ip2loc-csv-parser.cc -o ip2loc-csv-parser.o

ip2loc-server.o: ip2loc-server.cc IP2LocServer.h IP2LocLookup.h Location.h
	${CC} ${CCFLAGS} ip2loc-server.cc -o ip2loc-server.o

ip2loc-csv-parser: ip2loc-csv-parser.o IP2LocCSVParser.o IOSers.o
	${CC} ip2loc-csv-parser.o IP2LocCSVParser.o IOSers.o -o ip2loc-csv-parser

ip2loc-server: ip2loc-server.o Location.o IP2LocServer.o IP2LocLookup.o IOSers.o
	${CC} ip2loc-server.o Location.o IP2LocServer.o IP2LocLookup.o IOSers.o -o ip2loc-server -lpthread

clean:
	rm -f ip2loc-csv-parser ip2loc-server *.o

depend:
	${CC} -E -MM *.cc > .depend
