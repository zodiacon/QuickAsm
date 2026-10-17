#include "pch.h"
#include "Expression.h"
#include "RegisterInfo.h"
#include <charconv>

namespace {
	struct FlagInfo {
		std::string_view Name;
		int Bit;
	};
	const FlagInfo Flags[] = {
		{ "cf", 0 }, { "pf", 2 }, { "af", 4 }, { "zf", 6 }, { "sf", 7 }, { "tf", 8 }, { "if", 9 }, { "df", 10 }, { "of", 11 },
	};

	bool IsIdentifierChar(char ch) {
		return ::isalnum((unsigned char)ch) || ch == '_';
	}

	std::string ToLower(std::string_view text) {
		std::string lower(text);
		for (auto& ch : lower)
			ch = (char)::tolower((unsigned char)ch);
		return lower;
	}
}

// recursive descent, C's precedence; the operations go to the output in postfix order
class Expression::Parser {
public:
	Parser(std::string_view text, std::vector<Op>& ops) : m_Text(text), m_Ops(ops) {}

	bool Parse(std::string& error) {
		if (!ParseBinary(0) || (!AtEnd() && Fail("Unexpected '" + std::string(1, m_Text[m_Pos]) + "'"))) {
			error = m_Error;
			return false;
		}
		return true;
	}

private:
	struct BinaryOp {
		std::string_view Token;
		OpCode Code;
		int Level;		// 0 binds loosest
	};
	// (longer tokens first: "<=" before "<"; a single "=" is taken as "==")
	static constexpr BinaryOp BinaryOps[] = {
		{ "||", OpCode::LogicalOr, 0 }, { "&&", OpCode::LogicalAnd, 1 },
		{ "==", OpCode::Equal, 5 }, { "!=", OpCode::NotEqual, 5 },
		{ "<<", OpCode::ShiftLeft, 7 }, { ">>", OpCode::ShiftRight, 7 },
		{ "<=", OpCode::LessEqual, 6 }, { ">=", OpCode::GreaterEqual, 6 },
		{ "|", OpCode::Or, 2 }, { "^", OpCode::Xor, 3 }, { "&", OpCode::And, 4 },
		{ "=", OpCode::Equal, 5 }, { "<", OpCode::Less, 6 }, { ">", OpCode::Greater, 6 },
		{ "+", OpCode::Add, 8 }, { "-", OpCode::Subtract, 8 },
		{ "*", OpCode::Multiply, 9 }, { "/", OpCode::Divide, 9 }, { "%", OpCode::Modulo, 9 },
	};
	static constexpr int UnaryLevel = 10;

	bool Fail(std::string error) {
		if (m_Error.empty())
			m_Error = std::move(error);
		return false;
	}

	void SkipSpaces() {
		while (m_Pos < m_Text.size() && ::isspace((unsigned char)m_Text[m_Pos]))
			m_Pos++;
	}

	bool AtEnd() {
		SkipSpaces();
		return m_Pos >= m_Text.size();
	}

	bool Accept(std::string_view token) {
		SkipSpaces();
		if (m_Text.substr(m_Pos, token.size()) != token)
			return false;
		m_Pos += token.size();
		return true;
	}

	BinaryOp const* PeekBinary(int minLevel) {
		SkipSpaces();
		for (auto& op : BinaryOps)
			if (m_Text.substr(m_Pos, op.Token.size()) == op.Token) {
				// ("|" isn't the start of "||" here: the longer tokens come first)
				return op.Level >= minLevel ? &op : nullptr;
			}
		return nullptr;
	}

	// operators of this level and tighter, left associative
	bool ParseBinary(int minLevel) {
		if (!ParseUnary())
			return false;
		while (auto op = PeekBinary(minLevel)) {
			m_Pos += op->Token.size();
			if (!ParseBinary(op->Level + 1))
				return false;
			m_Ops.push_back({ op->Code });
		}
		return true;
	}

	bool ParseUnary() {
		OpCode code;
		if (Accept("-"))
			code = OpCode::Negate;
		else if (Accept("~"))
			code = OpCode::Not;
		else if (Accept("!"))
			code = OpCode::LogicalNot;
		else
			return ParsePrimary();
		if (!ParseUnary())
			return false;
		m_Ops.push_back({ code });
		return true;
	}

	std::string_view NextIdentifier() {
		SkipSpaces();
		auto start = m_Pos;
		while (m_Pos < m_Text.size() && IsIdentifierChar(m_Text[m_Pos]))
			m_Pos++;
		return m_Text.substr(start, m_Pos - start);
	}

	bool ParseMemory(int size) {
		if (!Accept("["))
			return Fail("Expected '[' after the size");
		if (!ParseBinary(0))
			return false;
		if (!Accept("]"))
			return Fail("Expected ']'");
		m_Ops.push_back({ OpCode::Memory, (uint64_t)size });
		return true;
	}

