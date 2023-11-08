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
#include <lib/nanojpeg.c>
#include <dev/clocks.h>
#include <boot/boot.h>
#include <ds/stack.h>
#include <video/color_code.h>
#include <dev/cga.h>
#include <lib/ctype.h>
#include <mm/vmm.h>

typedef struct lfb_ctx
{
    int wallbg;
    struct font *fontdata;
    int textlines;
    int lfb_textcc;
    int lfb_textcr;
    uint8_t *wallpaper;
    int textpointer;
    uint8_t cursor_char;
    uint32_t **scanline0;
    uint32_t **scanline1;
    char **textscanline;
    uint8_t cursor_timeout;
    char *lfb_textbuffer;
    uint32_t *lfb_backbuffer;
    uint32_t *lfb_background;
    uint32_t *lfb_frontbuffer;
    int op, fg, bg, transp;
    int cols, rows, cc, cr;
    spinlock_t lock;
} lfb_ctx_t;

lfb_ctx_t ctx;
inode_t *lfb = NULL;
inode_t *lfb_img = NULL;
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

int limeterm_init(void) {
    int err = -ENOMEM;

    memset(&ctx, 0, sizeof ctx);
    ctx.lock = SPINLOCK_INIT();

    if (NULL == (ctx.scanline0 = kcalloc(var_info.height, (sizeof *ctx.scanline0))))
        return err;

    if (NULL == (ctx.scanline1 = kcalloc(var_info.height, (sizeof *ctx.scanline1)))) {
        kfree(ctx.scanline0);
        return err;
    }
    
    if (NULL == (ctx.lfb_backbuffer = kcalloc(1, fix_info.memsz))) {
        kfree(ctx.scanline1);
        kfree(ctx.scanline0);
        return err;
    }

    if (NULL == (ctx.wallpaper = kcalloc(1, var_info.height * fix_info.line_length))) {
        kfree(ctx.lfb_backbuffer);
        kfree(ctx.scanline1);
        kfree(ctx.scanline0);
        return err;
    }

    if ((err = fontctx_alloc(&ctx.fontdata))) {
        kfree(ctx.wallpaper);
        kfree(ctx.lfb_backbuffer);
        kfree(ctx.scanline1);
        kfree(ctx.scanline0);
        return err;
    }

    ctx.cols = (var_info.width / ctx.fontdata->cols);
    ctx.rows = (var_info.height / ctx.fontdata->rows);

    if (NULL == (ctx.lfb_textbuffer = kcalloc(3 * ctx.cols * ctx.rows, sizeof (char)))) {
        kfree(ctx.fontdata->data);
        kfree(ctx.fontdata->glyphs);
        kfree(ctx.fontdata);
        kfree(ctx.wallpaper);
        kfree(ctx.lfb_backbuffer);
        kfree(ctx.scanline1);
        kfree(ctx.scanline0);
        return err;
    }

    if (NULL == (ctx.textscanline = kcalloc(3 * ctx.rows, sizeof (char *)))) {
        kfree(ctx.lfb_textbuffer);
        kfree(ctx.fontdata->data);
        kfree(ctx.fontdata->glyphs);
        kfree(ctx.fontdata);
        kfree(ctx.wallpaper);
        kfree(ctx.lfb_backbuffer);
        kfree(ctx.scanline1);
        kfree(ctx.scanline0);
        return err;
    }


    ctx.lfb_background  = (void *)ctx.wallpaper;
    for (size_t row = 0; row < var_info.height; ++row)
        ctx.scanline0[row] = &(ctx.lfb_background)[row * var_info.width];
    
    ctx.lfb_frontbuffer = (void *)fix_info.addr;
    for (size_t row = 0; row < var_info.height; row++)
        ctx.scanline1[row] = &(ctx.lfb_frontbuffer)[row * var_info.width];
    
    ctx.op = 200;
    ctx.bg = RGB_black;
    ctx.fg = RGB_white;
    ctx.cursor_char = '|';
    ctx.cursor_timeout = 20;

    use_limeterm_cons = 1;
    limeterm_clrscrn();

    printk(cbuf);
    return 0;
}

#include <video/color_code.h>
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

int font_putc(int c, struct lfb_ctx *ctx, int col, int row) {
    char glyph[ctx->fontdata->rows * ctx->fontdata->cols];
    font_bitmap(ctx->fontdata, glyph, c);
    for (int i = 0; i < ctx->fontdata->rows; ++i) {
        int cx = col * ctx->fontdata->cols;
        for (int j = 0; j < ctx->fontdata->cols; ++j) {
            char v = glyph[i * ctx->fontdata->cols + j];
            for (int b = 0; b < 8; ++b) {
                __put_pixel(ctx->scanline1, cx, (row * ctx->fontdata->rows + i), ((v & (1 << b)) ? ctx->fg
                    : (ctx->wallbg ? (int)__get_pixel(ctx->scanline0, cx, (row * ctx->fontdata->rows + i + 1)) : ctx->bg)));
            }
            ++cx;
        }
    }
    return 0;
}

