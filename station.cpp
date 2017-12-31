#include "main.h"
#include "station.h"
#include "display.h"

using namespace std;

Station::Station(
    uint8_t *bssid, u_char *ssid,
    size_t ssid_len,  uint8_t channel
) {
    memcpy(this->bssid, bssid, ETH_ALEN);
    
    if (!ssid_len) {
        this->ssid = string();
    } else {
        this->ssid = string((const char *)ssid, ssid_len);
    }
    
    this->channel = channel;
}

Station::~Station() {
}

void Station::print() {
    string mac = format_mac(this->bssid);

    display->cursor->print("BSSID ", false);
    display->cursor->print((const char *)mac.c_str(), false);
    display->cursor->set(0, display->cursor->y + 1);
    display->cursor->print("SSID ", false);
    display->cursor->print((const char *)this->ssid.c_str(), false);
    display->update();
}

void Station::add_peer(uint8_t *mac_addr) {
    if (this->has_peer(mac_addr))
        return;
    
    this->peers.push_back(Peer(this, mac_addr));
}

bool Station::has_peer(uint8_t *mac_addr) {
    vector<Peer>::iterator it = this->peers.begin();
    
    for (;it != this->peers.end(); it++) {
        if(cmp_macs((*it).mac_addr, mac_addr))
            return true;
    }
    
    return false;
}

Peer::Peer(Station *ap, uint8_t *addr) {
    this->ap = ap;
    memcpy(this->mac_addr, addr, ETH_ALEN);
}

void Peer::print() {
    cout << "Peer (";
    print_mac(this->mac_addr);
    cout << ')';
}