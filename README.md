# arp-guard
A linux kernel module to detect against arp attack 

Wiki : https://en.wikipedia.org/wiki/ARP_spoofing

arp-guard intercepts all ARP requests and responses. Each of these intercepted packets is verified for valid MAC address to IP address bindings before the local ARP cache is updated. Invalid ARP packets are dropped.

Determining the valid ARP packet is based on a cross-checking of ARP cache and a valid MAC address to IP address bindings stored in the DHCP snooping table which is built at runtime.