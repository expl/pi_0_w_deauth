/* 
 * File:   oui.cpp
 * Author: kostas
 * 
 * Created on October 3, 2017, 9:51 PM
 */

#include "oui.h"
#include <fstream>
#include <unistd.h>
#include <map>

#define OUI_TXT_FILE_PATH "/home/pi/pcap/oui.txt"
#define OUI_DB_FILE_PATH "/home/pi/pcap/oui.data"

using namespace std;

Oui::Oui() {
}

Oui::~Oui() {
}

void Oui::load_from_db() {
    FILE *fdb;
    
    fdb = fopen(OUI_DB_FILE_PATH, "r");
    
    if (fdb == (FILE *)NULL) {
        cout << "Failed reading from db file!\n";
        return;
    }
    
    this->oui_map.read_metadata(fdb);
    this->oui_map.read_nopointer_data(fdb);
    
    fclose(fdb);
}

void Oui::save_to_db() {
    FILE *fdb;
    
    fdb = fopen(OUI_DB_FILE_PATH, "w");
    
    if (fdb == (FILE *)NULL) {
        cout << "Failed writing to db file!\n";
        return;
    }
    
    this->oui_map.write_metadata(fdb);
    this->oui_map.write_nopointer_data(fdb);
    
    fclose(fdb);
}

void Oui::load_from_text() {
    char line[1024];
    oui_data oui_d;
    char hex[3];
    char *make_p;
    uint32_t oui = 0;
    uint8_t *oui_p = (uint8_t *)&oui;
    int ret;
    
    hex[3] = '\0';

    std::fstream fs (OUI_TXT_FILE_PATH, std::fstream::in);
    
    while (fs.getline(line, sizeof(line))) {
        int ii = 0;
        if (line[2] == '-') {
            make_p = &line[18];

            for (int i = 0; ii != OUI_MAKER_SIZE; i++) {
                if ((int)make_p[i] == 0) {
                    break;
                }
                
                if ((int)make_p[i] >= 48) {
                    oui_d.maker[ii++] = make_p[i];
                }
                else if (ii != 0 && oui_d.maker[ii - 1] != ' ') {
                    oui_d.maker[ii++] = ' ';
                }
            }
            oui_d.maker[ii - 1] = '\0';
            
            hex[0] = line[0];
            hex[1] = line[1];
            oui_p[0] = (uint8_t)strtol(hex, NULL, 16);
            
            hex[0] = line[3];
            hex[1] = line[4];
            oui_p[1] = (uint8_t)strtol(hex, NULL, 16);
            
            hex[0] = line[6];
            hex[1] = line[7];
            oui_p[2] = (uint8_t)strtol(hex, NULL, 16);
            
            this->oui_map[oui] = oui_d;
        }
    }
    
    if (ret == -1) {
        cout << "Failed to read oui text file!\n";
    }
    
    this->save_to_db();
    
    fs.close();
}

void Oui::load() {
    if (access(OUI_DB_FILE_PATH, R_OK) != 0)
        return this->load_from_text();
    
    this->load_from_db();
}

string Oui::get_make(uint8_t *mac_addr) {
    google::sparse_hash_map<uint32_t, oui_data>::iterator it;
    uint32_t oui = 0;
    
    memcpy((void *)&oui, mac_addr, 3);
    
    it = this->oui_map.find(oui);

    if (it != this->oui_map.end()) {
        return string((it->second).maker);
    }
    
    return string("Unknown");
}
