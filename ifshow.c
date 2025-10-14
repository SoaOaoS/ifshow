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

int main(int argc, char *argv[]) {

    const char *expected_argument_1 = "-a";
    const char *expected_argument_2 = "-i";

    if (argc < 2 || argc > 3) {
        printf("\nUnrecognized number of arguments. Please refer to the following:\n\n");
        help();
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], expected_argument_1) == 0) {
        // -a must be alone
        if (argc != 2) {
            printf("Error: '-a' must be used alone.\n\n");
            help();
            exit(EXIT_FAILURE);
        }
        printf("Option '-a' detected: showing all interfaces...\n");
        // TODO: logique pour afficher toutes les interfaces ici
    }

    else if (strcmp(argv[1], expected_argument_2) == 0) {
        // -i must have an interface name
        if (argc != 3) {
            printf("Error: '-i' requires an interface name.\n\n");
            help();
            exit(EXIT_FAILURE);
        }
        printf("Option '-i' detected: showing interface '%s'...\n", argv[2]);
        // TODO: logique pour afficher l'interface spécifiée ici
    }

    else {
        printf("Unrecognized argument: '%s'. Please refer to the following:\n\n", argv[1]);
        help();
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
