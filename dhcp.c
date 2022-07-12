#include "dhcp.h"
#include "errno.h"

DEFINE_SPINLOCK(slock);

LIST_HEAD(dhcp_snooping_list);

void insert_dhcp_snooping_entry(u8 *mac, u32 ip, u32 lease_time, u32 expire_time) {
    struct dhcp_snooping_entry *entry;

    entry = kmalloc(sizeof(struct dhcp_snooping_entry), GFP_KERNEL);
    entry->ip = ip;
    entry->lease_time = lease_time;
    entry->expires = expire_time;
    memcpy(entry->mac, mac, ETH_ALEN);
    
    spin_lock(&slock);
    
    list_add(&entry->list, &dhcp_snooping_list);
    
    spin_unlock(&slock);
}


struct dhcp_snooping_entry *find_dhcp_snooping_entry(u32 ip) {
    struct list_head* curr,*next;
    struct dhcp_snooping_entry *entry;

    spin_lock(&slock);
    list_for_each_safe(curr, next, &dhcp_snooping_list) {
        entry = list_entry(curr, struct dhcp_snooping_entry, list);
        if (entry->ip == ip) {
            spin_unlock(&slock);
            return entry;
        }
    }
    spin_unlock(&slock);
    return NULL;
}


void delete_dhcp_snooping_entry(u32 ip) {
    struct dhcp_snooping_entry *entry = find_dhcp_snooping_entry(ip);

    if (entry) {
        spin_lock(&slock);
        list_del(&entry->list);
        kfree(entry);
        spin_unlock(&slock);
    }   
}


void clean_dhcp_snooping_table(void) {
    struct list_head* curr, *next;
    struct dhcp_snooping_entry *entry;

    spin_lock(&slock);
    list_for_each_safe(curr, next, &dhcp_snooping_list) {
        entry = list_entry(curr, struct dhcp_snooping_entry, list);
        list_del(&entry->list);
        kfree(entry);
    }
    spin_unlock(&slock);
}


int dhcp_thread_handler(void *arg) {
    struct list_head* curr, *next;
    struct dhcp_snooping_entry *entry;
    struct timespec ts;

    while(!kthread_should_stop()) {
        getnstimeofday(&ts);
        spin_lock(&slock);
        list_for_each_safe(curr, next, &dhcp_snooping_list) {
            entry = list_entry(curr, struct dhcp_snooping_entry, list);
            if (ts.tv_sec >= entry->expires) {
                printk(KERN_INFO "arpguard:  %pI4 released on %ld\n", &entry->ip, ts.tv_sec);
                list_del(&entry->list);
                kfree(entry);
            }
        }
        spin_unlock(&slock);
        msleep(1000);
    }
    return 0;
}


int dhcp_is_valid(struct sk_buff* skb) {
    int status = SUCCESS;
    const struct udphdr* udp;
    const struct dhcp* payload;
    u8 dhcp_packet_type;
    const struct ethhdr* eth;
    unsigned char shaddr[ETH_ALEN];

    eth = eth_hdr(skb);
    memcpy(shaddr, eth->h_source, ETH_ALEN);

    udp = udp_hdr(skb);
    payload = (struct dhcp *) ((unsigned char *)udp + sizeof(struct udphdr));
    
    memcpy(&dhcp_packet_type, &payload->bp_options[2], 1);

    if (dhcp_packet_type == DHCP_DISCOVER || dhcp_packet_type == DHCP_REQUEST) {
        if (memcmp(payload->chaddr, shaddr, ETH_ALEN) != 0) {
            printk(KERN_ERR "arpguard:  the client MAC address %pM in the message body is NOT identical to the source MAC address in the Ethernet header %pM\n", payload->chaddr, shaddr);
            return -EHWADDR;
        }
    }
    
    if (payload->giaddr != 0) {
        printk(KERN_ERR "arpguard:  GW ip address is not zero\n");
        return -EIPADDR;
    }
    
    return status;
}
