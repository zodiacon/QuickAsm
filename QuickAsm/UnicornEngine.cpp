#include "pch.h"
#include "UnicornEngine.h"

UnicornEngine::~UnicornEngine() {
    Close();
}

bool UnicornEngine::Open(CpuArch arch, CpuMode mode) {
    Close();

    m_LastError = uc_open((uc_arch)arch, (uc_mode)mode, &m_UC);
    m_Mode = mode;
    return m_LastError == UC_ERR_OK;
}

void UnicornEngine::Close() {
    if (m_UC) {
        uc_close(m_UC);
        m_UC = nullptr;
    }
}

bool UnicornEngine::IsOpen() const {
    return m_UC != nullptr;
}

uc_err UnicornEngine::LastError() const {
    return m_LastError;
}

CpuMode UnicornEngine::GetMode() const {
    return m_Mode;
}

bool UnicornEngine::WriteReg(x86Register reg, void* value) {
    m_LastError = uc_reg_write(m_UC, (int)reg, value);
    return m_LastError == UC_ERR_OK;
}

bool UnicornEngine::ReadReg(x86Register reg, void* value) const {
    m_LastError = uc_reg_read(m_UC, (int)reg, value);
    return m_LastError == UC_ERR_OK;
}

bool UnicornEngine::MapMemory(uint64_t address, uint64_t size, MemProtection protect) {
    m_LastError = uc_mem_map(m_UC, address, size, (uint32_t)protect);
    return m_LastError == UC_ERR_OK;
}

bool UnicornEngine::MapHostMemory(uint64_t address, uint64_t size, MemProtection protect, void* mem) {
    m_LastError = uc_mem_map_ptr(m_UC, address, size, (uint32_t)protect, mem);
    return m_LastError == UC_ERR_OK;
}

bool UnicornEngine::UnmapMemory(uint64_t address, uint64_t size) {
    m_LastError = uc_mem_unmap(m_UC, address, size);
    return m_LastError == UC_ERR_OK;
}

bool UnicornEngine::ProtectMemory(uint64_t address, uint64_t size, MemProtection protect) {
    m_LastError = uc_mem_protect(m_UC, address, size, (uint32_t)protect);
    return m_LastError == UC_ERR_OK;
}

bool UnicornEngine::WriteMemory(uint64_t address, const void* data, uint64_t size) {
    m_LastError = uc_mem_write(m_UC, address, data, size);
    assert(m_LastError == UC_ERR_OK);
    return m_LastError == UC_ERR_OK;
}

bool UnicornEngine::ReadMemory(uint64_t address, void* data, uint64_t size) {
    m_LastError = uc_mem_read(m_UC, address, data, size);
    return m_LastError == UC_ERR_OK;
}

void UnicornEngine::UnmapAllMemory() {
    uc_mem_region* regions;
    uint32_t count = 0;
    uc_mem_regions(m_UC, &regions, &count);
    for (uint32_t i = 0; i < count; i++)
        uc_mem_unmap(m_UC, regions[i].begin, regions[i].end - regions[i].begin);
    uc_free(regions);
}

bool UnicornEngine::Start(uint64_t address, uint64_t until, uint64_t timeout, size_t count) {
    m_LastError = uc_emu_start(m_UC, address, until, timeout, count);
    return m_LastError == UC_ERR_OK;
}

bool UnicornEngine::Stop() {
    m_LastError = uc_emu_stop(m_UC);
    return m_LastError == UC_ERR_OK;
}

UnicornEngine::CpuContext UnicornEngine::AllocContext() {
    return CpuContext(m_UC);
}

void UnicornEngine::EnableHooks(bool enable) {
    m_HooksEnabled = enable;
}

void* UnicornEngine::SaveContext(void* ctx) {
    if (ctx == nullptr)
        m_LastError = uc_context_alloc(m_UC, (uc_context**)&ctx);
    if (ctx == nullptr)
        return nullptr;

    m_LastError = uc_context_save(m_UC, (uc_context*)ctx);
    return ctx;
}

bool UnicornEngine::RestoreContext(void* ctx) {
    return uc_context_restore(m_UC, (uc_context*)ctx) == UC_ERR_OK;
}

const char* UnicornEngine::GetErrorText(uc_err error) const {
    return uc_strerror(error);
}

UnicornEngine::CpuContext::~CpuContext() {
    uc_context_free(m_ctx);
}

bool UnicornEngine::CpuContext::Save() {
    return uc_context_save(m_uc, m_ctx) == UC_ERR_OK;
}

bool UnicornEngine::CpuContext::Restore() {
    return uc_context_restore(m_uc, m_ctx) == UC_ERR_OK;
}

UnicornEngine::CpuContext::CpuContext(uc_engine* uc) : m_uc(uc) {
    uc_context_alloc(uc, &m_ctx);
}

void UnicornEngine::_CallbackHookCode(uc_engine* uc, uint64_t address, uint32_t size, void* ud) {
    auto hh = (HookHandle*)ud;
    (hh->m_CB)(address, size);
}

void* UnicornEngine::HookCode(HookType type, std::function<void(uint64_t, uint32_t)> cb, uint64_t begin, uint64_t end) {
    auto hh = new HookHandle;
    hh->m_CB = cb;
    m_LastError = uc_hook_add(m_UC, &hh->m_Hook, (int)type, &_CallbackHookCode, hh, begin, end);
    if (m_LastError == UC_ERR_OK)
        return hh;
    delete hh;
    return nullptr;
}

void UnicornEngine::Unhook(void* hook) {
    auto hh = (HookHandle*)hook;
    uc_hook_del(m_UC, hh->m_Hook);
    delete hh;
}

