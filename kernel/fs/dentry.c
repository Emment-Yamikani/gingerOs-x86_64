#include <fs/fs.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <lib/printk.h>
#include <lib/string.h>

static LIST_HEAD(d_cache_list);
spinlock_t *dcache_list_lock = &SPINLOCK_INIT();

list_head_t *dcache_list_get(void)
{
    return &d_cache_list;
}

int dentry_cache(void)
{
    return 0;
}

static void dentry_free(dentry_t *dentry)
{
    printk("%s: dentry(%s)->drefs: %ld\n", __FILE__, dentry->d_name, dentry->d_count);
    if (dentry->d_next)
    {
        dentry_lock(dentry->d_next);
        dentry->d_next->d_prev = dentry->d_prev;
        if (!dentry->d_prev)
            dentry_release(dentry->d_next);
        dentry_unlock(dentry->d_next);
        dentry_release(dentry);
    }

    if (dentry->d_prev)
    {
        dentry_lock(dentry->d_prev);
        dentry->d_prev->d_next = dentry->d_next;
        if (!dentry->d_next)
            dentry_release(dentry->d_prev);
        dentry_unlock(dentry->d_prev);
        dentry_release(dentry);
    }
    else if (dentry->d_parent)
    {
        dentry_lock(dentry->d_parent);
        dentry->d_parent->d_child = dentry->d_next;
        dentry_unlock(dentry->d_parent);
        dentry_release(dentry);
    }

    dentry_lock(dentry->d_parent);
    if (dentry->d_parent)
        dentry_release(dentry->d_parent);
    dentry_unlock(dentry->d_parent);

    if (dentry->d_name)
        kfree(dentry->d_name);

    list_del(&dentry->d_list);
    *dentry = (dentry_t){0};

    kfree(dentry);
}

void dentry_release(dentry_t *dentry)
{
    if (!dentry)
        return;
    dentry_assert_locked(dentry);
    dentry->d_count--;
    if (dentry->d_count <= 0) {
        if (dentry->d_inode) {
            ilock(dentry->d_inode);
            iclose(dentry->d_inode);
            list_del(&dentry->d_alias);
            iunlock(dentry->d_inode);
        }

        dentry_free(dentry);
    }
}

int dentry_dup(dentry_t *dentry)
{
    if (!dentry)
        return -EINVAL;
    dentry_assert_locked(dentry);
    ++dentry->d_count;
    return 0;
}

dentry_t *dentry_alloc(const char *name)
{
    char *d_name = NULL;
    dentry_t *dentry = NULL;

    if ((d_name = strdup(name)) == NULL)
        goto error;

    if ((dentry = kcalloc(1, sizeof *dentry)) == NULL)
        goto error;

    dentry->d_count = 1;
    dentry->d_name = d_name;
    dentry->d_lock = SPINLOCK_INIT();

    dentry_lock(dentry);

    INIT_LIST_HEAD(&dentry->d_list);
    INIT_LIST_HEAD(&dentry->d_alias);

    spin_lock(dcache_list_lock);
    list_add(&dentry->d_list, &d_cache_list);
    spin_unlock(dcache_list_lock);

    return dentry;
error:
    if (d_name)
        kfree(d_name);
    if (dentry)
        kfree(dentry);
    return NULL;
}

