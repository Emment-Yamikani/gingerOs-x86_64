#include <bits/errno.h>
#include <fs/fs.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <lib/types.h>

// TODO: Solidify mount implementation.

static queue_t *mnt_queue = QUEUE_NEW(); // Queue for managing all mounts

/**
 * Allocate a new fs_mount_t structure.
 * Initializes the structure and locks it.
 */
fs_mount_t *alloc_fsmount(void) {
    fs_mount_t *mnt = NULL;
    if (NULL == (mnt = (fs_mount_t *)kmalloc(sizeof(*mnt))))
        return NULL;

    memset(mnt, 0, sizeof(*mnt));
    mnt->mnt_lock = SPINLOCK_INIT();
    mnt_lock(mnt); // Lock immediately after initialization
    return mnt;
}

/**
 * Free an fs_mount_t structure.
 * Ensures the mount is locked, releases its resources, and frees the memory.
 */
void fsmount_free(fs_mount_t *mnt) {
    if (!mnt)
        return;

    // Lock if not already locked
    if (!mnt_islocked(mnt))
        mnt_lock(mnt);

    // Release the root dentry if it exists
    if (mnt->mnt_root) {
        dlock(mnt->mnt_root);
        dclose(mnt->mnt_root);
    }

    mnt_unlock(mnt);
    kfree(mnt);
}

/**
 * Insert a mount point into the target dentry.
 * Ensures proper locking and adds the mount to the global mount queue.
 */
static int mnt_insert(fs_mount_t *mnt, dentry_t *target) {
    int         err             = 0;
    int         root_locked     = 0;
    int         parent_locked   = 0;
    stack_t     *stack          = NULL;
    dentry_t    *parent         = NULL;

    // Validate input parameters
    if (!mnt || !target)
        return -EINVAL;

    mnt_assert_locked(mnt);  // Ensure mount is locked
    dassert_locked(target);  // Ensure target dentry is locked

    if (!mnt->mnt_root)
        return -EINVAL;

    // Ensure the root dentry is locked
    if (!dislocked(mnt->mnt_root)) {
        root_locked = 1;
        dlock(mnt->mnt_root);
    }

    stack = &mnt->mnt_root->d_mnt_stack;

    // Add the target dentry to the mount stack
    stack_lock(stack);
    if ((err = stack_push(stack, (void *)target))) {
        stack_unlock(stack);
        if (root_locked)
            dunlock(mnt->mnt_root);
        return err;
    }
    stack_unlock(stack);

    // Handle parent dentry if it exists
    parent = target->d_parent;
    if (parent) {
        if (!dislocked(parent)) {
            parent_locked = 1;
            dlock(parent);
        }

        ddup(parent);        // Increment reference count
        if (parent_locked)
            dunlock(parent);
        
        dunbind(target);     // Unbind the target from its parent

        if (!dislocked(parent)) {
            parent_locked = 1;
            dlock(parent);
        }

        if ((err = dbind(parent, mnt->mnt_root))) {
            dput(parent);
            if (parent_locked)
                dunlock(parent);
            if (root_locked)
                dunlock(mnt->mnt_root);
            return err;
        }
        dput(parent);
        if (parent_locked)
            dunlock(parent);
    } else { // Root-level mount (e.g., "/")
        if ((err = vfs_mount_droot(mnt->mnt_root))) {
            if (root_locked)
                dunlock(mnt->mnt_root);
            return err;
        }
    }

    // Add the mount to the global mount queue
    queue_lock(mnt_queue);
    if ((err = enqueue(mnt_queue, (void *)mnt, 1, NULL))) {
        queue_unlock(mnt_queue);
        if (root_locked)
            dunlock(mnt->mnt_root);
        return err;
    }
    queue_unlock(mnt_queue);

    if (root_locked)
        dunlock(mnt->mnt_root);

    return 0;
}

/**
 * Remove a mount from the global mount queue.
 */
__unused static int mnt_remove(fs_mount_t *mnt) {
    int err = 0;

    if (!mnt)
        return -EINVAL;

    mnt_assert_locked(mnt);  // Ensure mount is locked

    queue_lock(mnt_queue);
    if ((err = queue_remove(mnt_queue, (void *)mnt))) {
        queue_unlock(mnt_queue);
        return err;
    }
    queue_unlock(mnt_queue);

    return 0;
}

/**
 * Move a mount to a new parent dentry.
 */
static int mnt_move(fs_mount_t *mnt, dentry_t *new_parent) {
    dentry_t *old_parent;
    int err = 0;

    mnt_assert_locked(mnt);
    dassert_locked(new_parent);

    old_parent = mnt->mnt_root->d_parent;
    if (old_parent) {
        dunbind(mnt->mnt_root);
    }

    err = dbind(new_parent, mnt->mnt_root);
    if (err)
        return err;

    mnt->mnt_root->d_parent = new_parent;

    return 0;
}

