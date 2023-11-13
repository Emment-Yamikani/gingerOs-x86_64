#include <lib/stdint.h>
#include <arch/firmware/bios.h>
#include <lib/string.h>
#include <arch/firmware/acpi.h>
#include <bits/errno.h>
#include <arch/paging.h>

static xsdt_t  *XSDT = NULL;
static rsdt_t  *RSDT = NULL;
static rsdp20_t  *RSDP = NULL;

int acpi_validate_table(char *addr, size_t size) {
    uint8_t sum = 0;
    while (size--)
        sum += *addr++;
    return sum == 0;
}

void *acpi_findrsdp(void) {
    char *bios = (char *)VMA2HI(EBDA);
    for (; bios < (char *)(VMA2HI(EBDA) + KiB(1)); bios +=4)
        if (!strncmp("RSD PTR ", bios, 8))
            return bios;
    
    bios = (char *)(VMA2HI(BIOSROM));
    for (; bios < (char *)(VMA2HI(BIOSROM) + 0xfffff); bios +=4)
        if (!strncmp("RSD PTR ", bios, 8))
            return bios;
    return NULL;
}

acpiSDT_t *acpi_parse_rsdt(rsdt_t *rsdt, const char *sign) {
    int count = 0;
    acpiSDT_t *sdt = NULL;

    if (!rsdt || !sign)
        return NULL;

    count = (rsdt->hdr.length - sizeof(acpiSDT_t)) / 4;

    for (int i = 0; i < count; ++i) {
        sdt = (void *)VMA2HI(rsdt->sdt[i]);
        if (!strncmp(sign, sdt->signature, 4))
            return sdt;
    }

    return NULL;
}

acpiSDT_t *acpi_parse_xsdt(xsdt_t *xsdt, const char *sign) {
    int count = 0;
    acpiSDT_t *sdt = NULL;

    if (!xsdt || !sign)
        return NULL;

    count = (xsdt->hdr.length - sizeof (acpiSDT_t)) / 8;

    for (int i = 0; i < count; ++i) {
        sdt = (void *)VMA2HI(xsdt->sdt[i]);
        if (!strncmp(sign, sdt->signature, 4))
            return sdt;
    }

    return NULL;
}

acpiSDT_t *acpi_enumerate(const char *sign) {
    acpiSDT_t *sdt = NULL;
    sdt = XSDT ? acpi_parse_xsdt(XSDT, sign) :
        RSDT ? acpi_parse_rsdt(RSDT, sign) : NULL;
    if (sdt && acpi_validate_table((void *)sdt, sdt->length))
        return sdt;
    return NULL;
}

int acpi_init(void) {
    size_t size = 0;

    if (!(RSDP = acpi_findrsdp()))
        return -ENOENT;

    size = RSDP->rsdp.revno < 2 ? sizeof(rsdp_t) : RSDP->length;

    if (!(acpi_validate_table((void *)RSDP, size)))
        return -EINVAL;

    if (RSDP->rsdp.revno < 2)
        RSDT = (rsdt_t *)VMA2HI(RSDP->rsdp.rsdtaddr);
    else
        XSDT = (xsdt_t *)VMA2HI(RSDP->xsdtaddr);
    return 0;
}