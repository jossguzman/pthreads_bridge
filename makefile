SOURCEC=NarrowBridge.c

TARGETC=narrowBridge

.PHONY: all c cpp

all: c

c:
	$(CC) $(SOURCEC) -o $(TARGETC) -lpthread -lm

clean:
	-rm -f *.o
	#-rm -f *.txt
	-rm -f $(TARGETC)
