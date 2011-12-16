/*
** slt
** The Sleuth Kit 
**
** Given an image and directory inode, display the file names and 
** directories that exist (both active and deleted)
**
** Brian Carrier [carrier <at> sleuthkit [dot] org]
** Copyright (c) 2006-2008 Brian Carrier, Basis Technology.  All Rights reserved
** Copyright (c) 2003-2005 Brian Carier.  All rights reserved
**
** TASK
** Copyright (c) 2002 @stake Inc.  All rights reserved
**
** TCTUTILs
** Copyright (c) 2001 Brian Carrier.  All rights reserved
**
**
** This software is distributed under the Common Public License 1.0
**
*/
#include "tsk3/tsk_tools_i.h"
#include "tsk3/fs/tsk_fs_i.h"
#include <locale.h>
#include <time.h>

static TSK_TCHAR *progname;
static TSK_DADDR_T detect_partition_offset(TSK_IMG_INFO *img);
static int process(int argc, char **argv1);
static uint8_t tsk_fs_fls2(TSK_FS_INFO * fs, TSK_FS_FLS_FLAG_ENUM lclflags,
    TSK_INUM_T inode, TSK_FS_DIR_WALK_FLAG_ENUM flags, TSK_TCHAR * tpre,
    int32_t skew);
static uint8_t tsk_fs_icat2(TSK_FS_INFO * fs, TSK_INUM_T inum,
    TSK_FS_ATTR_TYPE_ENUM type, uint8_t type_used,
    uint16_t id, uint8_t id_used, TSK_FS_FILE_WALK_FLAG_ENUM flags);
static uint8_t
tsk_fs_icat3(TSK_FS_INFO * fs, TSK_FS_FILE_WALK_FLAG_ENUM flags);

static uint8_t tsk_get_os_info(TSK_FS_INFO * fs);
void
usage()
{
    TFPRINTF(stderr,
        _TSK_T
        ("usage: %s [-acdDFlpruvV] [-f fstype] [-i imgtype] [-b dev_sector_size] [-m dir/] [-o imgoffset] [-z ZONE] [-s seconds] [-I inode] image [images] \n"),
        progname);
    tsk_fprintf(stderr,
        "\tIf [inode] is not given, the root directory is used\n");
    tsk_fprintf(stderr, "\t-a: Display \".\" and \"..\" entries\n");
    tsk_fprintf(stderr, "\t-c: icat the inode\n");
    tsk_fprintf(stderr, "\t-d: Display deleted entries only\n");
    tsk_fprintf(stderr, "\t-D: Display only directories\n");
    tsk_fprintf(stderr, "\t-F: Display only files\n");
    tsk_fprintf(stderr, "\t-l: Display long version (like ls -l)\n");
    tsk_fprintf(stderr,
        "\t-i imgtype: Format of image file (use '-i list' for supported types)\n");
    tsk_fprintf(stderr,
        "\t-b dev_sector_size: The size (in bytes) of the device sectors\n");
    tsk_fprintf(stderr,
        "\t-f fstype: File system type (use '-f list' for supported types)\n");
    tsk_fprintf(stderr,
        "\t-m: Display output in mactime input format with\n");
    tsk_fprintf(stderr,
        "\t      dir/ as the actual mount point of the image\n");
    tsk_fprintf(stderr,
        "\t-o imgoffset: Offset into image file (in sectors)\n");
    tsk_fprintf(stderr,
        "\t-I inode: Inode specification\n");
    tsk_fprintf(stderr, "\t-p: Display full path for each file\n");
    tsk_fprintf(stderr, "\t-r: Recurse on directory entries\n");
    tsk_fprintf(stderr, "\t-u: Display undeleted entries only\n");
    tsk_fprintf(stderr, "\t-v: verbose output to stderr\n");
    tsk_fprintf(stderr, "\t-V: Print version\n");
    tsk_fprintf(stderr,
        "\t-z: Time zone of original machine (i.e. EST5EDT or GMT) (only useful with -l)\n");
    tsk_fprintf(stderr,
        "\t-s seconds: Time skew of original machine (in seconds) (only useful with -l & -m)\n");

    exit(1);
}

TSK_IMG_TYPE_ENUM g_imgtype = TSK_IMG_TYPE_DETECT;
TSK_IMG_INFO *g_img = NULL;

