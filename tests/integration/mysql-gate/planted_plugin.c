/* A fake MariaDB Connector/C client plugin. run.sh plants copies where the
   connector could look for plugins on disk. If one is ever loaded, its
   constructor writes the MARKER file, and the gate fails. */
#include <stdio.h>

__attribute__((constructor)) static void planted_plugin_loaded(void) {
    FILE *marker = fopen(MARKER, "w");
    if (marker != NULL) {
        fputs("loaded\n", marker);
        fclose(marker);
    }
}
