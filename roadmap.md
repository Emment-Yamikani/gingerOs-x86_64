# Features

## Firmware

- [x] BIOS
- [ ] UEFI
- [x] ACPI

## Device Drivers

### Buses

- [ ] IDE
- [ ] Serial ATA
- [ ] USB

### Physical Devices

- [x] CGA
- [x] Real Time Clock
- [x] High Precision Event Timer
- [ ] Console
- [ ] PS2 Mouse
- [ ] PS2 Keyboard
- [ ] CD ROM
- [ ] Central Processing unit

### Virtual Devices

- [x] RamDisk
- [x] Full
- [x] Null
- [x] Zero
- [x] Random
- [ ] Mem
- [ ] Pseudo Teminals

## Memory Management

- [x] Physical Memery Management
- [x] Virtual Memory Management
- [x] Page cache

### Memory allocator(s)
  
- [x] Zones for physical memory (Similar, but not same, as in Linux).
- [x] Liballoc (Credit to whom it is due)

## Multitasking

- [x] Symmetric Multiprocessing up to 16 Cores
- [x] Multi-Threading
  - [x] Thread groups
- [ ] User processes

- [ ] POSIX Signals

## Virtual Filesystem

- [x] Generic inode operations.
- [x] File address space to allow file sharing.
- [ ] File locks
- [ ] File times
- [ ] Generic file operations.

## File System

- [x] ginger ramfs
- [x] tmpfs
- [x] devfs
- [ ] pipefs
- [ ] sysfs
- [ ] ext2fs
- [ ] FAT32
