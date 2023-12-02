#include "pch.h"
#include "CapstoneEngine.h"

CapstoneEngine::~CapstoneEngine() {
	if (m_CS)
		cs_close(&m_CS);
}

bool CapstoneEngine::Open(cs_mode mode) {
	csh h;
	m_Error = cs_open(CS_ARCH_X86, mode, &h);
	if (m_Error != CS_ERR_OK)
		return false;

	if (m_CS)
		cs_close(&m_CS);
	m_CS = h;
	return true;
}

std::vector<Instruction> CapstoneEngine::Disassemble(uint8_t const* code, size_t size, uint64_t address) {
	if (m_CS == 0)
		return {};

	cs_insn* insn;
	auto count = cs_disasm(m_CS, code, size, address, 0, &insn);
	if (count == 0) {
		m_Error = cs_errno(m_CS);
		return {};
	}

	std::vector<Instruction> inst;
	inst.reserve(count);
	for (size_t i = 0; i < count; i++) {
		auto& insni = insn[i];
		Instruction ins;
		ins.Id = insni.id;
		ins.Address = insni.address;
		ins.Bytes.resize(insni.size);
		memcpy(ins.Bytes.data(), insni.bytes, ins.Bytes.size());
		ins.Mnemonic = insni.mnemonic;
		ins.Operands = insni.op_str;
		inst.push_back(std::move(ins));
	}
	cs_free(insn, count);

	return inst;
}

cs_err CapstoneEngine::GetLastError() const {
	return m_Error;
}

const char* CapstoneEngine::GetError(cs_err error) {
	return cs_strerror(error);
}
