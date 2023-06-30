#pragma once

#include <lib/stdint.h>
#include <sys/system.h>
#include <lib/stddef.h>

/*header for a acpi v1.0*/
typedef struct
{
    char signature[8]; //signature
    uint8_t checksum;
    char oemid[6];
    uint8_t revno;     //revision number
    uint32_t rsdtaddr; //rsdt address
} __packed rsdp_t;

/*header for acpi v2.0*/
typedef struct
{
    rsdp_t rsdp;
    uint32_t length;
    uint64_t xsdtaddr;
    uint8_t ext_checksum;
    uint8_t revsd[3];
} __packed rsdp20_t;

typedef struct
{
    char signature[4];
    uint32_t length;
    uint8_t revno; //revision number
    uint8_t checksum;
    char oemid[6];
    char oemtable_id[8];
    uint32_t oemrevno;
    uint32_t creator_id;
    uint32_t creator_revno;
} __packed acpiSDT_t;

typedef struct
{
    acpiSDT_t hdr;
    uint32_t sdt[];
} __packed rsdt_t;

typedef struct {
    acpiSDT_t hdr;
    acpiSDT_t *sdt[];
} __packed xsdt_t;

typedef struct
{
    acpiSDT_t madt;
    uint32_t lapic_addr;
    uint32_t flags;
    uint8_t apics[];
} __packed acpiMADT_t;

#define ACPI_APIC   0
#define ACPI_IOAPIC 1

int acpi_mp(void);
void *acpi_findrsdp(void);
int acpi_parse_madt(acpiMADT_t *madt);
int acpi_validate_table(char *addr, size_t size);
acpiSDT_t *acpi_parse_rsdt(rsdt_t *rsdt, const char *sign);
acpiSDT_t *acpi_parse_xsdt(xsdt_t *xsdt, const char *sign);
acpiSDT_t *acpi_enumerate(const char *signature);

int acpi_init(void);