static int vfs_find_mount(const char *target, fs_mount_t **mnt) {
    if (target == NULL || mnt == NULL)
        return -EINVAL;

    return -ENOSYS;
}

/**
 * Handle remounting a filesystem.
 */
static int vfs_handle_remount(const i8 *target, filesystem_t *fs, u64 flags, const void *data) {
    int         err     = 0;
    fs_mount_t  *mnt    = NULL;

    // Find the mount point
    if ((err = vfs_find_mount(target, &mnt)))
        return err;

    // Perform the remount operation
    if (fs->remount == NULL)
        return -ENOSYS;

    mnt_lock(mnt);
    err = fs->remount(fs, mnt, flags, data);
    mnt_unlock(mnt);

    return err;
}

/**
 * Handle bind mounting a source dentry onto a target.
 */
static int vfs_handle_bind(const i8 *src, const i8 *target) {
    dentry_t *src_dentry = NULL, *target_dentry = NULL;
    int err;

    // Lookup source and target dentries
    if ((err = vfs_lookup(src, NULL, 0, &src_dentry)))
        return err;

    if ((err = vfs_lookup(target, NULL, O_EXCL, &target_dentry))) {
        dclose(src_dentry);
        return err;
    }

    // Perform the bind operation
    err = dbind(target_dentry, src_dentry);

    dclose(target_dentry);
    dclose(src_dentry);

    return err;
}

/**
 * Handle moving a mount to a new target location.
 */
static int vfs_handle_move(const i8 *src, const i8 *target) {
    fs_mount_t *mnt = NULL;
    dentry_t *target_dentry = NULL;
    int err;

    // Find the source mount and target dentry
    if ((err = vfs_find_mount(src, &mnt)))
        return err;

    if ((err = vfs_lookup(target, NULL, O_EXCL, &target_dentry)))
        return err;

    // Perform the move operation
    mnt_lock(mnt);
    err = mnt_move(mnt, target_dentry);
    mnt_unlock(mnt);

    dclose(target_dentry);

    return err;
}

static int vfs_handle_new_mount(filesystem_t *fs, const char *src, const char *target, u64 flags, void *data, fs_mount_t **pmnt) {
    int             err = 0;
    fs_mount_t      *mnt= NULL;
    superblock_t    *sb = NULL;

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

    mnt->mnt_sb     = sb;
    sb->sb_mnt      = mnt;
    mnt->mnt_root   = sb->sb_root;
    sbunlock(sb);
    *pmnt = mnt;

    return 0;
error:
    if (mnt) fsmount_free(mnt);
    return err;
}

int vfs_mount(const i8 *src, const i8 *target, const i8 *type, u64 flags, const void *data) {
    int                 err             = 0;
    filesystem_t        *fs             = NULL;
    fs_mount_t          *mnt            = NULL;
    char                *last_token     = NULL;
    dentry_t            *target_dentry  = NULL;

    if (target == NULL || type == NULL)
        return -EINVAL;

    // Lookup filesystem type
    if ((err = vfs_getfs(type, &fs)))
        return err;

    // Get the last token of the target path (e.g., "dir" from "/mnt/dir")
    if ((err = path_get_lasttoken(target, &last_token))) {
        fsunlock(fs);
        return err;
    }

    // Handle special flags
    if (flags & MS_REMOUNT) {
        // Handle remount
        err = vfs_handle_remount(last_token, fs, flags, data);
        goto cleanup;
    } else if (flags & MS_BIND) {
        // Handle bind mount
        err = vfs_handle_bind(src, last_token);
        goto cleanup;
    } else if (flags & MS_MOVE) {
        // Handle move mount
        err = vfs_handle_move(src, last_token);
        goto cleanup;
    } else {
        // Handle a new mount
        if ((err = vfs_handle_new_mount(fs, src, last_token, flags, (void *)data, &mnt)))
            goto cleanup;

        // Bind the new mount to the target path
        if ((err = vfs_lookup(target, NULL, O_EXCL, &target_dentry))) {
            fsmount_free(mnt);
            goto cleanup;
        }

        if ((err = mnt_insert(mnt, target_dentry))) {
            dclose(target_dentry);
            fsmount_free(mnt);
            goto cleanup;
        }

        dclose(target_dentry);
        mnt_unlock(mnt);
    }

    // Cleanup and return success
    err = 0;

cleanup:
    kfree(last_token);
    fsunlock(fs);

    if (err)
        printk("%s:%d: %s(): error: %d\n", __FILE__, __LINE__, __func__, err);

    return err;
}

int mount(const char *src, const char *target, const char *type, u64 flags, const void *data) {
    return vfs_mount(src, target, type, flags, data);
}