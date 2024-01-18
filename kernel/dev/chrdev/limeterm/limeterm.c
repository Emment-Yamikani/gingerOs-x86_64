#include <dev/fb.h>
#include <fs/fs.h>
#include <mm/kalloc.h>
#include <font/tinyfont.h>
#include <lib/printk.h>
#include <bits/errno.h>
#include <ginger/jiffies.h>
#include <video/color_code.h>
#include <sys/sleep.h>
#include <dev/limeterm.h>
#include <sys/thread.h>
#include <lib/string.h>
#include <lib/stdint.h>
#include <lib/nanojpeg.h>
#include <dev/clocks.h>
#include <boot/boot.h>
#include <ds/stack.h>
#include <video/color_code.h>
#include <dev/cga.h>
#include <lib/ctype.h>
#include <mm/vmm.h>
#include <video/color_code.h>

typedef struct limeterm_ctx {
    int         op;             // foreground opacity.
    int         cc;             // character col.
    int         cr;             // character row.
    int         cols;           // number of char cols.
    int         rows;           // number of char rows.
    int         transp;         // is transparency set.
    int         wallbg;         // use wallpaper image for background?
    int         fg_color;       // foreground color.
    int         bg_color;       // background color.
    struct font *font;          // tinyfont data.
    uint8_t     *wallpaper;     // decoded wallpaper image.
    uint32_t    **scanline0;    // multi-dimensional array of background pixel pointers.
    uint32_t    **scanline1;    // multi-dimensional array of framebuffer pixel pointers.
    uint32_t    *background;    // memory to hold the backgroung image.
    uint8_t     cursor_char;    // character to use for the cursor.
    uint32_t    *frontbuffer;   // frontbuffer of framebuffer.
    int         cursor_timeout; // cursor blink timeout.
    spinlock_t lock;            // ctx lock.
} limeterm_ctx_t;

limeterm_ctx_t ctx;
inode_t *limeterm = NULL;
inode_t *limeterm_img = NULL;
volatile int use_limeterm_cons = 0;
volatile int limeterm_esc = 0;
__unused static int cbufi = 0;
__unused static char cbuf[PGSZ] = {0};
static stack_t *limeterm_chars = STACK_NEW();
static stack_t *limeterm_themes = STACK_NEW();

const char *wallpaper_path[] = {
    "snow_800x600.jpg",
    "snow_1024x768.jpg",
    NULL,
};

#define __peek_pixel(f, x, y) ((f)[(int)(y)][(int)(x)])

#define __set_pixel(f, x, y, c) (__peek_pixel((f), (x), (y)) = (c))

#define __put_pixel(f, x, y, c) ({if (((x) < (int)var_info.width) &&\
                                     ((y) < (int)var_info.height) && ((x) >= 0) && ((y) >= 0))\
                                     __set_pixel((f), (x), (y), (c)); })

#define __get_pixel(f, x, y) ({uint32_t px = 0; if (((x) < (int)var_info.width) &&\
                                     ((y) < (int)var_info.height) && ((x) >= 0) && ((y) >= 0))\
                                     px = __peek_pixel((f), (x), (y)); px; })

#define __text_peek(buff, x, y) ((buff)[(int)(y)][(int)(x)])

#define __text_setc(buff, x, y, c) (__text_peek((buff), (x), (y)) = (c))

#define __text_putc(buff, x, y, c) ({if (((x) < (int)ctx.cols) && ((y) < (int)ctx.rows)\
                                        && ((x) >= 0) && ((y) >= 0))\
                                            __text_setc((buff), (x), (y), (c)); })

#define __text_getc(buff, x, y) ({if (((x) < (int)ctx.cols) && ((y) < (int)ctx.rows)\
                                        && ((x) >= 0) && ((y) >= 0))\
                                            int c = __text_peek((buff), (x), (y)); c; })

void limeterm_fill_rect(uint32_t **scanline, int x, int y, int w, int h, int color) {
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if ((w < 0) || (h < 0))
        return;

    int xmax = w + x, ymax = h + y;
    if (xmax > (int)var_info.width)
        xmax = var_info.width;
    if (ymax > (int)var_info.height)
        ymax = var_info.height;

    for (; y < ymax; ++y)
        for (int cx = x; cx < xmax; ++cx)
            __put_pixel(scanline, cx, y, color);
}

