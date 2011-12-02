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
#include <locale.h>
#include <time.h>

static TSK_TCHAR *progname;
static TSK_DADDR_T detect_partition_offset(TSK_IMG_INFO *img);

void
usage()
{
    TFPRINTF(stderr,
        _TSK_T
        ("usage: %s [-adDFlpruvV] [-f fstype] [-i imgtype] [-b dev_sector_size] [-m dir/] [-o imgoffset] [-z ZONE] [-s seconds] image [images] [inode]\n"),
        progname);
    tsk_fprintf(stderr,
        "\tIf [inode] is not given, the root directory is used\n");
    tsk_fprintf(stderr, "\t-a: Display \".\" and \"..\" entries\n");
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

int
main(int argc, char **argv1)
{
    TSK_IMG_TYPE_ENUM imgtype = TSK_IMG_TYPE_DETECT;
    TSK_IMG_INFO *img;

    TSK_OFF_T imgaddr = 0;
    TSK_FS_TYPE_ENUM fstype = TSK_FS_TYPE_DETECT;
    TSK_FS_INFO *fs;

    TSK_INUM_T inode;
    int name_flags = TSK_FS_DIR_WALK_FLAG_ALLOC | TSK_FS_DIR_WALK_FLAG_UNALLOC;
    TSK_FS_ATTR_TYPE_ENUM type = TSK_FS_ATTR_TYPE_DEFAULT;
    int ch;
    extern int OPTIND;
    int fls_flags;
    int32_t sec_skew = 0;
    static TSK_TCHAR *macpre = NULL;
    TSK_TCHAR **argv;
    unsigned int ssize = 0;
    TSK_TCHAR *cp;
    int tsk_icat = 0;
    uint16_t id = 0;
    uint8_t id_used, type_used = 0;
    int retval;

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

    progname = argv[0];
    setlocale(LC_ALL, "");

    fls_flags = TSK_FS_FLS_DIR | TSK_FS_FLS_FILE;

    while ((ch =
            GETOPT(argc, argv, _TSK_T("ab:cdDf:Fi:m:lo:prs:uvVz:"))) > 0) {
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
            fstype = tsk_fs_type_toid(OPTARG);
            if (fstype == TSK_FS_TYPE_UNSUPP) {
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
            imgtype = tsk_img_type_toid(OPTARG);
            if (imgtype == TSK_IMG_TYPE_UNSUPP) {
                TFPRINTF(stderr, _TSK_T("Unsupported image type: %s\n"),
                    OPTARG);
                usage();
            }
            break;
        case _TSK_T('l'):
            fls_flags |= TSK_FS_FLS_LONG;
            break;
        case _TSK_T('m'):
            fls_flags |= TSK_FS_FLS_MAC;
            macpre = OPTARG;
            break;
        case _TSK_T('o'):
            if ((imgaddr = tsk_parse_offset(OPTARG)) == -1) {
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
        case _TSK_T('s'):
            sec_skew = TATOI(OPTARG);
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
        case _TSK_T('c'):
            tsk_icat = 1;
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

    /* open image - there is an optional inode address at the end of args 
     *
     * Check the final argument and see if it is a number
     */
    if (tsk_fs_parse_inum(argv[argc - 1], &inode, &type, &type_used, &id, &id_used)) {
        /* Not an inode at the end */
        if ((img =
                tsk_img_open(argc - OPTIND, &argv[OPTIND],
                    imgtype, ssize)) == NULL) {
            tsk_error_print(stderr);
            exit(1);
        }
        if (imgtype == TSK_IMG_TYPE_QEMU) {
            imgaddr = detect_partition_offset(img);
        }
        if ((imgaddr * img->sector_size) >= img->size) {
            tsk_fprintf(stderr,
                "Sector offset supplied is larger than disk image (maximum: %"
                PRIu64 ")\n", img->size / img->sector_size);
            exit(1);
        }
        if ((fs = tsk_fs_open_img(img, imgaddr * img->sector_size, fstype)) == NULL) {
            tsk_error_print(stderr);
            if (tsk_errno == TSK_ERR_FS_UNSUPTYPE)
                tsk_fs_type_print(stderr);

            img->close(img);
            exit(1);
        }
        inode = fs->root_inum;
    }
    else {
        // check that we have enough arguments
        if (OPTIND + 1 == argc) {
            tsk_fprintf(stderr, "Missing image name or inode\n");
            usage();
        }

        if ((img =
                tsk_img_open(argc - OPTIND - 1, &argv[OPTIND],
                    imgtype, ssize)) == NULL) {
            tsk_error_print(stderr);
            exit(1);
        }
        if (imgtype == TSK_IMG_TYPE_QEMU) {
            imgaddr = detect_partition_offset(img);
        }
        if ((imgaddr * img->sector_size) >= img->size) {
            tsk_fprintf(stderr,
                "Sector offset supplied is larger than disk image (maximum: %"
                PRIu64 ")\n", img->size / img->sector_size);
            exit(1);
        }


        if ((fs = tsk_fs_open_img(img, imgaddr * img->sector_size, fstype)) == NULL) {
            tsk_error_print(stderr);
            if (tsk_errno == TSK_ERR_FS_UNSUPTYPE)
                tsk_fs_type_print(stderr);
            img->close(img);
            exit(1);
        }
    }

    if (tsk_icat) {
        retval = tsk_fs_icat(fs, inode, type, type_used, id, id_used,
        (TSK_FS_FILE_WALK_FLAG_ENUM) 0);
    } else {
        retval = tsk_fs_fls(fs, (TSK_FS_FLS_FLAG_ENUM) fls_flags, inode,
            (TSK_FS_DIR_WALK_FLAG_ENUM) name_flags, macpre, sec_skew);
        tsk_error_print(stderr);
    }
    if (retval) {
        tsk_error_print(stderr);
    }
    fs->close(fs);
    img->close(img);

    exit(retval);
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
