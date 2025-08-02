#include "dynarmic_interface.h"

#include <cstring>
#include <memory>

#include <dynarmic/interface/A64/a64.h>
#include <dynarmic/interface/A64/config.h>

// Simple memory implementation for testing
class MemoryCallbacks : public Dynarmic::A64::UserCallbacks {
public:
    // 4KB test memory (0x1000 - 0x2000)
    uint8_t test_memory[4096] = {0};

    uint8_t MemoryRead8(uint64_t vaddr) override {
        if (vaddr >= 0x1000 && vaddr < 0x2000) {
            return test_memory[vaddr - 0x1000];
        }
        return 0;
    }

    uint16_t MemoryRead16(uint64_t vaddr) override {
        if (vaddr >= 0x1000 && vaddr < 0x2000) {
            uint16_t value;
            memcpy(&value, &test_memory[vaddr - 0x1000], sizeof(value));
            return value;
        }
        return 0;
    }

    uint32_t MemoryRead32(uint64_t vaddr) override {
        if (vaddr >= 0x1000 && vaddr < 0x2000) {
            uint32_t value;
            memcpy(&value, &test_memory[vaddr - 0x1000], sizeof(value));
            return value;
        }
        return 0;
    }

    uint64_t MemoryRead64(uint64_t vaddr) override {
        if (vaddr >= 0x1000 && vaddr < 0x2000) {
            uint64_t value;
            memcpy(&value, &test_memory[vaddr - 0x1000], sizeof(value));
            return value;
        }
        return 0;
    }

    // Implement missing 128-bit read
    Dynarmic::A64::Vector MemoryRead128(Dynarmic::A64::VAddr vaddr) override {
        // Default: zero vector
        return Dynarmic::A64::Vector{};
    }

    void MemoryWrite8(uint64_t vaddr, uint8_t value) override {
        if (vaddr >= 0x1000 && vaddr < 0x2000) {
            test_memory[vaddr - 0x1000] = value;
        }
    }

    void MemoryWrite16(uint64_t vaddr, uint16_t value) override {
        if (vaddr >= 0x1000 && vaddr < 0x2000) {
            memcpy(&test_memory[vaddr - 0x1000], &value, sizeof(value));
        }
    }

    void MemoryWrite32(uint64_t vaddr, uint32_t value) override {
        if (vaddr >= 0x1000 && vaddr < 0x2000) {
            memcpy(&test_memory[vaddr - 0x1000], &value, sizeof(value));
        }
    }

    void MemoryWrite64(uint64_t vaddr, uint64_t value) override {
        if (vaddr >= 0x1000 && vaddr < 0x2000) {
            memcpy(&test_memory[vaddr - 0x1000], &value, sizeof(value));
        }
    }

    // Implement missing 128-bit write
    void MemoryWrite128(Dynarmic::A64::VAddr vaddr, Dynarmic::A64::Vector value) override {
        // No-op or implement as needed
    }

    // Required implementations
    uint64_t GetTicksRemaining() override { return 1; }
    void AddTicks(uint64_t ticks) override {}
    void ExceptionRaised(uint64_t pc, Dynarmic::A64::Exception exception) override {}

    // Add missing overrides
    bool IsReadOnlyMemory(uint64_t vaddr) override { return false; }
    void InterpreterFallback(uint64_t pc, size_t num_instructions) override {}
    void CallSVC(uint32_t swi) override {}
    uint64_t GetCNTPCT() override { return 0; }
};

struct DynarmicCPUIface {
    std::unique_ptr<Dynarmic::A64::Jit> a64_jit;
    bool is_running;
    MemoryCallbacks callbacks;  // Our memory implementation

    DynarmicCPUIface()
            : is_running(false) {
        Dynarmic::A64::UserConfig config;
        config.callbacks = &callbacks;

        // Commented out deprecated flags (not present in newer Dynarmic versions)
        // config.enable_armv8_2_features = true;
        // config.enable_dot_product = true;
        // config.enable_fp16 = true;

        a64_jit = std::make_unique<Dynarmic::A64::Jit>(config);
    }
};

extern "C" {

DynarmicCPUIface* dynarmic_create_instance() {
    try {
        return new DynarmicCPUIface();
    } catch (...) {
        return nullptr;
    }
}

void dynarmic_destroy_instance(DynarmicCPUIface* cpu) {
    delete cpu;
}

unsigned long long dynarmic_run(DynarmicCPUIface* cpu, unsigned long long cycles) {
    if (!cpu || !cpu->a64_jit)
        return 0;

    cpu->is_running = true;
    cpu->a64_jit->Run();
    cpu->is_running = false;

    return cpu->a64_jit->GetPC();
}

// New function: Execute single instruction
unsigned long long dynarmic_step(DynarmicCPUIface* cpu) {
    if (!cpu || !cpu->a64_jit)
        return 0;

    cpu->is_running = true;
    cpu->a64_jit->Step();
    cpu->is_running = false;

    return cpu->a64_jit->GetPC();
}

void dynarmic_halt(DynarmicCPUIface* cpu) {
    if (cpu && cpu->a64_jit && cpu->is_running) {
        cpu->a64_jit->HaltExecution();
        cpu->is_running = false;
    }
}

// Register access for A64
unsigned long long dynarmic_get_x(DynarmicCPUIface* cpu, unsigned int reg_index) {
    if (!cpu || !cpu->a64_jit || reg_index > 30)
        return 0;
    return cpu->a64_jit->GetRegister(reg_index);
}

void dynarmic_set_x(DynarmicCPUIface* cpu, unsigned int reg_index, unsigned long long value) {
    if (!cpu || !cpu->a64_jit || reg_index > 30)
        return;
    cpu->a64_jit->SetRegister(reg_index, value);
}

unsigned long long dynarmic_get_sp(DynarmicCPUIface* cpu) {
    if (!cpu || !cpu->a64_jit)
        return 0;
    return cpu->a64_jit->GetSP();
}

void dynarmic_set_sp(DynarmicCPUIface* cpu, unsigned long long value) {
    if (!cpu || !cpu->a64_jit)
        return;
    cpu->a64_jit->SetSP(value);
}

unsigned long long dynarmic_get_pc(DynarmicCPUIface* cpu) {
    if (!cpu || !cpu->a64_jit)
        return 0;
    return cpu->a64_jit->GetPC();
}

void dynarmic_set_pc(DynarmicCPUIface* cpu, unsigned long long value) {
    if (!cpu || !cpu->a64_jit)
        return;
    cpu->a64_jit->SetPC(value);
}

void dynarmic_write_u32(DynarmicCPUIface* cpu, unsigned long long vaddr, unsigned int value) {
    if (!cpu || !cpu->a64_jit)
        return;
    cpu->callbacks.MemoryWrite32(vaddr, value);
}

// Stub implementations for exclusive operations
void dynarmic_mark_exclusive(DynarmicCPUIface* cpu, unsigned long long vaddr, unsigned int size) {
    (void)vaddr;
    (void)size;
}

void dynarmic_clear_exclusive(DynarmicCPUIface* cpu) {
    if (!cpu || !cpu->a64_jit)
        return;
    cpu->a64_jit->ClearExclusiveState();
}
}
