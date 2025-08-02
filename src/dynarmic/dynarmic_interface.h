#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DynarmicCPUIface DynarmicCPUIface;

DynarmicCPUIface* dynarmic_create_instance();
void dynarmic_destroy_instance(DynarmicCPUIface* cpu);
unsigned long long dynarmic_run(DynarmicCPUIface* cpu, unsigned long long cycles);
void dynarmic_halt(DynarmicCPUIface* cpu);

unsigned long long dynarmic_get_x(DynarmicCPUIface* cpu, unsigned int reg_index);
void dynarmic_set_x(DynarmicCPUIface* cpu, unsigned int reg_index, unsigned long long value);
unsigned long long dynarmic_get_sp(DynarmicCPUIface* cpu);
void dynarmic_set_sp(DynarmicCPUIface* cpu, unsigned long long value);
unsigned long long dynarmic_get_pc(DynarmicCPUIface* cpu);
void dynarmic_set_pc(DynarmicCPUIface* cpu, unsigned long long value);

void dynarmic_write_u32(DynarmicCPUIface* cpu, unsigned long long vaddr, unsigned int value);

void dynarmic_mark_exclusive(DynarmicCPUIface* cpu, unsigned long long vaddr, unsigned int size);
void dynarmic_clear_exclusive(DynarmicCPUIface* cpu);

#ifdef __cplusplus
}  // extern "C"
#endif