TSK_OFF_T g_imgaddr = 0;
TSK_FS_TYPE_ENUM g_fstype = TSK_FS_TYPE_DETECT;
TSK_FS_INFO *g_fs = NULL;
FILE *g_ofile = NULL;
char *img_name = NULL;


#define NARGV 32

int
main(int argc, char **argv1)
{
    char line[1024], orig[1024];
    int nargc;
    char *nargv[NARGV], *ptr, *token, **argv;
    int i;

#ifdef TSK_WIN32
    // On Windows, get the wide arguments (mingw doesn't support wmain)
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == NULL) {
        fprintf(stderr, "Error getting wide arguments\n");
        exit(1);
    }
#else
    argv = (TSK_TCHAR **) argv1;
#endif
    setlocale(LC_ALL, "");

    img_name = strdup(argv[argc - 1]);
    process(argc, argv);

    /*
     * "This is extremely nasty, but we can't prosecute you for that."
     * "Agreed."
     */

    /* After processing the first request, we hang around reading stdin. In
     * case we get another request for the same image, we can process it in the
     * same context, using the already-open image and fs. This saves a bunch of
     * time in partition, image and FS detection and metadata loading for
     * subsequent requests, greatly improving performance.
     *
     * The sender (jlfs.js) is oblivious to the presence of this mechanism.  It
     * always emits request as a full CLI command: slt -i QEMU -I <inum> -O
     * /dev/clipboard <imagename>
     *
     * We have to parse a shell line and convert it to an argv, taking care of
     * backslash escaped characters. 
     */

    for (i = 0; i < NARGV; i++) {
        nargv[i] = (char *)calloc(256, 1);
    }

    while (fgets(line, 1024, stdin)) {
        for (i = 0; i < NARGV; i++) {
            memset(nargv[i], 0, 256);
        }
        strcpy(orig, line);
        ptr = token = line;
        nargc = 0;
        while (*ptr) {
            if (*ptr == '>' || *ptr == '\n') {
                *ptr = '\0';
                break;
            }
            if (*ptr == '\\') {
                *ptr = *(ptr + 1);
                *(ptr + 1) = '\0';
                strcat(nargv[nargc], token);
                token = ptr = ptr + 2;
            } else if (*ptr == ' ') {
                *ptr++ = '\0';
                strcat(nargv[nargc], token);
                while (*ptr == ' ') {
                    ptr++;
                }
                token = ptr;
                nargc++;
            } else {
                ptr++;
            }
        }
        if (token != ptr) {
            strcat(nargv[nargc], token);
            nargc++;
        }
        
        if (nargc == 0) {
            exit(0);
        }
        if (strcmp(nargv[nargc - 1], img_name) == 0) {
            process(nargc, nargv);
        } else {
            execl("/bin/sh", "/bin/sh", "-c", orig, NULL);
        }
    }
}

