
// #include "system.h"
// #include "vMachine.h"
// #include "fileIo.h"
// #include "cmdline.h"
// #include "memory.h"
// #include "listing.h"

#include "common.h"

int main(int argc, char** argv) {

    _init_memory();
    cmd_line cl = create_cmd_line("This is the dis-assembler");
    add_str_param(cl, "ifile", "-i", "input file name", "output.bin", CF_NONE);
    add_num_param(cl, "verbose", "-v", "verbosity number from 0 to 10", 0, CF_NONE);
    parse_cmd_line(cl, argc, argv);

    initVM();
    loadVM(get_str_param(cl, "ifile"));

    showListing(stdout);
    if(get_num_param(cl, "verbose") >= 1) {
        dumpVars(stdout);
        dumpStrs(stdout);
    }

    _uninit_memory();
    return 0;
}