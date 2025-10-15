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

/**
 * @brief Convert a socket address to a numeric host string.
 *
 * Converts an IPv4 or IPv6 `sockaddr` to its numeric string representation
 * using `inet_ntop` (no DNS lookups involved).
 *
 * @param sa      Pointer to source socket address (IPv4/IPv6). Must not be NULL.
 * @param buf     Output buffer to receive the numeric host string.
 * @param buflen  Size (in bytes) of the output buffer.
 * @return 0 on success; -1 on failure (NULL args, unsupported family, or buffer too small).
 */
static int addr_to_string(const struct sockaddr *sa, char *buf, size_t buflen) {
    if (!sa || !buf || buflen == 0) return -1;
    switch (sa->sa_family) {
        case AF_INET: {
            const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
            return inet_ntop(AF_INET, &sin->sin_addr, buf, (socklen_t)buflen) ? 0 : -1;
        }
        case AF_INET6: {
            const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
            return inet_ntop(AF_INET6, &sin6->sin6_addr, buf, (socklen_t)buflen) ? 0 : -1;
        }
        default:
            return -1;
    }
}

/**
 * @brief Compute CIDR prefix length from a netmask sockaddr.
 *
 * Supports IPv4 and IPv6 netmasks. For IPv4, counts leading 1-bits in the
 * 32-bit mask. For IPv6, counts leading 1-bits across the 128-bit mask.
 *
 * @param netmask  Pointer to a netmask `sockaddr` (AF_INET or AF_INET6).
 * @return Prefix length (0..32 for IPv4, 0..128 for IPv6), or -1 if `netmask`
 *         is NULL or the address family is unsupported.
 */
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

/**
 * @brief Print CLI usage instructions to stdout.
 *
 * Describes available options and examples for using the program. Does not
 * exit; callers decide flow control.
 */
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

/**
 * @brief Print an interface header line.
 *
 * Prints the interface name followed by a colon (e.g., "eth0:") if the
 * provided name is non-empty.
 *
 * @param ifname  Interface name string. May be NULL or empty; prints nothing.
 */
static void print_interface_header(const char *ifname) {
    if (ifname && *ifname) {
        printf("%s:\n", ifname);
    }
}

/**
 * @brief Print a bullet line for an address.
 *
 * Renders a line in the form " - <addr>/<prefix>". For IPv4, also prints the
 * dotted mask in parentheses, " - 192.0.2.10/24 (255.255.255.0)".
 *
 * @param addr     Pointer to the address `sockaddr` (IPv4 or IPv6). If NULL, no output.
 * @param netmask  Optional pointer to the netmask `sockaddr` used to compute
 *                 the prefix length. May be NULL.
 */
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

/**
 * @brief Enumerate and display all interfaces with their IP addresses.
 *
 * Uses `getifaddrs` to collect interface entries, groups them by interface
 * name, and prints IPv4/IPv6 addresses with prefixes. Exits the process on
 * `getifaddrs` failure.
 */
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

/**
 * @brief Display IP addresses for a specific interface.
 *
 * Prints IPv4/IPv6 addresses (with prefixes) for the given interface name.
 * If the interface is not found or has no IP addresses, prints a message, not stderr.
 * Exits the process on `getifaddrs` failure.
 * 
 * Looks relly the same as "show_all_interfaces() function detailed above
 *
 * @param target_ifname  Name of the interface to display.
 */
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

/**
 * @brief Program entry point.
 *
 * Parses command-line arguments and dispatches to the appropriate action:
 *  - `-a` to list all interfaces
 *  - `-i <name>` to list a specific interface
 * On invalid usage, prints help and exits with failure.
 *
 * @param argc  Argument count.
 * @param argv  Argument vector.
 * @return `EXIT_SUCCESS` on success; otherwise exits the process with failure.
 */
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