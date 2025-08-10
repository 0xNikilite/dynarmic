#include "dynarmic_interface.h"
#include <dynarmic/interface/A64/a64.h>
#include <dynarmic/interface/A64/config.h>
#include <cstdint>
#include <cstring>

// Include Rust memory FFI
extern "C" {
    uint8_t oboromi_memory_read_u8(void* mem_ptr, uint64_t addr);
    void    oboromi_memory_write_u8(void* mem_ptr, uint64_t addr, uint8_t value);
    uint16_t oboromi_memory_read_u16(void* mem_ptr, uint64_t addr);
    void     oboromi_memory_write_u16(void* mem_ptr, uint64_t addr, uint16_t value);
    uint32_t oboromi_memory_read_u32(void* mem_ptr, uint64_t addr);
    void     oboromi_memory_write_u32(void* mem_ptr, uint64_t addr, uint32_t value);
    uint64_t oboromi_memory_read_u64(void* mem_ptr, uint64_t addr);
    void     oboromi_memory_write_u64(void* mem_ptr, uint64_t addr, uint64_t value);
}

class MemoryCallbacks : public Dynarmic::A64::UserCallbacks {
public:
    // Pointer to Rust Memory backend
    void* rust_mem;

    explicit MemoryCallbacks(void* mem_ptr) : rust_mem(mem_ptr) {}

    // All memory accesses are delegated to Rust backend
    uint8_t MemoryRead8(uint64_t vaddr) override {
        return oboromi_memory_read_u8(rust_mem, vaddr);
    }

    uint16_t MemoryRead16(uint64_t vaddr) override {
        return oboromi_memory_read_u16(rust_mem, vaddr);
    }

    uint32_t MemoryRead32(uint64_t vaddr) override {
        return oboromi_memory_read_u32(rust_mem, vaddr);
    }

    uint64_t MemoryRead64(uint64_t vaddr) override {
        return oboromi_memory_read_u64(rust_mem, vaddr);
    }

    Dynarmic::A64::Vector MemoryRead128(Dynarmic::A64::VAddr) override {
        // If 128-bit loads are needed, split into two 64-bit loads
        Dynarmic::A64::Vector vec{};
        uint64_t lo = oboromi_memory_read_u64(rust_mem, 0);
        uint64_t hi = oboromi_memory_read_u64(rust_mem, 8);
        std::memcpy(&vec, &lo, sizeof(lo));
        std::memcpy(reinterpret_cast<uint8_t*>(&vec) + 8, &hi, sizeof(hi));
        return vec;
    }

    void MemoryWrite8(uint64_t vaddr, uint8_t value) override {
        oboromi_memory_write_u8(rust_mem, vaddr, value);
    }

    void MemoryWrite16(uint64_t vaddr, uint16_t value) override {
        oboromi_memory_write_u16(rust_mem, vaddr, value);
    }

    void MemoryWrite32(uint64_t vaddr, uint32_t value) override {
        oboromi_memory_write_u32(rust_mem, vaddr, value);
    }

    void MemoryWrite64(uint64_t vaddr, uint64_t value) override {
        oboromi_memory_write_u64(rust_mem, vaddr, value);
    }

    void MemoryWrite128(Dynarmic::A64::VAddr vaddr, Dynarmic::A64::Vector value) override {
        uint64_t lo, hi;
        std::memcpy(&lo, &value, sizeof(lo));
        std::memcpy(&hi, reinterpret_cast<const uint8_t*>(&value) + 8, sizeof(hi));
        oboromi_memory_write_u64(rust_mem, vaddr, lo);
        oboromi_memory_write_u64(rust_mem, vaddr + 8, hi);
    }

    // Other required overrides
    uint64_t GetTicksRemaining() override { return 1; }
    void AddTicks(uint64_t) override {}
    void ExceptionRaised(uint64_t, Dynarmic::A64::Exception) override {}
    bool IsReadOnlyMemory(uint64_t) override { return false; }
    void InterpreterFallback(uint64_t, size_t) override {}
    void CallSVC(uint32_t) override {}
    uint64_t GetCNTPCT() override { return 0; }
};

struct DynarmicCPUIface {
    std::unique_ptr<Dynarmic::A64::Jit> a64_jit;
    bool is_running;
    MemoryCallbacks callbacks;

    DynarmicCPUIface(void* mem_ptr)
        : is_running(false), callbacks(mem_ptr) {
        Dynarmic::A64::UserConfig config;
        config.callbacks = &callbacks;
        a64_jit = std::make_unique<Dynarmic::A64::Jit>(config);
    }
};

extern "C" {

DynarmicCPUIface* dynarmic_create_instance(void* memory_backend_ptr) {
    try {
        return new DynarmicCPUIface(memory_backend_ptr);
    } catch (...) {
        return nullptr;
    }
}

void dynarmic_destroy_instance(DynarmicCPUIface* cpu) {
    delete cpu;
}

unsigned long long dynarmic_run(DynarmicCPUIface* cpu) {
    if (!cpu || !cpu->a64_jit)
        return 0;
    cpu->is_running = true;
    cpu->a64_jit->Run();
    cpu->is_running = false;
    return cpu->a64_jit->GetPC();
}

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
    if (!cpu)
        return;
    oboromi_memory_write_u32(cpu->callbacks.rust_mem, vaddr, value);
}

}
