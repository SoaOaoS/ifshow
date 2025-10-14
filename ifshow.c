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

void help() {
    printf("Usage:\n");
    printf("  ifshow -a                     # Show all interfaces\n");
    printf("  ifshow -i <interface_name>    # Show specific interface\n");
    printf("\nExamples:\n");
    printf("  ifshow -a\n");
    printf("  ifshow -i eth0\n\n");
}

static void show_all_interfaces(void) {
    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = (*ifa).ifa_next) { // ifa points to the first node, ifaddr, of the struct ifaddrs
        if (!ifa || !(*ifa).ifa_addr) continue; // skip if no no ifa or no address on ifa
        int family = (*ifa).ifa_addr->sa_family; // retrieve family address of ifa, either AF_INET or AF_INET6 (v4 or v6)
        if (family == AF_INET || family == AF_INET6) {
            char host[NI_MAXHOST];
            if (getnameinfo(
                    (*ifa).ifa_addr,
                    (family == AF_INET) ? (socklen_t)sizeof(struct sockaddr_in)
                                        : (socklen_t)sizeof(struct sockaddr_in6),
                    host, sizeof(host),
                    NULL, 0,
                    NI_NUMERICHOST) == 0) {
                printf("%s\t%s\n", (*ifa).ifa_name, host);
            }
        }
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
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = (*ifa).ifa_next) {
        if (!ifa || !(*ifa).ifa_addr) continue;
        if (strcmp((*ifa).ifa_name, target_ifname) != 0) continue;
        int family = (*ifa).ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {
            char host[NI_MAXHOST];
            if (getnameinfo(
                    (*ifa).ifa_addr,
                    (family == AF_INET) ? (socklen_t)sizeof(struct sockaddr_in)
                                        : (socklen_t)sizeof(struct sockaddr_in6),
                    host, sizeof(host),
                    NULL, 0,
                    NI_NUMERICHOST) == 0) {
                printf("%s\t%s\n", (*ifa).ifa_name, host);
                found = 1;
            }
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
        printf("Option '-a' detected: showing all interfaces...\n");
        show_all_interfaces();
    }

    else if (strcmp(argv[1], expected_argument_2) == 0) {
        if (argc != 3) {
            printf("Error: '-i' requires an interface name.\n\n");
            help();
            exit(EXIT_FAILURE);
        }
        printf("Option '-i' detected: showing interface '%s'...\n", argv[2]);
        show_single_interface(argv[2]);
    }

    else {
        printf("Unrecognized argument: '%s'. Please refer to the following:\n\n", argv[1]);
        help();
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
