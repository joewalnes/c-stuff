.PHONY: all clean

all: mcast-listen mcast-send

clean:
	rm -f mcast-listen mcast-send
