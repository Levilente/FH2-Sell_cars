/* SellCar - pure helpers (no game access): number and text formatting, the price query. Host-testable. */
#ifndef SC_TEXT_H
#define SC_TEXT_H
#include <stddef.h>
#include <stdint.h>

typedef struct { char *p; size_t n, cap; } ScBuf;
static void sc_begin(ScBuf *b, char *mem, size_t cap) { b->p = mem; b->n = 0; b->cap = cap; if (cap) mem[0] = 0; }
static void sc_addc(ScBuf *b, char c) { if (b->n + 1 < b->cap) { b->p[b->n++] = c; b->p[b->n] = 0; } }
static void sc_adds(ScBuf *b, const char *s) { while (*s) sc_addc(b, *s++); }
static void sc_addn(ScBuf *b, long v) {
    char t[24]; int k = 0; unsigned long u;
    if (v < 0) { sc_addc(b, '-'); u = 0UL - (unsigned long)v; } else u = (unsigned long)v;
    if (!u) t[k++] = '0';
    while (u) { t[k++] = (char)('0' + u % 10); u /= 10; }
    while (k) sc_addc(b, t[--k]);
}
/* 1234567 -> "1,234,567" */
static void sc_addn_grouped(ScBuf *b, long v) {
    char t[32]; int k = 0; unsigned long u;
    if (v < 0) { sc_addc(b, '-'); u = 0UL - (unsigned long)v; } else u = (unsigned long)v;
    if (!u) t[k++] = '0';
    int d = 0;
    while (u) { if (d && d % 3 == 0) t[k++] = ','; t[k++] = (char)('0' + u % 10); u /= 10; d++; }
    while (k) sc_addc(b, t[--k]);
}

/* The sale value of one owned car, computed by the game's own database:
   base price of the model * base_pct / 100  +  what was paid for its parts * up_pct / 100 (whole credits).
   The result is returned in a column named V. */
static void sc_price_sql(char *out, size_t cap, long garage_id, int base_pct, int up_pct) {
    ScBuf b; sc_begin(&b, out, cap);
    sc_adds(&b, "SELECT CAST((IFNULL((SELECT BaseCost FROM Data_Car WHERE Id=(SELECT CarId FROM Profile0_Career_Garage WHERE Id=");
    sc_addn(&b, garage_id);
    sc_adds(&b, ")),0)*");
    sc_addn(&b, base_pct);
    sc_adds(&b, "+IFNULL((SELECT SUM(PricePaid) FROM Profile0_Career_PurchasedParts WHERE GarageId=");
    sc_addn(&b, garage_id);
    sc_adds(&b, "),0)*");
    sc_addn(&b, up_pct);
    sc_adds(&b, ")/100 AS INTEGER) AS V");
}

static int sc_clamp_pct(long v, int def) { return (v < 0 || v > 100) ? def : (int)v; }

/* ASCII to UTF-16 into a fixed buffer */
static void sc_widen(uint16_t *out, size_t cap, const char *s) {
    size_t i = 0;
    if (!cap) return;
    while (s[i] && i + 1 < cap) { out[i] = (uint16_t)(unsigned char)s[i]; i++; }
    out[i] = 0;
}

#endif
