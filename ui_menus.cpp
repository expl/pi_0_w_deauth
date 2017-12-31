#include "ui_menus.h"

void DisplayMenu::render_string_simple(
    Display *display, void *item, int row, bool focused
) {
    if (focused) {
        display->cursor->set(0, row);
        display->cursor->print("#", false);
    }
    
    display->cursor->set(2, row);
    display->cursor->print((const char *)item, false);
}

DisplayMenu::DisplayMenu(Display* display) {
    this->display = display;
    this->render_cb = DisplayMenu::render_string_simple;
    this->rows_per_item = 1;
    this->titled = true;
}

void DisplayMenu::render_title(int i, int count) {
    char number_i[12], number_c[12];
    int i_l = snprintf(number_i, 12, "(%d", i + 1);
    int i_c = snprintf(number_c, 12, "%d)", count);
    
    int center = this->display->cursor->chs_per_line / 2 + 1;
    
    this->display->cursor->set(0, 0);
    this->display->cursor->print("==========================", false);
    
    this->display->cursor->set(center - (1 + i_l), 0);
    this->display->cursor->print(number_i, false);
    
    this->display->cursor->set(center - 1, 0);
    this->display->cursor->print(" / ", false);
    
    this->display->cursor->set(center + 2, 0);
    this->display->cursor->print(number_c, false);
}

void DisplayMenu::get_selection(int& selected) {
    size_t items = this->items.size();
    int row_c = (this->display->cursor->row_count - (int)this->titled) /
        this->rows_per_item;
    int start = 0 + (int)this->titled;
    int pages = items / row_c;
    int focused = 0;
    int page = 0;
    int item_index = 0;
    int row_mod = items % row_c;
    int rows4page;
    selected = -1;
    
    if (!items)
        return;
    
    if (row_mod)
        pages++;
    
    while (1) {
        this->display->clear();
        
        this->render_title(page * row_c + focused, items);
        for (int i = 0; i != row_c; i++) {
            item_index = page * row_c + i;
            
            if (item_index == items)
                break;

            this->render_cb(
                this->display, this->items[item_index],
                i * this->rows_per_item + start, i == focused
            );
        }
        
        this->display->update();
        
        while (1) {
            if (!input_gpio(D_pin) || !input_gpio(R_pin)) {
                focused++;
                
                rows4page = page + 1 == pages ? row_mod ? row_mod : row_c : row_c;
                
                if (focused >= rows4page) {
                    focused = 0;
                    page++;
                    
                    if (page == pages) {
                        page = 0;
                    }
                }
                
                break;
            }
            else if (!input_gpio(U_pin) || !input_gpio(L_pin)) {
                focused--;
                
                if (focused < 0) {
                    page--;
                    
                    if (page == -1) {
                        page = pages - 1;
                    }
                    
                    rows4page = page + 1 == pages ? row_mod ? row_mod : row_c : row_c;
                    focused = rows4page - 1;
                }
                
                break;
            }
            else if (!input_gpio(A_pin)) {
                selected = page * row_c + focused;
                
                break;
            }
            else if (!input_gpio(B_pin)) {
                selected = -1;
                return;
            }
            
            usleep(10000);
        }
        
        if (selected != -1)
            break;
    }
    
    return;
}

RootMenu::RootMenu(Display* display): DisplayMenu(display) {
    this->items.push_back((void *)"Scan for networks");
    this->items.push_back((void *)"Shutdown");
}

void StationMenu::render_station(
    Display *display, void *item, int row, bool focused
) {
    Station *ap = (Station *)item;
    std::string mac = format_mac(ap->bssid);
    int start = row + 1;
    
    if (row != 0 && row != 1) {
        display->cursor->set(0, row);
        display->cursor->print("-------------------------", false);
    }
    
    if (focused) {
        display->cursor->set(0, start);
        display->cursor->print("#", false);
    }
    
    display->cursor->set(2, start);
    display->cursor->print("BSSID ", false);
    display->cursor->print((const char *)mac.c_str(), false);
    display->cursor->set(2, display->cursor->y + 1);
    display->cursor->print("SSID ", false);
    display->cursor->print((const char *)ap->ssid.c_str(), false);
}

StationMenu::StationMenu(Display* display): DisplayMenu(display) {
    this->render_cb = StationMenu::render_station;
    this->rows_per_item = 3;
    
    std::map <std::string, Station *>::iterator sit;
    
    for(sit = Globals::stations.begin(); sit != Globals::stations.end(); sit++) {
        this->items.push_back((void *)sit->second);
    }
}

Station* StationMenu::get_station() {
    int si;
    
    this->get_selection(si);
    
    if (si == -1)
        return (Station *)NULL;
    return (Station *)(this->items[si]);
}

void PeerMenu::render_peer(
    Display *display, void *item, int row, bool focused
) { 
    Peer *peer = (Peer *)item;
    std::string mac = format_mac(peer->mac_addr);
    std::string make = Globals::oui->get_make(peer->mac_addr);
    int start = row + 1;
    
    if (row != 0 && row != 1) {
        display->cursor->set(0, row);
        display->cursor->print("-------------------------", false);
    }
    
    if (focused) {
        display->cursor->set(0, start);
        display->cursor->print("#", false);
    }
    
    display->cursor->set(2, start);
    display->cursor->print((const char *)mac.c_str(), false);
    display->cursor->set(2, display->cursor->y + 1);
    display->cursor->print((const char *)make.c_str(), false);
}

PeerMenu::PeerMenu(
    Display* display, std::vector<Peer> peers
): DisplayMenu(display) {
    this->render_cb = PeerMenu::render_peer;
    this->rows_per_item = 3;
    
    std::vector<Peer>::iterator pit;
    
    for(pit = peers.begin(); pit != peers.end(); pit++) {
        this->items.push_back((void *)&(*pit));
    }
}

Peer* PeerMenu::get_peer() {
    int si;
    
    this->get_selection(si);
    
    if (si == -1)
        return (Peer *)NULL;
    return (Peer *)(this->items[si]);
}