int dentry_bind(dentry_t *d_parent, dentry_t *d_child)
{
    int err = -EINVAL;
    dentry_t *d_last = NULL, *d_next = NULL;

    if (!d_parent || !d_child || (d_parent == d_child))
        goto error;

    // if (!d_child->d_inode)
    // goto error;

    dentry_assert_locked(d_child);
    dentry_assert_locked(d_parent);

    if (!d_child->d_name)
        goto error;

    if ((!compare_strings("/", d_child->d_name)))
        goto error;

    if (!dentry_find(d_parent, d_child->d_name, NULL))
        return -EEXIST;

    d_child->d_next = NULL;
    d_child->d_prev = NULL;
    d_child->d_parent = NULL;

    if ((err = dentry_dup(d_parent)))
        goto error;

    if ((err = dentry_dup(d_child)))
    {
        dentry_release(d_parent);
        goto error;
    }

    if (d_parent->d_child == NULL)
    {
        d_child->d_parent = d_parent;
        d_parent->d_child = d_child;
        goto done;
    }

    forlinked(d_node, d_parent->d_child, d_next)
    {
        dentry_lock(d_node);
        if (d_last)
            dentry_unlock(d_last);
        d_next = d_node->d_next;
        d_last = d_node;
    }

    if ((err = dentry_dup(d_last)))
    {
        dentry_unlock(d_last);
        dentry_release(d_child);
        dentry_release(d_parent);
        goto error;
    }

    d_last->d_next = d_child;
    dentry_unlock(d_last);

    d_child->d_prev = d_last;
    d_child->d_parent = d_parent;

done:
    return 0;
error:
    return err;
}

int dentry_find(dentry_t *d_parent, const char *name, dentry_t **d_child)
{
    int err = 0;
    dentry_t *d_next = NULL;

    if (!d_parent)
        return -EINVAL;

    dentry_assert_locked(d_parent);

    if (!compare_strings(".", name)) {
        if (d_child)
        {
            if ((err = dentry_dup(d_parent)))
                goto error;
            *d_child = d_parent;
        }
        return 0;
    } else if (!compare_strings("..", name)) {
        if (d_child)
        {
            if (d_parent->d_parent)
                dentry_lock(d_parent->d_parent);
            if ((err = dentry_dup(d_parent->d_parent)))
            {
                if (d_parent->d_parent)
                    dentry_unlock(d_parent->d_parent);
                goto error;
            }
            *d_child = d_parent->d_parent;
        }
        return 0;
    }

    forlinked(d_node, d_parent->d_child, d_next)
    {
        dentry_lock(d_node);
        d_next = d_node->d_next;
        if (!compare_strings(d_node->d_name, name))
        {
            if (d_child)
            {
                if ((err = dentry_dup(d_node)))
                {
                    dentry_unlock(d_node);
                    goto error;
                }
                *d_child = d_node;
            }
            else
                dentry_unlock(d_node);
            return 0;
        }
        dentry_unlock(d_node);
    }

    return -ENOENT;
error:
    return err;
}

void dentry_close(dentry_t *dentry)
{
    if (!dentry)
        return;

    dentry_assert_locked(dentry);

    if (!dentry->d_child)
    {
        if (dentry->d_next)
        {
            dentry_lock(dentry->d_next);
            dentry->d_next->d_prev = dentry->d_prev;
            if (!dentry->d_prev)
                dentry_release(dentry->d_next);
            dentry_unlock(dentry->d_next);
            dentry_release(dentry);
        }

        if (dentry->d_prev)
        {
            dentry_lock(dentry->d_prev);
            dentry->d_prev->d_next = dentry->d_next;
            if (!dentry->d_next)
                dentry_release(dentry->d_prev);
            dentry_unlock(dentry->d_prev);
            dentry_release(dentry);
        }
        else if (dentry->d_parent)
        {
            dentry_lock(dentry->d_parent);
            dentry->d_parent->d_child = dentry->d_next;
            dentry_unlock(dentry->d_parent);
            dentry_release(dentry);
        }

        dentry_lock(dentry->d_parent);
        if (dentry->d_parent)
            dentry_release(dentry->d_parent);
        dentry_unlock(dentry->d_parent);

        dentry->d_next = NULL;
        dentry->d_prev = NULL;
        dentry->d_parent = NULL;
    }

    dentry_release(dentry);
}

int dentry_iset(dentry_t *dentry, inode_t *ip, int overwrite) {
    int err = 0;
    dentry_assert_locked(dentry);
    iassert_locked(ip);

    if (dentry->d_inode && !overwrite)
        return -EALREADY;
    else if (dentry->d_inode)
        err = iclose(dentry->d_inode);
        
    if (err)
        return err;
    
    return iopen(ip, dentry);
}