static int
process(int argc, char **argv)
{
    TSK_INUM_T inode;
    int name_flags = TSK_FS_DIR_WALK_FLAG_ALLOC | TSK_FS_DIR_WALK_FLAG_UNALLOC;
    TSK_FS_ATTR_TYPE_ENUM type = TSK_FS_ATTR_TYPE_DEFAULT;
    int ch;
    extern int OPTIND;
    int fls_flags;
    int32_t sec_skew = 0;
    static TSK_TCHAR *macpre = NULL;
    unsigned int ssize = 0;
    TSK_TCHAR *cp;
    int tsk_icat = 0, icat_reg = 0, slt_osinfo=0;
    uint16_t id = 0;
    uint8_t id_used, type_used = 0;
    int retval;
    int inode_specified = 0;

    progname = argv[0];

    fls_flags = TSK_FS_FLS_DIR | TSK_FS_FLS_FILE;

    OPTIND = 0;
    while ((ch =
            GETOPT(argc, argv, _TSK_T("ab:cdDf:Fi:I:m:lo:O:prRs:tuvVz:"))) > 0) {
        switch (ch) {
        case _TSK_T('?'):
        default:
            TFPRINTF(stderr, _TSK_T("Invalid argument: %s\n"),
                argv[OPTIND]);
            usage();
        case _TSK_T('a'):
            fls_flags |= TSK_FS_FLS_DOT;
            break;
        case _TSK_T('b'):
            ssize = (unsigned int) TSTRTOUL(OPTARG, &cp, 0);
            if (*cp || *cp == *OPTARG || ssize < 1) {
                TFPRINTF(stderr,
                    _TSK_T
                    ("invalid argument: sector size must be positive: %s\n"),
                    OPTARG);
                usage();
            }
            break;
        case _TSK_T('c'):
            tsk_icat = 1;
            break;
        case _TSK_T('d'):
            name_flags &= ~TSK_FS_DIR_WALK_FLAG_ALLOC;
            break;
        case _TSK_T('D'):
            fls_flags &= ~TSK_FS_FLS_FILE;
            fls_flags |= TSK_FS_FLS_DIR;
            break;
        case _TSK_T('f'):
            if (TSTRCMP(OPTARG, _TSK_T("list")) == 0) {
                tsk_fs_type_print(stderr);
                exit(1);
            }
            g_fstype = tsk_fs_type_toid(OPTARG);
            if (g_fstype == TSK_FS_TYPE_UNSUPP) {
                TFPRINTF(stderr,
                    _TSK_T("Unsupported file system type: %s\n"), OPTARG);
                usage();
            }
            break;
        case _TSK_T('F'):
            fls_flags &= ~TSK_FS_FLS_DIR;
            fls_flags |= TSK_FS_FLS_FILE;
            break;
        case _TSK_T('i'):
            if (TSTRCMP(OPTARG, _TSK_T("list")) == 0) {
                tsk_img_type_print(stderr);
                exit(1);
            }
            g_imgtype = tsk_img_type_toid(OPTARG);
            if (g_imgtype == TSK_IMG_TYPE_UNSUPP) {
                TFPRINTF(stderr, _TSK_T("Unsupported image type: %s\n"),
                    OPTARG);
                usage();
            }
            break;
        case _TSK_T('I'):
            if (tsk_fs_parse_inum(OPTARG, &inode, &type, &type_used, &id, &id_used)) {
                TFPRINTF(stderr, _TSK_T("Unsupported inode format: %s\n"),
                    OPTARG);
                usage();
            }
            inode_specified = 1;
            break;
        case _TSK_T('l'):
            fls_flags |= TSK_FS_FLS_LONG;
            break;
        case _TSK_T('m'):
            fls_flags |= TSK_FS_FLS_MAC;
            macpre = OPTARG;
            break;
        case _TSK_T('o'):
            if ((g_imgaddr = tsk_parse_offset(OPTARG)) == -1) {
                tsk_error_print(stderr);
                exit(1);
            }
            break;
        case _TSK_T('O'):
            if ((g_ofile = fopen(OPTARG, "w")) == NULL) {
                tsk_error_print(stderr);
                exit(1);
            }
            break;
        case _TSK_T('p'):
            fls_flags |= TSK_FS_FLS_FULL;
            break;
        case _TSK_T('r'):
            name_flags |= TSK_FS_DIR_WALK_FLAG_RECURSE;
            break;
        case _TSK_T('R'):
            icat_reg = 1;
            break;
        case _TSK_T('s'):
            sec_skew = TATOI(OPTARG);
            break;
        case _TSK_T('t'):
            slt_osinfo = 1;
            break;
        case _TSK_T('u'):
            name_flags &= ~TSK_FS_DIR_WALK_FLAG_UNALLOC;
            break;
        case _TSK_T('v'):
            tsk_verbose++;
            break;
        case _TSK_T('V'):
            tsk_version_print(stdout);
            exit(0);
        case 'z':
            {
                TSK_TCHAR envstr[32];
                TSNPRINTF(envstr, 32, _TSK_T("TZ=%s"), OPTARG);
                if (0 != PUTENV(envstr)) {
                    tsk_fprintf(stderr, "error setting environment");
                    exit(1);
                }

                /* we should be checking this somehow */
                TZSET();
            }
            break;
        }
    }

    /* We need at least one more argument */
    if (OPTIND == argc) {
        tsk_fprintf(stderr, "Missing image name\n");
        usage();
    }


    /* Set the full flag to print the full path name if recursion is
     ** set and we are only displaying files or deleted files
     */
    if ((name_flags & TSK_FS_DIR_WALK_FLAG_RECURSE)
        && (((name_flags & TSK_FS_DIR_WALK_FLAG_UNALLOC)
                && (!(name_flags & TSK_FS_DIR_WALK_FLAG_ALLOC)))
            || ((fls_flags & TSK_FS_FLS_FILE)
                && (!(fls_flags & TSK_FS_FLS_DIR))))) {

        fls_flags |= TSK_FS_FLS_FULL;
    }

    /* set flag to save full path for mactimes style printing */
    if (fls_flags & TSK_FS_FLS_MAC) {
        fls_flags |= TSK_FS_FLS_FULL;
    }

    /* we need to append a / to the end of the directory if
     * one does not already exist
     */
    if (macpre) {
        size_t len = TSTRLEN(macpre);
        if (macpre[len - 1] != '/') {
            TSK_TCHAR *tmp = macpre;
            macpre = (TSK_TCHAR *) malloc(len + 2 * sizeof(TSK_TCHAR));
            TSTRNCPY(macpre, tmp, len + 1);
            TSTRNCAT(macpre, _TSK_T("/"), len + 2);
        }
    }

    if (g_ofile == NULL) {
        g_ofile = stdout;
    }

    if (!g_img && (g_img =
            tsk_img_open(argc - OPTIND, &argv[OPTIND],
                g_imgtype, ssize)) == NULL) {
        tsk_error_print(stderr);
        exit(1);
    }
    if (!g_fs && g_imgtype == TSK_IMG_TYPE_QEMU) {
        g_imgaddr = detect_partition_offset(g_img);
    }
    if ((g_imgaddr * g_img->sector_size) >= g_img->size) {
        tsk_fprintf(stderr,
            "Sector offset supplied is larger than disk image (maximum: %"
            PRIu64 ")\n", g_img->size / g_img->sector_size);
        exit(1);
    }

    if (!g_fs && (g_fs = tsk_fs_open_img(g_img, g_imgaddr * g_img->sector_size, g_fstype)) == NULL) {
        tsk_error_print(stderr);
        if (tsk_errno == TSK_ERR_FS_UNSUPTYPE)
            tsk_fs_type_print(stderr);
        g_img->close(g_img);
        exit(1);
    }

    if (!inode_specified) {
        inode = g_fs->root_inum;
    }

    if (tsk_icat) {
        retval = tsk_fs_icat2(g_fs, inode, type, type_used, id, id_used,
        (TSK_FS_FILE_WALK_FLAG_ENUM) 0);
    } else if (icat_reg) {
        retval = tsk_fs_icat3(g_fs, (TSK_FS_FILE_WALK_FLAG_ENUM)0);
    } else if (slt_osinfo) {
        retval = tsk_get_os_info(g_fs);
    } else {
        retval = tsk_fs_fls2(g_fs, (TSK_FS_FLS_FLAG_ENUM) fls_flags, inode,
            (TSK_FS_DIR_WALK_FLAG_ENUM) name_flags, macpre, sec_skew);
        tsk_error_print(stderr);
    }
    if (retval) {
        tsk_error_print(stderr);
        exit(retval);
    }
    /*
    fs->close(fs);
    img->close(img);
    */
    if (g_ofile != stdout) {
        fclose(g_ofile);
        g_ofile = NULL;
    }
    return 0;
}

