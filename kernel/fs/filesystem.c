#include <fs/fs.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <sync/atomic.h>

static spinlock_t *fsIDlock = &SPINLOCK_INIT();

static atomic_t fsIDalloc(void) {
    static atomic_t fs_ID = 0;
    spin_lock(fsIDlock);
    atomic_t id = __atomic_add_fetch(&fs_ID, 1, __ATOMIC_SEQ_CST);
    spin_unlock(fsIDlock);
    return id;
}

int fsalloc(filesystem_t **pfs) {
    int err = 0;
    queue_t *queue = NULL;
    filesystem_t *fs = NULL;

    if (pfs == NULL)
        return -EINVAL;

    if ((err = queue_alloc(&queue)))
        return err;

    if ((fs = kmalloc(sizeof *fs)) == NULL) {
        err = -ENOMEM;
        goto error;
    }

    memset(fs, 0, sizeof *fs);

    fs->fs_count = 1;
    fs->fs_id = fsIDalloc();
    fs->fs_superblocks = queue;
    fs->fs_lock = SPINLOCK_INIT();
    fslock(fs);
    *pfs = fs;
    return 0;
error:
    if (queue)
        queue_free(queue);
    return err;
}

void fs_free(filesystem_t *fs) {
    if (!fsislocked(fs))
        fslock(fs);

    fs_put(fs);

    if (fs->fs_count <= 0) {
        if (fs->fs_name)
            fs_unsetname(fs);
        if (fs->fs_superblocks)
            queue_free(fs->fs_superblocks);
        fsunlock(fs);
        kfree(fs);
        return;
    }

    fsunlock(fs);
}

int fs_create(const char *name, iops_t *iops, filesystem_t **pfs) {
    int err = 0;
    filesystem_t *fs = NULL;
    if (name == NULL || iops == NULL || pfs == NULL)
        return -EINVAL;
    
    if ((err = fsalloc(&fs)))
        return err;
    
    if ((err = fs_setname(fs, name)))
        goto error;

    fs->fs_iops = iops;

    *pfs = fs;

    return 0;
error:
    if (fs)
        fs_free(fs);
    return err;
}


void fs_dup(filesystem_t *fs) {
    fsassert_locked(fs);
    fs->fs_count++;
}

void fs_put(filesystem_t *fs) {
    fsassert_locked(fs);
    fs->fs_count--;
}

long fs_count(filesystem_t *fs) {
    fsassert_locked(fs);
    return fs->fs_count;
}

int fs_setname(filesystem_t *fs, const char *fsname) {
    char *name = NULL;

    fsassert_locked(fs);

    if (fs == NULL || fsname == NULL)
        return -EINVAL;

    if ((name = strdup(fsname)) == NULL)
        return -ENOMEM;
    
    fs->fs_name = name;

    return 0;
}

void fs_unsetname(filesystem_t *fs) {
    fsassert_locked(fs);
    if (fs == NULL)
        return;
    if (fs->fs_name)
        kfree(fs->fs_name);
}

int fs_set_iops(filesystem_t *fs, iops_t *iops) {
    fsassert_locked(fs);
    if (fs == NULL || iops == NULL)
        return -EINVAL;
    fs->fs_iops = iops;
    return 0;
}

int fs_add_superblock(filesystem_t *fs, superblock_t *sb) {
    int err = 0;

    fsassert_locked(fs);
    if (fs == NULL || sb == NULL)
        return -EINVAL;
    
    queue_lock(fs->fs_superblocks);
    if ((err =  enqueue(fs->fs_superblocks, sb, 1, NULL))) {
        queue_unlock(fs->fs_superblocks);
        return -ENOMEM;
    }
    queue_unlock(fs->fs_superblocks);

    sb->sb_iops = fs->fs_iops;

    return 0;
}

int fs_del_superblock(filesystem_t *fs, superblock_t *sb)
{
    int err = 0;
    fsassert_locked(fs);
    if (fs == NULL || sb == NULL)
        return -EINVAL;
    queue_lock(fs->fs_superblocks);
    err = queue_remove(fs->fs_superblocks, sb);
    queue_unlock(fs->fs_superblocks);
    return err;
}