/*
 * ifshow - Lists network interfaces and their IP addresses.
 * FR: ifshow - Liste les interfaces réseau et leurs adresses IP.
 *
 * Usage / Utilisation:
 * - ifshow -a. EN: Show all interfaces with IPv4/IPv6. FR: Affiche toutes les interfaces avec IPv4/IPv6.
 * - ifshow -i <name>. EN: Show only the specified interface. FR: Affiche uniquement l'interface spécifiée.
 */

#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

// Convert a sockaddr (IPv4/IPv6) to a numeric host string.
static int addr_to_string(const struct sockaddr *sa, char *buf, size_t buflen) {
    if (!sa) return -1;
    int family = sa->sa_family;
    return getnameinfo(
        sa,
        (family == AF_INET) ? (socklen_t)sizeof(struct sockaddr_in)
                            : (socklen_t)sizeof(struct sockaddr_in6),
        buf, (socklen_t)buflen,
        NULL, 0,
        NI_NUMERICHOST);
}

// Count prefix length from a netmask sockaddr. Works for IPv4 and IPv6.
// Returns -1 if input is NULL or family unsupported.
static int count_prefix_length(const struct sockaddr *netmask) {
    if (!netmask) return -1;
    if (netmask->sa_family == AF_INET) {
        const struct sockaddr_in *nm4 = (const struct sockaddr_in *)netmask;
        uint32_t m = ntohl(nm4->sin_addr.s_addr);
        int count = 0;
        for (int i = 31; i >= 0; --i) {
            if ((m >> i) & 1U) count++; else break; // stop at first 0 from MSB
        }
        return count;
    } else if (netmask->sa_family == AF_INET6) {
        const struct sockaddr_in6 *nm6 = (const struct sockaddr_in6 *)netmask;
        int count = 0;
        for (int i = 0; i < 16; ++i) {
            unsigned char b = nm6->sin6_addr.s6_addr[i];
            for (int bit = 7; bit >= 0; --bit) {
                if ((b >> bit) & 1U) count++; else return count; // stop at first 0
            }
        }
        return count;
    }
    return -1;
}

// (previous single-line formatter removed in favor of grouped bullet output)

void help() {
    printf("Usage:\n");
    printf("  ifshow -a                     # Show all interfaces\n");
    printf("  ifshow -i <interface_name>    # Show specific interface\n");
    printf("\nExamples:\n");
    printf("  ifshow -a\n");
    printf("  ifshow -i eth0\n");
    printf("\nNotes:\n");
    printf("  Addresses include netmask as address/prefix.\n");
    printf("  IPv4 also shows dotted mask in parentheses.\n\n");
}

// Print a header for an interface, e.g., "eth0:".
static void print_interface_header(const char *ifname) {
    if (ifname && *ifname) {
        printf("%s:\n", ifname);
    }
}

// Print a bullet line for an address with prefix and optional dotted mask.
static void print_address_bullet(const struct sockaddr *addr, const struct sockaddr *netmask) {
    if (!addr) return;
    char addr_str[NI_MAXHOST] = {0};
    char mask_str[NI_MAXHOST] = {0};
    int family = addr->sa_family;
    if (addr_to_string(addr, addr_str, sizeof(addr_str)) != 0) return;
    int prefix = count_prefix_length(netmask);
    if (family == AF_INET && netmask && addr_to_string(netmask, mask_str, sizeof(mask_str)) == 0 && prefix >= 0) {
        printf(" - %s/%d (%s)\n", addr_str, prefix, mask_str);
    } else if (prefix >= 0) {
        printf(" - %s/%d\n", addr_str, prefix);
    } else {
        printf(" - %s\n", addr_str);
    }
}

static void show_all_interfaces(void) {
    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    // Collect unique interface names with IP addresses (simple fixed-size list to stay lightweight).
    const int MAX_IFS = 256;
    const char *names[MAX_IFS];
    int name_count = 0;

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = (*ifa).ifa_next) {
        if (!ifa || !(*ifa).ifa_addr) continue;
        int family = (*ifa).ifa_addr->sa_family;
        if (family != AF_INET && family != AF_INET6) continue;
        // Check if name already recorded
        int seen = 0;
        for (int i = 0; i < name_count; ++i) {
            if (strcmp(names[i], (*ifa).ifa_name) == 0) { seen = 1; break; }
        }
        if (!seen && name_count < MAX_IFS) {
            names[name_count++] = (*ifa).ifa_name; // pointer valid while ifaddr is alive
        }
    }

    // Print grouped output
    for (int i = 0; i < name_count; ++i) {
        const char *ifname = names[i];
        print_interface_header(ifname);
        for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = (*ifa).ifa_next) {
            if (!ifa || !(*ifa).ifa_addr) continue;
            if (strcmp((*ifa).ifa_name, ifname) != 0) continue;
            int family = (*ifa).ifa_addr->sa_family;
            if (family == AF_INET || family == AF_INET6) {
                print_address_bullet((*ifa).ifa_addr, (*ifa).ifa_netmask);
            }
        }
        printf("\n");
    }

    freeifaddrs(ifaddr);
}

static void show_single_interface(const char *target_ifname) {
    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    int found = 0;
    print_interface_header(target_ifname);
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = (*ifa).ifa_next) {
        if (!ifa || !(*ifa).ifa_addr) continue;
        if (strcmp((*ifa).ifa_name, target_ifname) != 0) continue;
        int family = (*ifa).ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {
            print_address_bullet((*ifa).ifa_addr, (*ifa).ifa_netmask);
            found = 1;
        }
    }

    freeifaddrs(ifaddr);
    if (!found) {
        printf("Interface '%s' not found or has no IP.\n", target_ifname);
    }
}

int main(int argc, char *argv[]) {

    const char *expected_argument_1 = "-a";
    const char *expected_argument_2 = "-i";

    if (argc < 2 || argc > 3) {
        printf("\nUnrecognized number of arguments. Please refer to the following:\n\n");
        help();
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], expected_argument_1) == 0) {
        if (argc != 2) {
            printf("Error: '-a' must be used alone.\n\n");
            help();
            exit(EXIT_FAILURE);
        }
        show_all_interfaces();
    }

    else if (strcmp(argv[1], expected_argument_2) == 0) {
        if (argc != 3) {
            printf("Error: '-i' requires an interface name.\n\n");
            help();
            exit(EXIT_FAILURE);
        }
        show_single_interface(argv[2]);
    }

    else {
        printf("Unrecognized argument: '%s'. Please refer to the following:\n\n", argv[1]);
        help();
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
