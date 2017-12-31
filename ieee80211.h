#ifndef IEEE80211_H
#define	IEEE80211_H

#define MY_MAC { 0x22, 0x22, 0x22, 0x22, 0x22, 0x22 }
#define BROADCAST_MAC { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }

// IEEE 802.11 header type masks
#define VERSION_M 0x03
#define TYPE_M 0x0c
#define SUBTYPE_M 0xF0

// IEEE 802.11 header frame types
#define MANAGEMENT_T 0x00
#define CONTROL_T 0x04
#define DATA_T 0x08

// IEEE 802.11 header frame subtypes
#define BEACON_T 0x80
#define PROBE_R_T 0x50
#define PROBE_Q_T 0x40
#define DEAUTH_T 0xC0
#define BLK_ACK_T 0x80
#define REQ_SND_T 0xB0

// IEEE 802.11 header flag masks
#define DS_M 0x03

// IEEE 802.11 header flag types
#define FROM_DS_T 0x02
#define TO_DS_T 0x01

// IEEE 802.11 header tag definitions
#define SSID_T 0x00
#define CHANNEL_T 0x03

// MAC address length
#define ETH_ALEN 6
// Maximum SSID length
#define SSID_LEN 32
// IEEE 802.11 header fixed tag segment size
#define FIXED_TAGS_SIZE 12

// Radio Tap header
struct ieee80211_radiotap_hdr {
	uint8_t it_version;
	uint8_t it_pad;
	uint16_t it_len;
};

// IEEE 802.11 header
struct ieee80211_hdr {
	uint16_t frame_control;
	uint16_t duration_id;
	uint8_t addr1[ETH_ALEN];
	uint8_t addr2[ETH_ALEN];
	uint8_t addr3[ETH_ALEN];
	uint16_t seq_ctrl;
};

void peer_scan_handler(u_char *, const struct pcap_pkthdr*, const u_char*);
void ap_scan_handler(u_char *, const struct pcap_pkthdr*, const u_char*);

void print_mac(uint8_t *);
bool cmp_macs(uint8_t *, uint8_t *);
std::string format_mac(uint8_t *);
int parse_channel(u_char *, size_t);
void parse_ssid(u_char *, size_t , char *);
void send_probe(pcap_t *);
void send_deauth(pcap_t *, uint8_t *, uint8_t *, bool, uint8_t);
#endif	/* IEEE80211_H */