static uint8_t recurse = 1;

static int recurse_cnt = 0;
static TSK_DADDR_T recurse_list[64];
static TSK_DADDR_T selected_part_start = 0;
static TSK_DADDR_T selected_part_len = 0;
static char selected_part_desc[256];

static TSK_WALK_RET_ENUM
part_act(TSK_VS_INFO *vs, const TSK_VS_PART_INFO *part, void *ptr)
{
    if (part->flags & TSK_VS_PART_FLAG_META)
        return TSK_WALK_CONT;

    if (part->len > selected_part_len) {
        selected_part_len = part->len;
        selected_part_start = part->start;
        strncpy(selected_part_desc, part->desc, 255);
    }

    if ((recurse) && (vs->vstype == TSK_VS_TYPE_DOS)
        && (part->flags == TSK_VS_PART_FLAG_ALLOC)) {
        if (recurse_cnt < 64)
            recurse_list[recurse_cnt++] = part->start * part->vs->block_size;
    }

    return TSK_WALK_CONT;
}
static TSK_DADDR_T
detect_partition_offset(TSK_IMG_INFO *img)
{
    TSK_VS_INFO *vs;
    int flags = TSK_VS_PART_FLAG_ALLOC;
    flags &= ~TSK_VS_PART_FLAG_META;
    TSK_VS_TYPE_ENUM vstype = TSK_VS_TYPE_DETECT;
    vs = tsk_vs_open(img, 0, vstype);
    if (vs == NULL) {
        tsk_error_print(stderr);
        if (tsk_errno == TSK_ERR_VS_UNSUPTYPE) {
            tsk_vs_type_print(stderr);
        }
        return 0;
    }
    if (tsk_vs_part_walk(vs, 0, vs->part_count - 1,
            (TSK_VS_PART_FLAG_ENUM) flags, part_act, NULL)) {
        tsk_error_print(stderr);
        tsk_vs_close(vs);
        return 0;
    }
    tsk_vs_close(vs);
    if ((recurse) && (vs->vstype == TSK_VS_TYPE_DOS)) {
        int i;
        /* disable recursing incase we hit another DOS partition
         * future versions may support more layers */
        recurse = 0;

        for (i = 0; i < recurse_cnt; i++) {
            vs = tsk_vs_open(img, recurse_list[i], TSK_VS_TYPE_DETECT);
            if (vs != NULL) {
                if (tsk_vs_part_walk(vs, 0, vs->part_count - 1,
                        (TSK_VS_PART_FLAG_ENUM) flags, part_act, NULL)) {
                    tsk_error_reset();
                }
                tsk_vs_close(vs);
            }
            else {
                /* Ignore error in this case and reset */
                tsk_error_reset();
            }
        }
    }
    if (selected_part_len) {
        fprintf(stderr, "Selected partition at offset %.10" PRIuDADDR " size %.10" PRIuDADDR " desc %s\n", selected_part_start, selected_part_len, selected_part_desc);
    }
    return selected_part_start;
}

