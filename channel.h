#ifndef CHANNEL_H
#define	CHANNEL_H

#include <linux/wireless.h>
#include <linux/if_ether.h>

#include <linux/nl80211.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/genetlink.h>
#include <linux/sockios.h>

int ieee80211_channel_to_frequency(int chan);

class Channel {
private:
    struct nl_sock *nl_sock;
    struct nl_cache *nl_cache;
    struct genl_family *nl80211;
    
public:
    Channel();
    ~Channel();
    int set_channel(int);
    int get_channel();
};

#endif	/* CHANNEL_H */

