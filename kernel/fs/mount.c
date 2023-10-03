#include <fs/fs.h>
#include <mm/kalloc.h>
#include <bits/errno.h>
#include <sys/_fcntl.h>

static queue_t *mnt_queue = QUEUE_NEW();

fs_mount_t *alloc_fsmount(void) {
    fs_mount_t *mnt = NULL;
    if ((mnt = kmalloc(sizeof *mnt)) == NULL)
        return NULL;
    memset(mnt, 0, sizeof *mnt);
    mnt->mnt_lock = SPINLOCK_INIT();
    mnt_lock(mnt);
    return mnt;
}

__unused static int mnt_insert(fs_mount_t *mnt, dentry_t *target) {
    int err = 0;
    int locked_root= 0, parent_lk = 0;
    stack_t *stack = NULL;
    dentry_t *parent = NULL;

    if (mnt == NULL || target == NULL)
        return -EINVAL;
    
    mnt_assert_locked(mnt);
    dassert_locked(target);

    if (mnt->mnt_root == NULL)
        return -EINVAL;
    
    if ((locked_root = !dislocked(mnt->mnt_root)))
        dlock(mnt->mnt_root);
    
    if (mnt->mnt_root->d_mnt_stack == NULL) {
        if ((err = stack_alloc(&stack))) {
            if (locked_root)
                dunlock(mnt->mnt_root);
            return err;
        }

    }

    if ((err = stack_push(stack, (void *)target))) {
        if (locked_root)
            dunlock(mnt->mnt_root);
        goto error;
    }

    if ((parent = target->d_parent)) {
        if ((parent_lk = dislocked(parent)))
            dlock(parent);
        ddup(parent);
        dunbind(target);
        if ((err = dbind(parent, mnt->mnt_root))) {
            if (parent_lk) {
                dput(parent);
                dunlock(parent);
            }

            if (locked_root)
                dunlock(mnt->mnt_root);
            goto error;
        }
        
        if (parent_lk) {
            dput(parent);
            dunlock(parent);
        }
    } else if ((err = vfs_mount_droot(mnt->mnt_root))) {
            if (locked_root)
                dunlock(mnt->mnt_root);
            goto error;
    }

    queue_lock(mnt_queue);
    if ((err = queue_contains(mnt_queue, (void *)mnt, NULL))) {
        queue_unlock(mnt_queue);
        if (locked_root)
            dunlock(mnt->mnt_root);
        goto error;
        return err;
    }

    if (enqueue(mnt_queue, (void *)mnt) == NULL) {
        queue_unlock(mnt_queue);
        if (locked_root)
            dunlock(mnt->mnt_root);
        err -ENOMEM;
        goto error;
    }
    queue_unlock(mnt_queue);

    mnt->mnt_root->d_mnt_stack = stack;
    return 0;
error:
    if (stack)
        stack_free(stack);
    return err;
}


__unused static int mnt_remove(fs_mount_t *mnt) {
    int err = 0;

    if (mnt == NULL)
        return -EINVAL;
    
    mnt_assert_locked(mnt);

    queue_lock(mnt_queue);

    if ((err = queue_remove(mnt_queue, (void *)mnt))) {
        queue_unlock(mnt_queue);
        return err;
    }

    queue_unlock(mnt_queue);
    return 0;
}

static int do_new_mount(filesystem_t *fs, const char *src, unsigned long flags, void *data, fs_mount_t **pmnt) {
    int err = 0;
    fs_mount_t *mnt = NULL;
    superblock_t *sb = NULL;

    fsassert_locked(fs);

    if (pmnt == NULL || fs == NULL)
        return -EINVAL;

    if ((mnt = alloc_fsmount()) == NULL) {
        err = -ENOMEM;
        goto error;
    }

    if (fs->get_sb == NULL) {
        err = -EINVAL;
        goto error;
    }

    if ((err = fs->get_sb(fs, src, flags, data, &sb)))
        goto error;

    mnt->mnt_sb = sb;
    sb->sb_mnt = mnt;
    mnt->mnt_root = sb->sb_root;
    sbunlock(sb);
    *pmnt = mnt;

    return 0;
error:
    if (mnt) kfree(mnt);
    return err;
}

int vfs_mount(const char *src,
              const char *target,
              const char *type,
              unsigned long flags,
              const void *data) {
    int err = 0;
    fs_mount_t *mnt = NULL;
    filesystem_t *fs = NULL;
    __unused inode_t *isrc = NULL, *itarget = NULL;
    __unused dentry_t *dentry = NULL, *dsrc = NULL, *dtarget = NULL;

    if (target == NULL)
        return -EINVAL;

    if ((err = vfs_getfs(type, &fs)))
        return err;

    if (flags & MS_REMOUNT) {
    } else if (flags & MS_BIND) {
    } else if (flags & MS_MOVE) {
    } else {
        // Do New Mount.
        if ((err = do_new_mount(fs, src, flags, (void *)data, &mnt))) {
            fsunlock(fs);
            return err;
        }


        goto bind;
    }
    fsunlock(fs);
    return 0;
bind:
    if ((err = vfs_lookup(target, NULL, O_RDONLY, 0, 0, NULL, &dtarget)))
        goto error;
    
    if ((err = mnt_insert(mnt, dtarget))) {
        dclose(dtarget);
        fsunlock(fs);
        goto error;
    }

    dclose(dtarget);
    fsunlock(fs);

    return 0;

error:
    panic("%s: error: %d\n", __func__, err);
    return err;
}