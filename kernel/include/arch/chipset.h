#pragma once

int ioapic_init(void);
void ioapic_enable(int irq, int cpunum);

void pic_init(void);
void pit_init(void);