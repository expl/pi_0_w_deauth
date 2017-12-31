#ifndef STATION_H
#define	STATION_H
#include <map>
#include "ieee80211.h"

class Station;

class Peer {
public:
    Station *ap;
    uint8_t mac_addr[ETH_ALEN];
    
    Peer(Station *, uint8_t *);
    
    void print();
};

class Station {
public:
    std::string ssid;
    uint8_t bssid[ETH_ALEN];
    int channel;
    std::vector<Peer> peers;

    Station(uint8_t *, u_char *, size_t, uint8_t);
    virtual ~Station();
    
    void print();
    bool has_peer(uint8_t *);
    void add_peer(uint8_t *);
};

#endif	/* STATION_H */
