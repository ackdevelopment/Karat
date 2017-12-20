#include<locale.h>
#include<wchar.h>
#include<time.h>

#include<unistd.h>

#include<karat/log.h>
#include<karat/alloc.h>
#include<karat/vm/vm.h>
#include<karat/mod.h>
#include<karat/debug.h>
#include<karat/parse/parse.h>
#include<karat/mod.h>

#define usage() \
    ({ printf("Usage: karat <file>\n"); exit(0); })

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    int c = EOF;
    bool lint = false;
    bool debug = false;
    const char *output = NULL;
    while((c = getopt(argc, argv, "o:ld")) != -1){
        switch(c){
        case 'o':
            output = optarg;
            break;
        case 'l':
            lint = true;
            break;
        case 'd':
            debug = true;
            break;
        case '?':
            usage();
            return 1;
        }
    }

    if(optind >= argc){
        fputs("expected a file argument\n", stderr);
        usage();
        return 1;
    }else if(optind != argc - 1){
        fputs("too many arguments\n", stderr);
        usage();
        return 1;
    }

    const char *prog = argv[optind];

    FILE *program = fopen(prog, "r");
    err_on(!program, "could not open %s", prog);

    int err = 0;
    printf("assembling \"%s\"\n", prog);
    const time_t start = clock();
    struct kprog *rprog = kprog_create();
    if(!parse_file(program, rprog)){
        printf("assembly took %lfms\n", ((double)clock()-start)/CLOCKS_PER_SEC);
        /* Run the program */
        if(lint){
            puts("linted successfully");
            goto done;
        }else if(debug){
            err = idebug(rprog, vm_opts(8096));
            goto done;
        }else if(output){
            printf("writing to %s...\n", output);
            FILE *out = fopen(output, "w");
            if(!out){
                fprintf(stderr, "unable to open output file %s\n", output);
                err = 1;
                goto done;
            }
            fwrite(rprog->program, rprog->prog_size, 1, out);
            fclose(out);
        }else{
            printf("running program (%lu bytes)...\n", rprog->prog_size);
            struct vm vm;
            vm_run(&vm, vm_opts(8096), rprog);
        } 
    }

done:
    kprog_destroy(rprog);
    fclose(program);
    return err;
}
