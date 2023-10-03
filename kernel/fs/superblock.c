#include <fs/fs.h>
#include <bits/errno.h>
#include <dev/dev.h>

static atomic_t sbID = 0;

#define NODEV_TMPFS 0
#define NODEV_DEVFS 1

static const char *minors[] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69",
    "70", "71", "72", "73", "74", "75", "76", "77", "78", "79",
    "80", "81", "82", "83", "84", "85", "86", "87", "88", "89",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99",
    "100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
    "110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
    "120", "121", "122", "123", "124", "125", "126", "127", "128", "129",
    "130", "131", "132", "133", "134", "135", "136", "137", "138", "139",
    "140", "141", "142", "143", "144", "145", "146", "147", "148", "149",
    "150", "151", "152", "153", "154", "155", "156", "157", "158", "159",
    "160", "161", "162", "163", "164", "165", "166", "167", "168", "169",
    "170", "171", "172", "173", "174", "175", "176", "177", "178", "179",
    "180", "181", "182", "183", "184", "185", "186", "187", "188", "189",
    "190", "191", "192", "193", "194", "195", "196", "197", "198", "199",
    "200", "201", "202", "203", "204", "205", "206", "207", "208", "209",
    "210", "211", "212", "213", "214", "215", "216", "217", "218", "219",
    "220", "221", "222", "223", "224", "225", "226", "227", "228", "229",
    "230", "231", "232", "233", "234", "235", "236", "237", "238", "239",
    "240", "241", "242", "243", "244", "245", "246", "247", "248", "249",
    "250", "251", "252", "253", "254", "255", NULL,
};

int getsb(filesystem_t * fs, struct devid *devid, superblock_t **psb)
{
    int err = 0;
    superblock_t *sb = NULL;
    queue_node_t *next = NULL;

    fsassert_locked(fs);

    if (psb == NULL || devid == NULL)
        return -EINVAL;

    queue_lock(fs->fs_superblocks);
    forlinked(node, fs->fs_superblocks->head, next) {
        next = node->next;
        sb = node->data;
        sblock(sb);
        if (DEVID_CMP(devid, &sb->sb_devid)) {
            *psb = sb;
            queue_unlock(fs->fs_superblocks);
            return 0;
        }
        sbunlock(sb);
    }
    queue_unlock(fs->fs_superblocks);

    if ((sb = kmalloc(sizeof *sb)) == NULL)
        return -ENOMEM;

    memset(sb, 0, sizeof *sb);
    sb->sb_lock = SPINLOCK_INIT();

    *sb = (superblock_t) {
        .sb_count = 1,
        .sb_blocksize = 0,
        .sb_filesystem = fs,
        .sb_flags = 0,
        .sb_devid = *devid,
        .sb_id = atomic_inc_fetch(&sbID),
        .sb_priv = NULL,
    };

    sblock(sb);

    if ((err = fs_add_superblock(fs, sb))) {
        sbunlock(sb);
        kfree(sb);
        return err;
    }
    
    *psb = sb;
    return 0;
}

int getsb_bdev(filesystem_t *fs, const char *bdev_name, const char *target,
                unsigned long flags, void *data __unused, superblock_t **psbp,
                int (*sb_fill)(filesystem_t *fs, const char *target, struct devid *dd, superblock_t *sb)) {
    int err = 0;
    struct devid devid;
    bdev_info_t bdevinfo;
    superblock_t *sb = NULL;

    fsassert_locked(fs);

    if (psbp == NULL)
        return -EINVAL;

    if (sb_fill == NULL)
        return -ENOSYS;

    if ((err = kdev_open_bdev(bdev_name, &devid)))
        return err;
    
    if ((err = getsb(fs, &devid, &sb))) {
        if (err == -EINVAL)
            return err;
    }

    kdev_getinfo(&devid, &bdevinfo);

    sb->sb_flags |= flags;
    sb->sb_size = bdevinfo.bi_size;
    sb->sb_blocksize = bdevinfo.bi_blocksize;

    if ((err = sb_fill(fs, target, &devid, sb)))
        return err;

    *psbp = sb;
    return 0;
}

int getsb_nodev(filesystem_t *fs, const char *target,
                unsigned long flags __unused, void *data __unused,
                superblock_t **psbp, int (*sb_fill)(filesystem_t *fs,
                const char *target, struct devid *dd, superblock_t *sb)) {
    int err = 0;
    dev_t *dev = NULL;
    char *name = NULL;
    superblock_t *sb = NULL;
    static uint16_t devno = 1;

    if ((sb_fill == NULL))
        return -ENOSYS;
    
    if (psbp == NULL)
        return -EINVAL;

    uint16_t minor = atomic_fetch_inc(&devno);

    if (minor >= 256)
        return -ENOMEM;

    if ((name = combine_strings("virtdev", minors[minor])) == NULL)
        return -ENOMEM;

    if ((err = kdev_create(name, FS_BLK, 0, minor, &dev)))
        return err;

    if ((err = getsb(fs, &dev->devid, &sb))) {
        dev_unlock(dev);
        return err;
    }
    
    if ((err = sb_fill(fs, target, &dev->devid, sb))) {
        dev_unlock(dev);
        return err;
    }

    dev_unlock(dev);
    *psbp = sb;
    return 0;
}

int getsb_pseudo(filesystem_t *fs, const char *target,
    unsigned long flags, void *data, superblock_t **psbp,
    int (*sb_fill)(filesystem_t *fs, const char *target,
    struct devid *dd, superblock_t *sb));
int getsb_single(filesystem_t *fs, const char *target,
    unsigned long flags, void *data, superblock_t **psbp,
    int (*sb_fill)(filesystem_t *fs, const char *target,
    struct devid *dd, superblock_t *sb));