void ctx_putchar(struct lfb_ctx *ctx, int c);
int cxx = 0;

void ctx_putchar(struct lfb_ctx *ctx, int c) {
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

void ctx_puts(struct lfb_ctx *ctx, char *str) {
    while (*str)
        ctx_putchar(ctx, *str++);
}

void limeterm_fill_rect(int x, int y, int w, int h, int color) {
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
            __put_pixel(ctx.scanline1, cx, y, color);
}

void limeterm_clrscrn(void) {
    spin_lock(&ctx.lock);
    if (!ctx.wallpaper)
        limeterm_fill_rect(0, 0, var_info.width, var_info.height, ctx.bg);
    else {
        memcpy(ctx.lfb_frontbuffer, ctx.lfb_background, var_info.height * fix_info.line_length);
        ctx.wallbg = 1;
    }
    ctx.cc = ctx.cr = 0;
    //limeterm_render();
    spin_unlock(&ctx.lock);
}

void limeterm_scroll(void) {
    font_putc(' ', &ctx, ctx.cc, ctx.cr);
    for (int row = ctx.fontdata->rows; row < (int)var_info.height; ++row)
        for (int col = 0; col < (int)var_info.width; ++col)
            __put_pixel((ctx.scanline1), col, (row - ctx.fontdata->rows), __peek_pixel(ctx.scanline0, col, row));
    for (int row = (int)var_info.height - ctx.fontdata->rows; row < (int)var_info.height; ++row)
        for (int col = 0; col < (int)var_info.width; ++col)
            __put_pixel((ctx.scanline1), col, row, (ctx.wallbg ? (int)__get_pixel(ctx.scanline0, col, row) : ctx.bg));
    ctx.cc = 0;
    ctx.cr = ctx.rows - 1;
}

void lfbtext_putc(int c) {
    //__text_putc()
    //char **buff = &ctx.scanline2[ctx.textpointer];
    //__text_putc(buff, ctx.lfb_textcc, ctx.lfb_textcr, c);

    switch (c) {
    case '\n':
        ctx.lfb_textcc = 0;
        ctx.lfb_textcr++;
        break;
    case '\t':
        ctx.lfb_textcc = (ctx.lfb_textcc + 4) & ~3;
        if (ctx.lfb_textcc >= ctx.cols) {
            ctx.lfb_textcr++;
            ctx.lfb_textcc = 0;
        }
        break;
    case '\r':
        ctx.lfb_textcc = 0;
        break;
    case '\b':
        ctx.lfb_textcc--;
        if (ctx.lfb_textcc < 0) {
            ctx.lfb_textcr--;
            if (ctx.lfb_textcr >= 0)
                ctx.lfb_textcc = ctx.cols - 1;
            else
                ctx.lfb_textcr = ctx.lfb_textcc = 0;
        }
        font_putc(' ', &ctx, ctx.lfb_textcc, ctx.lfb_textcr);
        break;
    default:
        font_putc(c, &ctx, ctx.lfb_textcc, ctx.lfb_textcr);
        ctx.lfb_textcc++;
        if (ctx.lfb_textcc >= ctx.cols) {
            ctx.lfb_textcr++;
            ctx.lfb_textcc = 0;
        }
    }
    if (ctx.lfb_textcr >= ctx.rows)
        limeterm_scroll();
}

void limeterm_setcolor(int bg, int fg) {
    spin_lock(&ctx.lock);
    ctx.bg = brew_color(bg);
    ctx.fg = brew_color(fg);
    spin_unlock(&ctx.lock);
}

void limeterm_putc(int c) {
    cxx = c;

    if (c == '\e') {
        limeterm_esc = 1;
        return;
    }

    if (vmm_active()) {

        if (limeterm_esc) {
            size_t val = 0;
            static int open_ = 0;
            static int16_t fg = 0;
            static int16_t bg = 0;
            if (c == '[') {
                open_ = 1;
                spin_lock(&ctx.lock);
                val = ((size_t)ctx.fg << 32 | ctx.bg);
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
                        bg += c * pw;
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
                    fg = 0;
                    bg = 0;
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
                    fg += c * pw;
                }
                stack_unlock(limeterm_chars);
                limeterm_setcolor(bg, fg);
                fg = 0;
                bg = 0;
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

void ctx_drawcursor(struct lfb_ctx *ctx) {
    loop() {
    debugloc();
        spin_lock(&ctx->lock);
        font_putc(ctx->cursor_char, ctx, ctx->cc, ctx->cr);
        spin_unlock(&ctx->lock);
        timer_wait(CLK_ANY, ctx->cursor_timeout);
    debugloc();
        spin_lock(&ctx->lock);
        font_putc(' ', ctx, ctx->cc, ctx->cr);
        spin_unlock(&ctx->lock);
        timer_wait(CLK_ANY, ctx->cursor_timeout);
    }
}

static void limeterm_cursor(void *arg __unused) {
    if (use_limeterm_cons == 0)
        thread_exit(-ENOENT);
    ctx_drawcursor(&ctx);
    loop();
}
BUILTIN_THREAD(lfb_text, limeterm_cursor, NULL);