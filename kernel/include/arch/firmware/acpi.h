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


typedef struct {
    acpiSDT_t   hdr;
    uint32_t    firmware_ctrl;
    uint32_t    dsdt;
    uint8_t     rsvd0;
    uint8_t     Preferred_PM_Profile;
    uint16_t    SCI_INT;
    uint32_t    SMI_CMD;
    uint8_t     ACPI_ENABLE;
    uint8_t     ACPI_DISABLE;
    uint8_t     S4BIOS_REQ;
    uint8_t     PSTATE_CNT;
    uint32_t    PM1a_EVT_BLK;
    uint32_t    PM1b_EVT_BLK;
    uint32_t    PM1a_CNT_BLK;
    uint32_t    PM1b_CNT_BLK;
    uint32_t    PM2_CNT_BLK;
    uint32_t    PM_TMR_BLK;
    uint32_t    GPE0_BLK;
    uint32_t    GPE1_BLK;
    uint8_t     PM1_EVT_LEN;
    uint8_t     PM1_CNT_LEN;
    uint8_t     PM2_CNT_LEN;
    uint8_t     PM_TMR_LEN;
    uint8_t     GPE0_BLK_LEN;
    uint8_t     GPE1_BLK_LEN;
    uint8_t     GPE1_BASE;
    uint8_t     CST_CNT;
    uint16_t    P_LVL2_LAT;
    uint16_t    P_LVL3_LAT;
    uint16_t    FLUSH_SIZE;
    uint16_t    FLUSH_STRIDE;
    uint8_t     DUTY_OFFSET;
    uint8_t     DUTY_WIDTH;
    uint8_t     RTC_DAY_ALRM;
    uint8_t     RTC_MON_ALRM;
    uint8_t     RTC_CENTURY;
    uint16_t    IAPC_BOOT_ARCH;
    uint8_t     Reserved1;
    uint32_t    Flags;
    uint32_t    RESET_REG[3];
    uint8_t     RESET_VALUE;
    uint8_t     Reserved2[3];
    uint64_t    X_FIRMWARE_CTRL;
    uint64_t    X_DSDT;
    uint32_t    X_PM1a_EVT_BLK[3];
    uint32_t    X_PM1b_EVT_BLK[3];
    uint32_t    X_PM1a_CNT_BLK[3];
    uint32_t    X_PM1b_CNT_BLK[3];
    uint32_t    X_PM2_CNT_BLK[3];
    uint32_t    X_PM_TMR_BLK[3];
    uint32_t    X_GPE0_BLK[3];
    uint32_t    X_GPE1_BLK[3];
} __packed acpiFADT_t;

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