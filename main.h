#ifndef MAIN_H
#define	MAIN_H

#include <iostream>
#include <ctime>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include <map>
#include <vector>
#include <array>

#include <pthread.h>

#include "station.h"
#include "oui.h"

#define SCAN_INTERVAL 200
#define WDEV "wlan0"

#define L_pin 27 
#define R_pin 23 
#define C_pin 4 
#define U_pin 17 
#define D_pin 22 

#define A_pin 5 
#define B_pin 6 

class Display;

namespace Globals {
    extern std::map <std::string, Station *> stations;
    extern Oui *oui;
    extern pcap_t *pcap_descr;
    extern Display *display;
}

#endif	/* MAIN_H */