/* Call back action for file_walk
 */
static TSK_WALK_RET_ENUM
icat_action(TSK_FS_FILE * fs_file, TSK_OFF_T a_off, TSK_DADDR_T addr,
    char *buf, size_t size, TSK_FS_BLOCK_FLAG_ENUM flags, void *ptr)
{
    if (size == 0)
        return TSK_WALK_CONT;

    if (!g_ofile) {
        return TSK_WALK_ERROR;
    }

    if (fwrite(buf, size, 1, g_ofile) != 1) {
        tsk_error_reset();
        tsk_errno = TSK_ERR_FS_WRITE;
        snprintf(tsk_errstr, TSK_ERRSTR_L,
            "icat_action: error writing to stdout: %s", strerror(errno));
        return TSK_WALK_ERROR;
    }

    return TSK_WALK_CONT;
}

/* Return 1 on error and 0 on success */
static uint8_t
tsk_fs_icat2(TSK_FS_INFO * fs, TSK_INUM_T inum,
    TSK_FS_ATTR_TYPE_ENUM type, uint8_t type_used,
    uint16_t id, uint8_t id_used, TSK_FS_FILE_WALK_FLAG_ENUM flags)
{
    TSK_FS_FILE *fs_file;

#ifdef TSK_WIN32
    if (-1 == _setmode(_fileno(stdout), _O_BINARY)) {
        tsk_error_reset();
        tsk_errno = TSK_ERR_FS_WRITE;
        snprintf(tsk_errstr, TSK_ERRSTR_L,
            "icat_lib: error setting stdout to binary: %s",
            strerror(errno));
        return 1;
    }
#endif

    fs_file = tsk_fs_file_open_meta(fs, NULL, inum);
    if (!fs_file) {
        return 1;
    }

    if (type_used) {
        if (id_used == 0) {
            flags = (TSK_FS_FILE_WALK_FLAG_ENUM)(flags | TSK_FS_FILE_WALK_FLAG_NOID);
        }
        if (tsk_fs_file_walk_type(fs_file, type, id, flags, icat_action,
                NULL)) {
            tsk_fs_file_close(fs_file);
            return 1;
        }
    }
    else {
        if (tsk_fs_file_walk(fs_file, flags, icat_action, NULL)) {
            tsk_fs_file_close(fs_file);
            return 1;
        }
    }


    tsk_fs_file_close(fs_file);

    return 0;
}

