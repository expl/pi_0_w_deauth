CPPFLAGS=-pthread -I/usr/include/libnl3
LDFLAGS=-pthread
LDLIBS=-lpcap -lnl-3 -lnl-genl-3 -lstdc++

SRCS=main.cpp channel.cpp ieee80211.cpp station.cpp display.cpp gpio.cpp oui.cpp ui_menus.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: wifi

wifi: $(OBJS)
	c++ $(LDFLAGS) -o wifi $(OBJS) $(LDLIBS) 

main.o: main.cpp
	c++ $(CPPFLAGS) -c main.cpp

channel.o: channel.cpp
	c++ $(CPPFLAGS) -c channel.cpp
	
ieee80211.o: ieee80211.cpp
	c++ $(CPPFLAGS) -c ieee80211.cpp

station.o: station.cpp
	c++ $(CPPFLAGS) -c station.cpp

display.o: display.cpp
	c++ $(CPPFLAGS) -c display.cpp

gpio.o: gpio.cpp
	c++ $(CPPFLAGS) -c gpio.cpp
	
oui.o: oui.cpp
	c++ $(CPPFLAGS) -c oui.cpp

ui_menus.o: ui_menus.cpp
	c++ $(CPPFLAGS) -c ui_menus.cpp

clean:
	rm -f $(OBJS) wifi