int limeterm_init(void) {
    int err = -ENOMEM;

    memset(&ctx, 0, sizeof ctx);
   
    ctx.op              = 200;
    ctx.cursor_char     = '|';
    ctx.cursor_timeout  = 150;
    ctx.fg_color        = RGB_dark_cyan;
    ctx.bg_color        = RGB_black;
    ctx.bg_color        = 0x00002b2b;
    ctx.lock            = SPINLOCK_INIT();

    if ((err = fontctx_alloc(&ctx.font)))
        goto error;

    ctx.cols = (var_info.width / ctx.font->cols);
    ctx.rows = (var_info.height / ctx.font->rows);

    err = -ENOMEM;
    if (NULL == (ctx.scanline0 = kcalloc(var_info.height, (sizeof *ctx.scanline0))))
        goto error;

    if (NULL == (ctx.scanline1 = kcalloc(var_info.height, (sizeof *ctx.scanline1))))
        goto error;

    if (NULL == (ctx.background = kcalloc(1, fix_info.memsz)))
        goto error;

    for (int row = 0; row < var_info.height; ++row)
        ctx.scanline0[row] = &(ctx.background)[row * var_info.width];
    
    ctx.frontbuffer = (void *)fix_info.addr;
    for (int row = 0; row < var_info.height; row++)
        ctx.scanline1[row] = &(ctx.frontbuffer)[row * var_info.width];

    use_limeterm_cons = 1;
    limeterm_clrscrn();

    printk(cbuf);
    return 0;
error:
    if (ctx.background)
        kfree(ctx.background);
    
    if (ctx.scanline1)
        kfree(ctx.scanline1);
    
    if (ctx.scanline0)
        kfree(ctx.scanline0);
    
    if (ctx.font) {
        kfree(ctx.font->data);
        kfree(ctx.font->glyphs);
        kfree(ctx.font);
    }
    return err;
}

int brew_color(int __color) {
    switch (__color) {
    case CGA_BLACK:
        __color = RGB_black;
        break;
    case CGA_DARK_GREY:
        __color = RGB_gray_grey;
        break;
    case CGA_BLUE:
        __color = RGB_blue;
        break;
    case CGA_LIGHT_BLUE:
        __color = RGB_light_blue;
        break;
    case CGA_GREEN:
        __color = RGB_green;
        break;
    case CGA_LIGHT_GREEN:
        __color = RGB_light_green;
        break;
    case CGA_CYAN:
        __color = RGB_cyan;
        break;
    case CGA_LIGHT_CYAN:
        __color = RGB_light_cyan;
        break;
    case CGA_RED:
        __color = RGB_dark_red;
        break;
    case CGA_LIGHT_RED:
        __color = RGB_red;
        break;
    case CGA_MAGENTA:
        __color = RGB_magenta;
        break;
    case CGA_LIGHT_MAGENTA:
        __color = RGB_light_pink;
        break;
    case CGA_BROWN:
        __color = RGB_brown;
        break;
    case CGA_YELLOW:
        __color = RGB_yellow;
        break;
    case CGA_LIGHT_GREY:
        __color = RGB_light_gray;
        break;
    case CGA_WHITE:
        __color = RGB_white;
        break;
    }
    return __color;
}

int font_putc(int c, struct limeterm_ctx *ctx, int col, int row) {
    char glyph[ctx->font->rows * ctx->font->cols];
    font_bitmap(ctx->font, glyph, c);
    for (int i = 0; i < ctx->font->rows; ++i) {
        int cx = col * ctx->font->cols;
        for (int j = 0; j < ctx->font->cols; ++j) {
            char v = glyph[i * ctx->font->cols + j];
            for (int b = 0; b < 8; ++b) {
                __put_pixel(ctx->scanline1, cx, (row * ctx->font->rows + i), ((v & (1 << b)) ? ctx->fg_color
                    : (ctx->wallbg ? (int)__get_pixel(ctx->scanline0, cx, (row * ctx->font->rows + i + 1)) : ctx->bg_color)));
            }
            ++cx;
        }
    }
    return 0;
}

void ctx_putchar(struct limeterm_ctx *ctx, int c) {
    font_putc(' ', ctx, ctx->cc, ctx->cr);
    switch (c) {
    case '\n':
        ctx->cc = 0;
        ctx->cr++;
        break;
    case '\t':
        ctx->cc = (ctx->cc + 4) & ~3;
        if (ctx->cc >= ctx->cols) {
            ctx->cr++;
            ctx->cc = 0;
        }
        break;
    case '\r':
        ctx->cc = 0;
        break;
    case '\b':
        ctx->cc--;
        if (ctx->cc < 0) {
            ctx->cr--;
            if (ctx->cr >= 0)
                ctx->cc = ctx->cols - 1;
            else
                ctx->cr = ctx->cc = 0;
        }
        font_putc(' ', ctx, ctx->cc, ctx->cr);
        break;
    default:
        font_putc(c, ctx, ctx->cc, ctx->cr);
        ctx->cc++;
        if (ctx->cc >= ctx->cols) {
            ctx->cr++;
            ctx->cc = 0;
        }
    }
    if (ctx->cr >= ctx->rows)
        limeterm_scroll();
}

void ctx_puts(struct limeterm_ctx *ctx, char *str) {
    while (*str)
        ctx_putchar(ctx, *str++);
}

void limeterm_clrscrn(void) {
    spin_lock(&ctx.lock);
    limeterm_fill_rect(ctx.scanline0, 0, 0, var_info.width, var_info.height, ctx.bg_color);
    memcpy(ctx.frontbuffer, ctx.background, fix_info.memsz);
    ctx.cc = ctx.cr = 0;
    spin_unlock(&ctx.lock);
}