/* Return 1 on error and 0 on success */
static uint8_t
tsk_fs_icat3(TSK_FS_INFO * fs, TSK_FS_FILE_WALK_FLAG_ENUM flags)
{
    TSK_FS_FILE *fs_file;

#ifdef TSK_WIN32
    if (-1 == _setmode(_fileno(stdout), _O_BINARY)) {
        tsk_error_reset();
        tsk_errno = TSK_ERR_FS_WRITE;
        snprintf(tsk_errstr, TSK_ERRSTR_L,
            "icat_lib: error setting stdout to binary: %s",
            strerror(errno));
        return 1;
    }
#endif

    fs_file = tsk_fs_file_open(fs, NULL, "/Windows/system32/config/software");
    if (!fs_file) {
        return 1;
    }

    fprintf(g_ofile, "opening file\n");
    if (tsk_fs_file_walk(fs_file, flags, icat_action, NULL)) {
        tsk_fs_file_close(fs_file);
        return 1;
    }
    tsk_fs_file_close(fs_file);

    return 0;
}

/** \internal 
* Structure to store data for callbacks.
*/
typedef struct {
    /* Time skew of the system in seconds */
    int32_t sec_skew;

    /*directory prefix for printing mactime output */
    char *macpre;
    int flags;
} FLS_DATA;




/* this is a wrapper type function that takes care of the runtime
 * flags
 * 
 * fs_attr should be set to NULL for all non-NTFS file systems
 */
static void
printit(TSK_FS_FILE * fs_file, const char *a_path,
    const TSK_FS_ATTR * fs_attr, const FLS_DATA * fls_data)
{
    unsigned int i;

    if ((!(fls_data->flags & TSK_FS_FLS_FULL)) && (a_path)) {
        uint8_t printed = 0;
        // lazy way to find out how many dirs there could be
        for (i = 0; a_path[i] != '\0'; i++) {
            if ((a_path[i] == '/') && (i != 0)) {
                tsk_fprintf(g_ofile, "+");
                printed = 1;
            }
        }
        if (printed)
            tsk_fprintf(g_ofile, " ");
    }


    if (fls_data->flags & TSK_FS_FLS_MAC) {
        tsk_fs_name_print_mac(g_ofile, fs_file, a_path,
            fs_attr, fls_data->macpre, fls_data->sec_skew);
    }
    else if (fls_data->flags & TSK_FS_FLS_LONG) {
        tsk_fs_name_print_long(g_ofile, fs_file, a_path, fs_file->fs_info,
            fs_attr, TSK_FS_FLS_FULL & fls_data->flags ? 1 : 0,
            fls_data->sec_skew);
    }
    else {
        tsk_fs_name_print(g_ofile, fs_file, a_path, fs_file->fs_info,
            fs_attr, TSK_FS_FLS_FULL & fls_data->flags ? 1 : 0);
        tsk_fprintf(g_ofile, "\n");
    }
}


/* 
 * call back action function for dent_walk
 */
