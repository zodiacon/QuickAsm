#pragma once

#include "UnicornEngine.h"

// what an expression reads: called on the emulator thread (while it waits in the code hook)
struct IExpressionContext {
	virtual bool ReadRegister(x86Register reg, uint64_t& value) = 0;
	// size: 1, 2, 4 or 8 bytes; 0 for the mode's size
	virtual bool ReadMemory(uint64_t address, int size, uint64_t& value) = 0;
};

// a breakpoint's condition, C-like: registers (rax, al...), flags (zf, cf...), numbers (10, 0x1F, 1Fh),
// memory ([rsi], byte [rsp+8]: a byte, word, dword or qword; the mode's size if not given),
// the arithmetic, bitwise, comparison (unsigned) and logical operators, and parentheses
class Expression {
public:
	// null if it isn't valid (the error says why)
	static std::unique_ptr<Expression> Parse(std::string_view text, std::string& error);

	// false if it can't be evaluated (memory that isn't there, a division by zero)
	bool Evaluate(IExpressionContext& context, uint64_t& result) const;

private:
	enum class OpCode {
		Number, Register, Flag, Memory,
		Negate, Not, LogicalNot,
		Multiply, Divide, Modulo, Add, Subtract, ShiftLeft, ShiftRight,
		Less, LessEqual, Greater, GreaterEqual, Equal, NotEqual,
		And, Xor, Or, LogicalAnd, LogicalOr,
	};
	struct Op {
		OpCode Code;
		uint64_t Value{ 0 };	// a number, a flag's bit, a memory size (bytes, 0: the mode's)
		x86Register Register{ x86Register::_INVALID };
		uint64_t Mask{ ~0ull };	// the register's size
	};

	class Parser;

	std::vector<Op> m_Ops;		// postfix
};
