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

void fsmount_free(fs_mount_t *mnt) {
    if (mnt == NULL)
        return;
    
    if (!mnt_islocked(mnt))
        mnt_lock(mnt);

    if (mnt->mnt_root) {
        dlock(mnt->mnt_root);
        dclose(mnt->mnt_root);
    }

    mnt_unlock(mnt);
    kfree(mnt);
}

static int mnt_insert(fs_mount_t *mnt, dentry_t *target) {
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

    stack_lock(stack);
    if ((err = stack_push(stack, (void *)target))) {
        stack_unlock(stack);
        if (locked_root)
            dunlock(mnt->mnt_root);
        goto error;
    }
    stack_unlock(stack);

    if ((parent = target->d_parent)) {
        if ((parent_lk = !dislocked(parent)))
            dlock(parent);
        ddup(parent);
        if (parent_lk)
            dunlock(parent);

        dunbind(target);

        if ((parent_lk = !dislocked(parent)))
            dlock(parent);

        if ((err = dbind(parent, mnt->mnt_root))) {
            dput(parent);
            if (parent_lk)
                dunlock(parent);

            if (locked_root)
                dunlock(mnt->mnt_root);
            goto error;
        }
        
        dput(parent);
        if (parent_lk)
            dunlock(parent);
    } else if ((err = vfs_mount_droot(mnt->mnt_root))) {
            if (locked_root)
                dunlock(mnt->mnt_root);
            goto error;
    }

    queue_lock(mnt_queue);

    if ((err = enqueue(mnt_queue, (void *)mnt, 1, NULL))) {
        queue_unlock(mnt_queue);
        if (locked_root)
            dunlock(mnt->mnt_root);
        goto error;
    }
    queue_unlock(mnt_queue);

    mnt->mnt_root->d_mnt_stack = stack;

    if (locked_root)
        dunlock(mnt->mnt_root);
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

static int do_new_mount(filesystem_t *fs, const char *src,
                        const char *target, unsigned long flags,
                        void *data, fs_mount_t **pmnt) {
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
        err = -ENOSYS;
        goto error;
    }

    if ((err = fs->get_sb(fs, src, target, flags, data, &sb)))
        goto error;

    mnt->mnt_sb = sb;
    sb->sb_mnt = mnt;
    mnt->mnt_root = sb->sb_root;
    sbunlock(sb);
    *pmnt = mnt;

    return 0;
error:
    if (mnt) fsmount_free(mnt);
    return err;
}

int vfs_mount(const char *src,
              const char *target,
              const char *type,
              unsigned long flags,
              const void *data) {
    int err = 0;
    char *lasttok = NULL;
    fs_mount_t *mnt = NULL;
    filesystem_t *fs = NULL;
    __unused inode_t *isrc = NULL, *itarget = NULL;
    __unused dentry_t *dentry = NULL, *dsrc = NULL, *dtarget = NULL;

    if (target == NULL)
        return -EINVAL;

    if ((err = vfs_getfs(type, &fs)))
        return err;
    
    if ((err = path_get_lasttoken(target, &lasttok))) {
        fsunlock(fs);
        return err;
    }

    if (flags & MS_REMOUNT) {
    } else if (flags & MS_BIND) {
    } else if (flags & MS_MOVE) {
    } else {
        // Do New Mount.
        if ((err = do_new_mount(fs, src, lasttok, flags, (void *)data, &mnt))) {
            kfree(lasttok);
            fsunlock(fs);
            return err;
        }
        goto bind;
    }

    kfree(lasttok);
    fsunlock(fs);

    return 0;
bind:
    if ((err = vfs_lookup(target, NULL, O_RDONLY, 0, 0, &dtarget))) {
        mnt_unlock(mnt);
        fsunlock(fs);
        goto error;
    }

    if ((err = mnt_insert(mnt, dtarget))) {
        dclose(dtarget);
        mnt_unlock(mnt);
        fsunlock(fs);
        goto error;
    }

    dclose(dtarget);
    mnt_unlock(mnt);
    fsunlock(fs);

    return 0;

error:
    if (mnt) fsmount_free(mnt);
    panic("%s: error: %d\n", __func__, err);
    return err;
}