static TSK_WALK_RET_ENUM
print_dent_act(TSK_FS_FILE * fs_file, const char *a_path, void *ptr)
{
    FLS_DATA *fls_data = (FLS_DATA *) ptr;

    /* only print dirs if TSK_FS_FLS_DIR is set and only print everything
     ** else if TSK_FS_FLS_FILE is set (or we aren't sure what it is)
     */
    if (((fls_data->flags & TSK_FS_FLS_DIR) &&
            ((fs_file->meta) &&
                (fs_file->meta->type == TSK_FS_META_TYPE_DIR)))
        || ((fls_data->flags & TSK_FS_FLS_FILE) && (((fs_file->meta)
                    && (fs_file->meta->type != TSK_FS_META_TYPE_DIR))
                || (!fs_file->meta)))) {


        /* Make a special case for NTFS so we can identify all of the
         * alternate data streams!
         */
        if ((TSK_FS_TYPE_ISNTFS(fs_file->fs_info->ftype))
            && (fs_file->meta)) {
            uint8_t printed = 0;
            int i, cnt;

            // cycle through the attributes
            cnt = tsk_fs_file_attr_getsize(fs_file);
            for (i = 0; i < cnt; i++) {
                const TSK_FS_ATTR *fs_attr =
                    tsk_fs_file_attr_get_idx(fs_file, i);
                if (!fs_attr)
                    continue;

                if (fs_attr->type == TSK_FS_ATTR_TYPE_NTFS_DATA) {
                    printed = 1;

                    if (fs_file->meta->type == TSK_FS_META_TYPE_DIR) {

                        /* we don't want to print the ..:blah stream if
                         * the -a flag was not given
                         */
                        if ((fs_file->name->name[0] == '.')
                            && (fs_file->name->name[1])
                            && (fs_file->name->name[2] == '\0')
                            && ((fls_data->flags & TSK_FS_FLS_DOT) == 0)) {
                            continue;
                        }
                    }

                    printit(fs_file, a_path, fs_attr, fls_data);
                }
                else if (fs_attr->type == TSK_FS_ATTR_TYPE_NTFS_IDXROOT) {
                    printed = 1;

                    /* If it is . or .. only print it if the flags say so,
                     * we continue with other streams though in case the 
                     * directory has a data stream 
                     */
                    if (!((TSK_FS_ISDOT(fs_file->name->name)) &&
                            ((fls_data->flags & TSK_FS_FLS_DOT) == 0)))
                        printit(fs_file, a_path, fs_attr, fls_data);
                }
            }

            /* A user reported that an allocated file had the standard
             * attributes, but no $Data.  We should print something */
            if (printed == 0) {
                printit(fs_file, a_path, NULL, fls_data);
            }

        }
        else {
            /* skip it if it is . or .. and we don't want them */
            if (!((TSK_FS_ISDOT(fs_file->name->name))
                    && ((fls_data->flags & TSK_FS_FLS_DOT) == 0)))
                printit(fs_file, a_path, NULL, fls_data);
        }
    }
    return TSK_WALK_CONT;
}


/* Returns 0 on success and 1 on error */
static uint8_t
tsk_fs_fls2(TSK_FS_INFO * fs, TSK_FS_FLS_FLAG_ENUM lclflags,
    TSK_INUM_T inode, TSK_FS_DIR_WALK_FLAG_ENUM flags, TSK_TCHAR * tpre,
    int32_t skew)
{
    FLS_DATA data;

    data.flags = lclflags;
    data.sec_skew = skew;

#ifdef TSK_WIN32
    {
        char *cpre;
        size_t clen;
        UTF8 *ptr8;
        UTF16 *ptr16;
        int retval;

        if (tpre != NULL) {
            clen = TSTRLEN(tpre) * 4;
            cpre = (char *) tsk_malloc(clen);
            if (cpre == NULL) {
                return 1;
            }
            ptr8 = (UTF8 *) cpre;
            ptr16 = (UTF16 *) tpre;

            retval =
                tsk_UTF16toUTF8_lclorder((const UTF16 **) &ptr16, (UTF16 *)
                & ptr16[TSTRLEN(tpre) + 1], &ptr8,
                (UTF8 *) ((uintptr_t) ptr8 + clen), TSKlenientConversion);
            if (retval != TSKconversionOK) {
                tsk_error_reset();
                tsk_errno = TSK_ERR_FS_UNICODE;
                snprintf(tsk_errstr, TSK_ERRSTR_L,
                    "Error converting fls mactime pre-text to UTF-8 %d\n",
                    retval);
                return 1;
            }
            data.macpre = cpre;
        }
        else {
            data.macpre = NULL;
            cpre = NULL;
        }

        retval = tsk_fs_dir_walk(fs, inode, flags, print_dent_act, &data);

        if (cpre)
            free(cpre);

        return retval;
    }
#else
    data.macpre = tpre;
    return tsk_fs_dir_walk(fs, inode, flags, print_dent_act, &data);
#endif
}

struct file_handle {
    TSK_FS_INFO * fs;
    void *handle;
    uint32_t coff;
};

#include <dlfcn.h>