void limeterm_scroll(void) {
    for (int row = ctx.font->rows; row < var_info.height; ++row) {
        for (int col = 0; col < var_info.width; ++col) {
            __put_pixel(ctx.scanline1, col, row - ctx.font->rows,
                __peek_pixel(ctx.scanline1, col, row));
        }
    }

    for (int row = var_info.height - ctx.font->rows; row < var_info.height; ++row) {
        for (int col = 0; col < var_info.width; ++col){
            __put_pixel((ctx.scanline1), col, row, (ctx.wallbg ?
                (int)__get_pixel(ctx.scanline0, col, row) : ctx.bg_color));
        }
    }

    ctx.cc = 0;
    ctx.cr = ctx.rows - 1;
}

void limeterm_setcolor(int bg_color, int fg_color) {
    spin_lock(&ctx.lock);
    ctx.bg_color = brew_color(bg_color);
    ctx.fg_color = brew_color(fg_color);
    spin_unlock(&ctx.lock);
}

void limeterm_putc(int c) {
    if (c == '\e') {
        limeterm_esc = 1;
        return;
    }

    if (vmm_active()) {

        if (limeterm_esc) {
            size_t val = 0;
            static int open_ = 0;
            static int16_t fg_color = 0;
            static int16_t bg_color = 0;
            if (c == '[') {
                open_ = 1;
                spin_lock(&ctx.lock);
                val = ((size_t)ctx.fg_color << 32 | ctx.bg_color);
                spin_unlock(&ctx.lock);
                stack_lock(limeterm_themes);
                stack_push(limeterm_themes, (void *)val);
                stack_unlock(limeterm_themes);
                return;
            }

            if (open_) {
                if (c == ';') {
                    open_ = 0;
                    stack_lock(limeterm_chars);
                    for (int ni = 0, i = 0, pw = 1;
                        stack_pop(limeterm_chars, (void **)((long *)&c)) == 0; ni++) {
                        for (pw = 1, i = ni; i; --i)
                            pw *= 8;
                        bg_color += c * pw;
                    }
                    stack_unlock(limeterm_chars);
                    return;
                }

                if (c == 'm') {
                    stack_lock(limeterm_themes);
                    stack_pop(limeterm_themes, (void **)&val);
                    stack_pop(limeterm_themes, (void **)&val);
                    stack_unlock(limeterm_themes);
                    limeterm_setcolor(val, val >> 32);
                    fg_color = 0;
                    bg_color = 0;
                    open_ = 0;
                    limeterm_esc = 0;
                    return;
                }

                if (isdigit(c)) {
                    stack_lock(limeterm_chars);
                    stack_push(limeterm_chars, (void *)(long)(c - '0'));
                    stack_unlock(limeterm_chars);
                }
                return;
            }

            if (c == 'm') {
                stack_lock(limeterm_chars);
                for (int ni = 0, i = 0, pw = 1;
                    stack_pop(limeterm_chars, (void **)((long *)&c)) == 0; ni++) {
                    for (pw = 1, i = ni; i; --i)
                        pw *= 8;
                    fg_color += c * pw;
                }
                stack_unlock(limeterm_chars);
                limeterm_setcolor(bg_color, fg_color);
                fg_color = 0;
                bg_color = 0;
                limeterm_esc = 0;
                return;
            }
            if (isdigit(c)) {
                stack_lock(limeterm_chars);
                stack_push(limeterm_chars, (void *)(long)(c - '0'));
                stack_unlock(limeterm_chars);
            }
            return;
        }
    } else {
        if (limeterm_esc) {
            if (c == 'm')
                limeterm_esc = 0;
            return;
        }
    }

    switch (c) {
    case _SC_UP:
        limeterm_puts("^[[A");
        return;
    case _SC_LEFT:
        limeterm_puts("^[[D");
        return;
    case _SC_RIGHT:
        limeterm_puts("^[[C");
        return;
    case _SC_DOWN:
        limeterm_puts("^[[B");
        return;
    case CTRL('C'):
        limeterm_puts("^C");
        return;
    default:
        spin_lock(&ctx.lock);
        ctx_putchar(&ctx, c);
        spin_unlock(&ctx.lock);
    }
}

void limeterm_sputc(int c) {
    cbuf[cbufi++] = (char)c;
}

size_t limeterm_puts(const char *s) {
    char *S = (char *)s;
    while (*S)
        ctx_putchar(&ctx, *S++);
    return S - s;
}

static void limeterm_cursor(void *arg __unused) {
    if (use_limeterm_cons == 0)
        thread_exit(-ENOENT);
    loop() {
        spin_lock(&ctx.lock);
        font_putc(ctx.cursor_char, &ctx, ctx.cc, ctx.cr);
        spin_unlock(&ctx.lock);
        jiffies_sleep(ms_TO_jiffies(ctx.cursor_timeout));
        spin_lock(&ctx.lock);
        font_putc(' ', &ctx, ctx.cc, ctx.cr);
        spin_unlock(&ctx.lock);
        jiffies_sleep(ms_TO_jiffies(ctx.cursor_timeout));
    }
}
BUILTIN_THREAD(limeterm_text, limeterm_cursor, NULL);