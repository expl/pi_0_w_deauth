#include "main.h"
#include "channel.h"
#include "ieee80211.h"

using namespace std;

static int getChannelFromFrequency(int frequency) {
    if (frequency >= 2412 && frequency <= 2472)
        return (frequency - 2407) / 5;
    else if (frequency == 2484)
        return 14;

    else if (frequency >= 4920 && frequency <= 6100)
        return (frequency - 5000) / 5;
    else
        return -1;
}

int ieee80211_channel_to_frequency(int chan) {
    if (chan < 14)
        return 2407 + chan * 5;

    if (chan == 14)
        return 2484;

    return (chan + 1000) * 5;
}

int Channel::get_channel() {
    struct iwreq wrq;
    int fd, frequency;
    int chan = 0;
    
    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    memset(&wrq, 0, sizeof(struct iwreq) );

    strncpy(wrq.ifr_name, WDEV, sizeof(WDEV));

    if( ioctl( fd, SIOCGIWFREQ, &wrq ) < 0 ) {
        close(fd);
        return -1;
    }

    frequency = wrq.u.freq.m;
    if (frequency > 100000000)
        frequency /= 100000;
    else if (frequency > 1000000)
        frequency /= 1000;

    if (frequency > 1000)
        chan = getChannelFromFrequency(frequency);
    else chan = frequency;
    
    close(fd);

    return chan;
}

static unsigned int if_nametoindex(const char *ifname) {
    int index;
    int ctl_sock;
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    index = 0;
    if ((ctl_sock = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
        if (ioctl(ctl_sock, SIOCGIFINDEX, &ifr) >= 0) {
            index = ifr.ifr_ifindex;
        }
        close(ctl_sock);
    }
    return index;
}

Channel::Channel() {
    this->nl_sock = nl_socket_alloc();

    if (!this->nl_sock) {
        fprintf(stderr, "Failed to allocate netlink socket.\n");
        return;
    }

    if (genl_connect(this->nl_sock)) {
        fprintf(stderr, "Failed to connect to generic netlink.\n");
        goto out_handle_destroy;
    }

    if (genl_ctrl_alloc_cache(this->nl_sock, &this->nl_cache)) {
        fprintf(stderr, "Failed to allocate generic netlink cache.\n");
        goto out_handle_destroy;
    }

    this->nl80211 = genl_ctrl_search_by_name(this->nl_cache, "nl80211");
    if (!this->nl80211) {
        fprintf(stderr, "nl80211 not found.\n");
        goto out_cache_free;
    }

    return;

 out_cache_free:
    nl_cache_free(this->nl_cache);
    this->nl_cache = (struct nl_cache *) NULL;
 out_handle_destroy:
    nl_socket_free(this->nl_sock);
    this->nl_sock = (struct nl_sock *) NULL;
}

Channel::~Channel() {
    if (this->nl_cache != (struct nl_cache *) NULL)
        nl_cache_free(this->nl_cache);
    if (this->nl_sock != (struct nl_sock *) NULL)
        nl_socket_free(this->nl_sock);
}

int Channel::set_channel(int channel) {
    unsigned int devid;
    struct nl_msg *msg;
    unsigned int freq;
    unsigned int htval = NL80211_CHAN_NO_HT;
    struct iwreq wrq;
    
    devid = if_nametoindex(WDEV);
    freq = ieee80211_channel_to_frequency(channel);
    msg = nlmsg_alloc();
    
    if (!msg) {
        fprintf(stderr, "failed to allocate netlink message\n");
        return 2;
    }

    genlmsg_put(msg, 0, 0, genl_family_get_id(this->nl80211), 0,
            0, NL80211_CMD_SET_WIPHY, 0);

    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devid);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, htval);

    nl_send_auto_complete(this->nl_sock, msg);
    nlmsg_free(msg);

    return 0;
    
    nla_put_failure:
        return -ENOBUFS;
}