	bool ParseNumber(std::string_view token) {
		auto lower = ToLower(token);
		int base = 10;
		std::string_view digits = lower;
		if (digits.starts_with("0x"))
			base = 16, digits.remove_prefix(2);
		else if (digits.ends_with('h'))
			base = 16, digits.remove_suffix(1);
		uint64_t value = 0;
		auto [end, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value, base);
		if (digits.empty() || ec != std::errc() || end != digits.data() + digits.size())
			return Fail("Invalid number '" + std::string(token) + "'");
		m_Ops.push_back({ OpCode::Number, value });
		return true;
	}

	bool ParsePrimary() {
		if (Accept("(")) {
			if (!ParseBinary(0))
				return false;
			return Accept(")") || Fail("Expected ')'");
		}
		SkipSpaces();
		if (m_Pos < m_Text.size() && m_Text[m_Pos] == '[')
			return ParseMemory(0);

		auto token = NextIdentifier();
		if (token.empty())
			return Fail(AtEnd() ? "Incomplete expression" : "Unexpected '" + std::string(1, m_Text[m_Pos]) + "'");
		if (::isdigit((unsigned char)token[0]))
			return ParseNumber(token);

		auto name = ToLower(token);
		static const std::pair<std::string_view, int> sizes[] = { { "byte", 1 }, { "word", 2 }, { "dword", 4 }, { "qword", 8 } };
		for (auto& [sizeName, size] : sizes)
			if (name == sizeName)
				return ParseMemory(size);
		for (auto& flag : Flags)
			if (name == flag.Name) {
				m_Ops.push_back({ OpCode::Flag, (uint64_t)flag.Bit });
				return true;
			}
		for (auto& ri : AllRegisters) {
			// (registers that fit in 64 bits)
			if ((ri.Category & (RegisterType::General | RegisterType::Segment | RegisterType::Special | RegisterType::Control)) == RegisterType::None)
				continue;
			if (ToLower(ri.Name) == name) {
				Op op{ OpCode::Register };
				op.Register = ri.Id;
				op.Mask = ri.Size >= 64 ? ~0ull : (1ull << ri.Size) - 1;
				m_Ops.push_back(op);
				return true;
			}
		}
		return Fail("Unknown name '" + std::string(token) + "'");
	}

	std::string_view m_Text;
	size_t m_Pos{ 0 };
	std::vector<Op>& m_Ops;
	std::string m_Error;
};

std::unique_ptr<Expression> Expression::Parse(std::string_view text, std::string& error) {
	auto expr = std::make_unique<Expression>();
	Parser parser(text, expr->m_Ops);
	if (!parser.Parse(error))
		return nullptr;
	return expr;
}

bool Expression::Evaluate(IExpressionContext& context, uint64_t& result) const {
	std::vector<uint64_t> stack;
	stack.reserve(m_Ops.size());
	for (auto& op : m_Ops) {
		uint64_t value;
		switch (op.Code) {
			case OpCode::Number:
				stack.push_back(op.Value);
				continue;
			case OpCode::Register:
				if (!context.ReadRegister(op.Register, value))
					return false;
				stack.push_back(value & op.Mask);
				continue;
			case OpCode::Flag:
				if (!context.ReadRegister(x86Register::EFLAGS, value))
					return false;
				stack.push_back((value >> op.Value) & 1);
				continue;
		}

		// unary
		auto& top = stack.back();
		switch (op.Code) {
			case OpCode::Memory:
				if (!context.ReadMemory(top, (int)op.Value, top))
					return false;
				continue;
			case OpCode::Negate: top = 0 - top; continue;
			case OpCode::Not: top = ~top; continue;
			case OpCode::LogicalNot: top = top == 0; continue;
		}

		// binary
		auto right = stack.back();
		stack.pop_back();
		auto& left = stack.back();
		switch (op.Code) {
			case OpCode::Multiply: left *= right; break;
			case OpCode::Divide:
			case OpCode::Modulo:
				if (right == 0)
					return false;
				left = op.Code == OpCode::Divide ? left / right : left % right;
				break;
			case OpCode::Add: left += right; break;
			case OpCode::Subtract: left -= right; break;
			case OpCode::ShiftLeft: left = right >= 64 ? 0 : left << right; break;
			case OpCode::ShiftRight: left = right >= 64 ? 0 : left >> right; break;
			case OpCode::Less: left = left < right; break;
			case OpCode::LessEqual: left = left <= right; break;
			case OpCode::Greater: left = left > right; break;
			case OpCode::GreaterEqual: left = left >= right; break;
			case OpCode::Equal: left = left == right; break;
			case OpCode::NotEqual: left = left != right; break;
			case OpCode::And: left &= right; break;
			case OpCode::Xor: left ^= right; break;
			case OpCode::Or: left |= right; break;
			case OpCode::LogicalAnd: left = left && right; break;
			case OpCode::LogicalOr: left = left || right; break;
		}
	}
	result = stack.back();
	return true;
}
