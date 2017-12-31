#ifndef UI_MENUS_H
#define	UI_MENUS_H

#include "main.h"
#include "display.h"
#include "gpio.h"

class DisplayMenu {
public:
    DisplayMenu(Display* display);
    Display *display;
    std::vector<void *> items;
    void (*render_cb)(Display *, void *, int, bool);
    bool titled;
    int rows_per_item;
    
    void render_title(int, int);
    void get_selection(int&);
    
    static void render_string_simple(Display *, void *, int, bool);
};

class RootMenu: public DisplayMenu {
public:
    RootMenu(Display *display);
};

class StationMenu: public DisplayMenu {
public:
    StationMenu(Display *display);
    
    Station* get_station();
    static void render_station(Display *, void *, int, bool);
};

class PeerMenu: public DisplayMenu {
public:
    PeerMenu(Display *display, std::vector<Peer> peers);
    
    Peer* get_peer();
    static void render_peer(Display *, void *, int, bool);
};

#endif	/* UI_MENUS_H */
