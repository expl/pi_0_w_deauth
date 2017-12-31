/* 
 * File:   oui.h
 * Author: kostas
 *
 * Created on October 3, 2017, 9:51 PM
 */
#include <google/sparse_hash_map>
#include "main.h"

#ifndef OUI_H
#define	OUI_H

#define OUI_MAKER_SIZE 64

struct oui_data {
    char maker[OUI_MAKER_SIZE];
};

class Oui {
private:
    google::sparse_hash_map<uint32_t, oui_data> oui_map;
    
    void load_from_db();
    void load_from_text();
    void save_to_db();
public:
    Oui();
    ~Oui();
    
    void load();
    std::string get_make(uint8_t *mac_addr);
};

#endif	/* OUI_H */

