#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DynarmicCPUIface DynarmicCPUIface;

// Core functions
DynarmicCPUIface* dynarmic_create_instance(void* memory_backend_ptr);
void dynarmic_destroy_instance(DynarmicCPUIface* cpu);
unsigned long long dynarmic_run(DynarmicCPUIface* cpu);
unsigned long long dynarmic_step(DynarmicCPUIface* cpu);
void dynarmic_halt(DynarmicCPUIface* cpu);

// Register access
unsigned long long dynarmic_get_x(DynarmicCPUIface* cpu, unsigned int reg_index);
void dynarmic_set_x(DynarmicCPUIface* cpu, unsigned int reg_index, unsigned long long value);
unsigned long long dynarmic_get_sp(DynarmicCPUIface* cpu);
void dynarmic_set_sp(DynarmicCPUIface* cpu, unsigned long long value);
unsigned long long dynarmic_get_pc(DynarmicCPUIface* cpu);
void dynarmic_set_pc(DynarmicCPUIface* cpu, unsigned long long value);

// Memory access
void dynarmic_write_u32(DynarmicCPUIface* cpu, unsigned long long vaddr, unsigned int value);

#ifdef __cplusplus
}
#endif