#define OS_INFO_LIB_NAME    "libosinfo.so"
#define OS_INFO_FUN_NAME    "osi_get_os_details"
struct file_handle regfile;
const char *regfilename = "/Windows/system32/config/software";
typedef int (* osi_get_os_details_t)(void *open, void *read, void *lseek, char ***info);
static void *oslib = NULL;
static osi_get_os_details_t osi_get_os_details = NULL;
static int dumpfd = 0;
static int readcount = 0;
#define BUF_33MB    (33*1024*1024)
static int readoffcount[1] = { 0 };
static int readsizecount[1] = { 0 };
int clbk_open(char *fname, int mode)
{
    void *fs_file;
    fprintf(g_ofile, "Opening registry file %s\n", regfilename);
    fs_file = tsk_fs_file_open(regfile.fs, NULL, regfilename);
    if (!fs_file) {
        fprintf(g_ofile, "Failed\n");
        return 1;
    }

    //dumpfd = open("/tmp/dump2", O_CREAT|O_WRONLY|O_TRUNC, 0777);
    regfile.handle = fs_file;
    regfile.coff = 0;

    return 10;
}

int clbk_read(int fd, char *buf, size_t size, size_t off)
{
    int ret = 0;
    regfile.coff = off;
    ret = tsk_fs_file_read((TSK_FS_FILE *)regfile.handle, regfile.coff, buf, size, 
                            (TSK_FS_FILE_READ_FLAG_ENUM)0);

    if(regfile.coff < (sizeof(readoffcount)/sizeof(readoffcount[0]))) {
        readoffcount[regfile.coff] += 1;
        readsizecount[regfile.coff] += ret;
    }
        
    readcount += ret;
    if(dumpfd) {
        int x =0;
        x =write(dumpfd, buf, ret);
    }
    //fprintf(g_ofile, "Read %d asked %ld \n", ret, size);
    return ret;
}
size_t clbk_get_size(int fd)
{
    int ret = 0;
    TSK_FS_FILE *fl = (TSK_FS_FILE *)regfile.handle;
    fprintf(g_ofile, "File size : %d\n", fl->meta->size);

    return fl->meta->size;
}


int clbk_seek(int fd, off_t off, int wh)
{
    int ret = 0;

    if(wh == SEEK_SET)
        regfile.coff = off;
    else if (wh == SEEK_CUR)
        regfile.coff += off;

    //fprintf(g_ofile, "Seek off %d wh %d \n", (int) off, wh);
    return ret;
}

static uint8_t tsk_get_os_info(TSK_FS_INFO * fs)
{
    TSK_FS_FILE *fs_file;
    char **info = NULL;
    int i=0;
    char *error = NULL;

#ifdef TSK_WIN32
    if (-1 == _setmode(_fileno(stdout), _O_BINARY)) {
        tsk_error_reset();
        tsk_errno = TSK_ERR_FS_WRITE;
        snprintf(tsk_errstr, TSK_ERRSTR_L,
            "icat_lib: error setting stdout to binary: %s",
            strerror(errno));
        return 1;
    }
#endif
    memset(&regfile, 0, sizeof(regfile));

    oslib = dlopen(OS_INFO_LIB_NAME, RTLD_LAZY);
    if (!oslib) {
        fprintf(stderr, "Failed to load library %s due to %s",
                OS_INFO_LIB_NAME, dlerror());
        return 1;
    }

    osi_get_os_details = (osi_get_os_details_t)dlsym(oslib, OS_INFO_FUN_NAME);
    if ((error = dlerror()) != NULL) {
        fprintf(g_ofile, "Failed to load symbol %s from library error %s \n",
                OS_INFO_FUN_NAME, error);
        return 1;
    }

    regfile.fs = fs;
    fprintf(g_ofile, "Reading os info from registry\n");
    
    i = osi_get_os_details((void *)clbk_open,(void *) clbk_read, (void *)clbk_get_size, &info);

    printf("Total read %d \n", readcount);

    if(dumpfd) close(dumpfd);
    i=0;
    if(!info || info[0]==NULL) printf("No info found \n");
    while(info && info[i])
    {
        fprintf(g_ofile, "\t%s: %s\n", info[i], info[i+1]);
        free(info[i]);
        free(info[i+1]);
        i+=2;
    }

    for (i=0;i<(sizeof(readoffcount)/sizeof(readoffcount[0]));i++)
    {
        if(readoffcount[i])
            printf("count %d off %d size %d \n", readoffcount[i], i, readsizecount[i]);
    }
    free(info);

    
/*
    if (tsk_fs_file_walk(fs_file, flags, icat_action, NULL)) {
        tsk_fs_file_close(fs_file);
        return 1;
    }
*/
    tsk_fs_file_close(fs_file);

    return 0;
}


