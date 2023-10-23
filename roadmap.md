# Features

## Firmware

- [x] BIOS
- [ ] UEFI
- [x] ACPI

- [x] Symmetric Multiprocessing up to 16 Cores

## Device Drivers

### Physical Devices

- [x] Real Time Clock
- [x] High Precision Event Timer
- [ ] Central Processing unit
- [x] CGA
- [ ] Console
- [ ] PS2 Keyboard
- [ ] PS2 Mouse
- [ ] Serail ATA
- [ ] IDE
- [ ] CD ROM

### Virtual Devices

- [ ] Null
- [ ] Zero
- [ ] Mem
- [ ] Random
- [ ] Pseudo Teminals
- [x] RamDisk

## Memory Management

- [x] Physical Memery Management
- [x] Virtual Memory Management

### Memory allocator(s)
  
- [x] Zones for physical memory (Similar, but not same, as in Linux).
- [x] Liballoc (Credit to whom it is due)

## Multitasking

- [x] Multi-Threading
  - [x] Thread groups
- [ ] User processes

- [ ] POSIX Signals

## Virtual Filesystem

- [x] Generic inode operations.
- [ ] Generic file operations.
- [ ] File address space to alloc file sharing.
- [ ] File locks
- [ ] File times

## File System

- [x] ginger ramfs
- [x] tmpfs
- [ ] devfs
- [ ] pipefs
- [ ] sysfs
- [ ] ext2fs
- [ ] FAT32
