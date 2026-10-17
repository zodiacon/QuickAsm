#include "pch.h"
#include "AsmEditor.h"
#include <WTLHelper.h>

// the lexer's word lists: instructions, FPU instructions, registers, directives, directive operands, extended instructions
static const char* KeyWords_ASM[] = {
	"aaa aad aam aas adc add and arpl blsr bnd bndcl bndcn bndcu bndmov bndstx bound bsf bsr bswap bt btc btr bts call cbw cdq cflush clc cld cli clts "
	"cmc cmova cmovae cmovb cmovbe cmovc cmove cmovg cmovge cmovl cmovle cmovna cmovnae cmovnb cmovnbe cmovnc "
	"cmovne cmovng cmovnge cmovnl cmovnle cmovno cmovnp cmovns cmovnz cmovo cmovp cmovpe cmovpo cmovs cmovz "
	"cmp cmps cmpsb cmpsd cmpsq cmpsw cmpxchg cmpxchg486 cmpxchg8b cpuid cwd cwde daa das dec div emms enter "
	"esc femms hlt ibts icebp idiv imul in inc ins insb insd insw int int01 int03 int1 int3 into invd invlpg "
	"iret iretd iretdf iretf iretw ja jae jb jbe jc jcxz je jecxz jg jge jl jle jmp jna jnae jnb jnbe jnc jne "
	"jng jnge jnl jnle jno jnp jns jnz jo jp jpe jpo js jz lahf lar lds lea leave les lfs lgdt lgs lidt lldt "
	"lmsw loadall loadall286 lock lods lodsb lodsd lodsq lodsw loop loopd loope looped loopew loopne loopned "
	"loopnew loopnz loopnzd loopnzw loopw loopz loopzd loopzw lsl lss ltr mov movs movsb movsd movsq movsw "
	"movsx movsxd movzx mul neg nop not or out outs outsb outsd outsw pop popa popad popaw popf popfd popfw "
	"push pusha pushad pushaw pushd pushf pushfd pushfw pushw rcl rcr rdmsr rdpmc rdshr rdtsc rep repe repne "
	"repnz repz ret retf retn rol ror rsdc rsldt rsm rsts sahf sal salc sar sbb scas scasb scasd scasq scasw "
	"seta setae setb setbe setc sete setg setge setl setle setna setnae setnb setnbe setnc setne setng setnge "
	"setnl setnle setno setnp setns setnz seto setp setpe setpo sets setz sgdt shl shld shr shrd sidt sldt smi "
	"smint smintold smsw stc std sti stos stosb stosd stosq stosw str sub svdc svldt svts syscall sysenter "
	"sysexit sysret test ud0 ud1 ud2 umov verr verw wait wbinvd wrmsr wrshr xadd xbts xchg xlat xlatb xor",
	"f2xm1 fabs fadd faddp fbld fbstp fchs fclex fcmovb fcmovbe fcmove fcmovnb fcmovnbe fcmovne fcmovnu fcmovu "
	"fcom fcomi fcomip fcomp fcompp fcos fdecstp fdisi fdiv fdivp fdivr fdivrp feni ffree ffreep fiadd ficom "
	"ficomp fidiv fidivr fild fimul fincstp finit fist fistp fisub fisubr fld fld1 fldcw fldenv fldenvd "
	"fldenvw fldl2e fldl2t fldlg2 fldln2 fldpi fldz fmul fmulp fnclex fndisi fneni fninit fnop fnsave fnsaved "
	"fnsavew fnstcw fnstenv fnstenvd fnstenvw fnstsw fpatan fprem fprem1 fptan frndint frstor frstord frstorw "
	"fsave fsaved fsavew fscale fsetpm fsin fsincos fsqrt fst fstcw fstenv fstenvd fstenvw fstp fstsw fsub "
	"fsubp fsubr fsubrp ftst fucom fucomp fucompp fwait fxam fxch fxtract fyl2x fyl2xp1",
	"ah al ax bh bl bp bx ch cl cr0 cr2 cr3 cr4 cs cx dh di dl dr0 dr1 dr2 dr3 dr6 dr7 ds dx eax ebp ebx ecx edi "
	"edx eip es esi esp fs gs mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7 r10 r10b r10d r10w r11 r11b r11d r11w r12 r12b "
	"r12d r12w r13 r13b r13d r13w r14 r14b r14d r14w r15 r15b r15d r15w r8 r8b r8d r8w r9 r9b r9d r9w rax rbp "
	"rbx rcx rdi rdx rip rsi rsp si sp ss st st0 st1 st2 st3 st4 st5 st6 st7 tr3 tr4 tr5 tr6 tr7 xmm0 xmm1 "
	"xmm10 xmm11 xmm12 xmm13 xmm14 xmm15 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7 xmm8 xmm9 ymm0 ymm1 ymm10 ymm11 ymm12 "
	"ymm13 ymm14 ymm15 ymm2 ymm3 ymm4 ymm5 ymm6 ymm7 ymm8 ymm9",
	"%arg %assign %define %elif %elifctk %elifdef %elifid %elifidn %elifidni %elifmacro %elifnctk %elifndef "
	"%elifnid %elifnidn %elifnidni %elifnmacro %elifnnum %elifnstr %elifnum %elifstr %else %endif %endmacro "
	"%endrep %error %exitrep %iassign %idefine %if %ifctk %ifdef %ifid %ifidn %ifidni %ifmacro %ifnctk %ifndef "
	"%ifnid %ifnidn %ifnidni %ifnmacro %ifnnum %ifnstr %ifnum %ifstr %imacro %include %line %local %macro %out "
	"%pop %push %rep %repl %rotate %stacksize %strlen %substr %undef %xdefine %xidefine .186 .286 .286c .286p "
	".287 .386 .386c .386p .387 .486 .486p .8086 .8087 .alpha .break .code .const .continue .cref .data .data? "
	".dosseg .else .elseif .endif .endw .err .err1 .err2 .errb .errdef .errdif .errdifi .erre .erridn .erridni "
	".errnb .errndef .errnz .exit .fardata .fardata? .if .lall .lfcond .list .listall .listif .listmacro "
	".listmacroall .model .msfloat .no87 .nocref .nolist .nolistif .nolistmacro .radix .repeat .sall .seq "
	".sfcond .stack .startup .tfcond .type .until .untilcxz .while .xall .xcref .xlist absolute alias align "
	"alignb assume at bits catstr comm comment common cpu db dd df dosseg dq dt dup dw echo else elseif "
	"elseif1 elseif2 elseifb elseifdef elseifdif elseifdifi elseife elseifidn elseifidni elseifnb elseifndef "
	"end endif endm endp ends endstruc eq equ default even exitm export extern externdef extrn for forc ge global goto "
	"group gt high highword iend if if1 if2 ifb ifdef ifdif ifdifi ife ifidn ifidni ifnb ifndef import incbin "
	"include includelib instr invoke irp irpc istruc label le length lengthof local low lowword lroffset lt "
	"macro mask mod name ne offset opattr option org page popcontext proc proto ptr public purge pushcontext "
	"record repeat rept resb resd resq rest resw section seg segment short size sizeof sizestr struc struct "
	"substr subtitle subttl textequ this times title type typedef union use16 use32 while width",
	"$ $$ %0 %1 %2 %3 %4 %5 %6 %7 %8 %9 .bss .data .text ? @b @f a16 a32 abs addr all assumes at basic byte c "
	"carry? casemap common compact cpu dotname dword emulator epilogue error export expr16 expr32 far far16 "
	"far32 farstack flat forceframe fortran fword huge language large listing ljmp loadds m510 medium memory "
	"near near16 near32 nearstack nodotname noemulator nokeyword noljmp nom510 none nonunique nooldmacros "
	"nooldstructs noreadonly noscoped nosignextend nosplit nothing notpublic o16 o32 oldmacros oldstructs "
	"os_dos overflow? para parity? pascal private prologue qword radix readonly real10 real4 real8 req sbyte "
	"scoped sdword seq setif2 sign? small smallstack stdcall sword syscall tbyte tiny use16 use32 uses vararg "
	"word wrt zero?",
	"addpd addps addsd addss andnpd andnps andpd andps blendpd blendps blendvpd blendvps cmpeqpd cmpeqps cmpeqsd "
	"cmpeqss cmplepd cmpleps cmplesd cmpless cmpltpd cmpltps cmpltsd cmpltss cmpnepd cmpneps cmpnesd cmpness "
	"cmpnlepd cmpnleps cmpnlesd cmpnless cmpnltpd cmpnltps cmpnltsd cmpnltss cmpordpd cmpordps cmpordsd "
	"cmpordss cmpunordpd cmpunordps cmpunordsd cmpunordss comisd comiss crc32 cvtdq2pd cvtdq2ps cvtpd2dq "
	"cvtpd2pi cvtpd2ps cvtpi2pd cvtpi2ps cvtps2dq cvtps2pd cvtps2pi cvtsd2si cvtsd2ss cvtsi2sd cvtsi2ss "
	"cvtss2sd cvtss2si cvttpd2dq cvttpd2pi cvttps2dq cvttps2pi cvttsd2si cvttss2si divpd divps divsd divss "
	"dppd dpps extractps fxrstor fxsave insertps ldmxscr lfence maskmovdq maskmovdqu maxpd maxps maxss mfence "
	"minpd minps minsd minss movapd movaps movd movdq2q movdqa movdqu movhlps movhpd movhps movlhps movlpd "
	"movlps movmskpd movmskps movntdq movntdqa movnti movntpd movntps movntq movq movq2dq movsd movss movupd "
	"movups mpsadbw mulpd mulps mulsd mulss orpd orps packssdw packsswb packusdw packuswb paddb paddd paddq "
	"paddsb paddsiw paddsw paddusb paddusw paddw pand pandn pause paveb pavgb pavgusb pavgw paxsd pblendvb "
	"pblendw pcmpeqb pcmpeqd pcmpeqq pcmpeqw pcmpestri pcmpestrm pcmpgtb pcmpgtd pcmpgtq pcmpgtw pcmpistri "
	"pcmpistrm pdistib pextrb pextrd pextrq pextrw pf2id pf2iw pfacc pfadd pfcmpeq pfcmpge pfcmpgt pfmax pfmin "
	"pfmul pfnacc pfpnacc pfrcp pfrcpit1 pfrcpit2 pfrsqit1 pfrsqrt pfsub pfsubr phminposuw pi2fd pinsrb pinsrd "
	"pinsrq pinsrw pmachriw pmaddwd pmagw pmaxsb pmaxsd pmaxsw pmaxub pmaxud pmaxuw pminsb pminsd pminsw "
	"pminub pminud pminuw pmovmskb pmovsxbd pmovsxbq pmovsxbw pmovsxdq pmovsxwd pmovsxwq pmovzxbd pmovzxbq "
	"pmovzxbw pmovzxdq pmovzxwd pmovzxwq pmuldq pmulhriw pmulhrwa pmulhrwc pmulhuw pmulhw pmulld pmullw "
	"pmuludq pmvgezb pmvlzb pmvnzb pmvzb popcnt por prefetch prefetchnta prefetcht0 prefetcht1 prefetcht2 "
	"prefetchw psadbw pshufd pshufhw pshuflw pshufw pslld pslldq psllq psllw psrad psraw psrld psrldq psrlq "
	"psrlw psubb psubd psubq psubsb psubsiw psubsw psubusb psubusw psubw pswapd ptest punpckhbw punpckhdq "
	"punpckhqdq punpckhwd punpcklbw punpckldq punpcklqdq punpcklwd pxor rcpps rcpss roundpd roundps roundsd "
	"roundss rsqrtps rsqrtss sfence shufpd shufps sqrtpd sqrtps sqrtsd sqrtss stmxcsr subpd subps subsd subss "
	"ucomisd ucomiss unpckhpd unpckhps unpcklpd unpcklps xorpd xorps",
};

namespace {
	// what a word to complete is made of (directives start with % or .)
	bool IsWordChar(char ch) {
		return ::isalnum((unsigned char)ch) || ch == '_' || ch == '.' || ch == '%' || ch == '@' || ch == '$' || ch == '?';
	}

	bool StartsWithNoCase(std::string_view text, std::string_view prefix) {
		return text.size() >= prefix.size() &&
			std::equal(prefix.begin(), prefix.end(), text.begin(), [](char a, char b) { return ::tolower((unsigned char)a) == ::tolower((unsigned char)b); });
	}

	bool LessNoCase(std::string const& a, std::string const& b) {
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
			[](char x, char y) { return ::tolower((unsigned char)x) < ::tolower((unsigned char)y); });
	}

	// the words of the lexer's lists that can be completed (not the directive operands, mostly MASM's)
	std::vector<std::string> const& CompletionWords() {
		static const std::vector<std::string> words = [] {
			std::vector<std::string> words{ "byte", "word", "dword", "qword", "tword", "rel" };
			for (int i : { 0, 1, 2, 3, 5 }) {
				std::string_view list(KeyWords_ASM[i]);
				while (!list.empty()) {
					auto end = list.find(' ');
					if (end > 0)
						words.emplace_back(list.substr(0, end));
					list.remove_prefix(end == std::string_view::npos ? list.size() : end + 1);
				}
			}
			std::ranges::sort(words);
			words.erase(std::unique(words.begin(), words.end()), words.end());
			return words;
			}();
		return words;
	}
}

HWND CAsmEditor::Create(HWND hParent, UINT id, bool readOnly) {
	auto hWnd = CScintillaCtrl::Create(hParent, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, id);
	if (!hWnd)
		return nullptr;

	SetCodePage(SC_CP_UTF8);
	SetTabWidth(4);
	// the horizontal scroll bar follows the longest line shown
	Execute(SCI_SETSCROLLWIDTH, 1);
	Execute(SCI_SETSCROLLWIDTHTRACKING, TRUE);
	SetMarginType(0, Scintilla::MarginType::Number);
	SetMarginType(1, Scintilla::MarginType::Symbol);
	SetMarginSensitive(1, true);
	SetMarginWidth(0, 40);
	SetMarginWidth(1, 20);
	MarkerDefine(BreakpointMarker, Scintilla::MarkerSymbol::Circle);
	MarkerDefine(CurrentLineMarker, Scintilla::MarkerSymbol::ShortArrow);
	MarkerDefine(DisabledBreakpointMarker, Scintilla::MarkerSymbol::Circle);
	MarkerDefine(ConditionalBreakpointMarker, Scintilla::MarkerSymbol::CirclePlus);
	MarkerDefine(DisabledConditionalBreakpointMarker, Scintilla::MarkerSymbol::CirclePlus);
	Execute(SCI_INDICSETSTYLE, CurrentLineIndicator, INDIC_FULLBOX);
	Execute(SCI_INDICSETSTYLE, ErrorIndicator, INDIC_SQUIGGLEPIXMAP);
	Execute(SCI_AUTOCSETIGNORECASE, TRUE);
	Execute(SCI_AUTOCSETCASEINSENSITIVEBEHAVIOUR, SC_CASEINSENSITIVEBEHAVIOUR_IGNORECASE);
	Execute(SCI_AUTOCSETORDER, SC_ORDER_PERFORMSORT);
	Execute(SCI_AUTOCSETMAXHEIGHT, 12);

	SetLexer(CreateLexer("asm"));
	for (int i = 0; i < _countof(KeyWords_ASM); i++)
		SetKeywords(i, KeyWords_ASM[i]);

	ApplyTheme();
	SetReadOnly(readOnly);
	return hWnd;
}

void CAsmEditor::ApplyTheme() {
	auto dark = WTLHelper::IsDarkMode();
	// Scintilla's own scroll bars
	::SetWindowTheme(m_hWnd, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);

	StyleResetDefault();
	StyleSetFont(STYLE_DEFAULT, m_FontFace.c_str());
	StyleSetSize(STYLE_DEFAULT, m_FontSize);
	StyleSetFore(STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
	StyleSetBack(STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
	StyleClearAll();

	StyleSetFore(STYLE_LINENUMBER, dark ? RGB(160, 160, 160) : RGB(96, 96, 96));
	StyleSetBack(STYLE_LINENUMBER, ::GetSysColor(COLOR_3DFACE));
	Execute(SCI_SETMARGINBACKN, 1, ::GetSysColor(COLOR_3DFACE));
	// (the margins grow with the font)
	SetMarginWidth(0, (int)Execute(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"_9999"));
	SetMarginWidth(1, std::max(20, (int)Execute(SCI_TEXTHEIGHT, 0)));

	StyleSetFore(SCE_ASM_DIRECTIVE, dark ? RGB(255, 165, 0) : RGB(165, 42, 42));
	StyleSetFore(SCE_ASM_NUMBER, dark ? RGB(0, 255, 255) : RGB(128, 0, 128));
	StyleSetBold(SCE_ASM_REGISTER, true);
	StyleSetFore(SCE_ASM_STRING, dark ? RGB(0xC0, 0x80, 0) : RGB(0x80, 0, 0));
	StyleSetFore(SCE_ASM_COMMENT, dark ? RGB(0, 192, 0) : RGB(0, 128, 0));
	StyleSetFore(SCE_ASM_CPUINSTRUCTION, dark ? RGB(255, 255, 0) : RGB(0, 0, 255));
	StyleSetBold(SCE_ASM_CPUINSTRUCTION, true);
	StyleSetFore(SCE_ASM_MATHINSTRUCTION, dark ? RGB(255, 255, 0) : RGB(0, 0, 255));
	StyleSetFore(SCE_ASM_EXTINSTRUCTION, dark ? RGB(255, 255, 0) : RGB(0, 0, 255));

	Execute(SCI_SETELEMENTCOLOUR, SC_ELEMENT_CARET, ::GetSysColor(COLOR_WINDOWTEXT) | 0xFF000000);
	Execute(SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK, (dark ? RGB(38, 79, 120) : RGB(173, 214, 255)) | 0xFF000000);
	// the autocompletion list
	Execute(SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST, ::GetSysColor(COLOR_WINDOWTEXT) | 0xFF000000);
	Execute(SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST_BACK, (dark ? RGB(37, 37, 38) : ::GetSysColor(COLOR_WINDOW)) | 0xFF000000);
	Execute(SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST_SELECTED, (dark ? RGB(255, 255, 255) : ::GetSysColor(COLOR_HIGHLIGHTTEXT)) | 0xFF000000);
	Execute(SCI_SETELEMENTCOLOUR, SC_ELEMENT_LIST_SELECTED_BACK, (dark ? RGB(38, 79, 120) : ::GetSysColor(COLOR_HIGHLIGHT)) | 0xFF000000);

	auto bpColor = dark ? RGB(220, 40, 40) : RGB(128, 0, 0);
	MarkerSetBack(BreakpointMarker, bpColor);
	MarkerSetFore(BreakpointMarker, bpColor);
	// a hollow circle: filled with the margin's color
	MarkerSetBack(DisabledBreakpointMarker, ::GetSysColor(COLOR_3DFACE));
	MarkerSetFore(DisabledBreakpointMarker, bpColor);
	// (a circle with a plus: filled with the fore color, its outline and plus in the back color)
	MarkerSetFore(ConditionalBreakpointMarker, bpColor);
	MarkerSetBack(ConditionalBreakpointMarker, ::GetSysColor(COLOR_3DFACE));
	MarkerSetFore(DisabledConditionalBreakpointMarker, ::GetSysColor(COLOR_3DFACE));
	MarkerSetBack(DisabledConditionalBreakpointMarker, bpColor);
	MarkerSetBack(CurrentLineMarker, dark ? RGB(255, 220, 0) : RGB(255, 200, 0));
	MarkerSetFore(CurrentLineMarker, dark ? RGB(160, 120, 0) : RGB(128, 96, 0));
	Execute(SCI_INDICSETFORE, CurrentLineIndicator, dark ? RGB(160, 160, 0) : RGB(255, 255, 0));
	Execute(SCI_INDICSETALPHA, CurrentLineIndicator, dark ? 80 : 100);
	Execute(SCI_INDICSETFORE, ErrorIndicator, dark ? RGB(255, 80, 80) : RGB(220, 0, 0));
}

void CAsmEditor::SetEditorFont(std::string const& face, int sizePoints) {
	m_FontFace = face;
	m_FontSize = sizePoints;
	ApplyTheme();
}

std::vector<std::string> CAsmEditor::GetLabels() const {
	std::vector<std::string> labels;
	auto text = GetAllText();
	std::string_view rest(text);
	while (!rest.empty()) {
		auto end = rest.find('\n');
		auto line = rest.substr(0, end);
		rest.remove_prefix(end == std::string_view::npos ? rest.size() : end + 1);

		size_t i = 0;
		while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
			i++;
		auto start = i;
		while (i < line.size() && IsWordChar(line[i]))
			i++;
		if (i > start && i < line.size() && line[i] == ':' && !::isdigit((unsigned char)line[start]))
			labels.emplace_back(line.substr(start, i - start));
	}
	return labels;
}

void CAsmEditor::AutoComplete(bool explicitRequest) {
	if (Execute(SCI_AUTOCACTIVE) && !explicitRequest)
		return;

	auto pos = GetCurrentPos();
	auto line = LineFromPosition(pos);
	auto lineStart = PositionFromLine(line);
	auto before = GetLine(line).substr(0, pos - lineStart);
	auto start = before.size();
	while (start > 0 && IsWordChar(before[start - 1]))
		start--;
	auto prefix = before.substr(start);
	if (!prefix.empty() && ::isdigit((unsigned char)prefix[0]))
		return;
	if (!explicitRequest && prefix.size() < 2)
		return;
	// (not in a comment, outside quotes)
	char quote = 0;
	for (size_t i = 0; i < start; i++) {
		auto ch = before[i];
		if (quote)
			quote = ch == quote ? 0 : quote;
		else if (ch == '\'' || ch == '"' || ch == '`')
			quote = ch;
		else if (ch == ';')
			return;
	}
	if (quote)
		return;

	std::vector<std::string> candidates;
	for (auto& word : CompletionWords())
		if (StartsWithNoCase(word, prefix))
			candidates.push_back(word);
	for (auto& label : GetLabels())
		if (StartsWithNoCase(label, prefix) && label != prefix)
			candidates.push_back(std::move(label));
	if (candidates.empty() || (!explicitRequest && candidates.size() == 1 && candidates[0].size() == prefix.size()))
		return;

	// typed in capitals: the words too (not the labels, which are as defined)
	bool upper = std::ranges::any_of(prefix, [](char ch) { return ::isupper((unsigned char)ch); }) &&
		!std::ranges::any_of(prefix, [](char ch) { return ::islower((unsigned char)ch); });
	std::ranges::sort(candidates, LessNoCase);
	candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
	std::string list;
	for (auto& word : candidates) {
		if (!list.empty())
			list += ' ';
		if (upper && std::ranges::binary_search(CompletionWords(), word))
			std::ranges::transform(word, word.begin(), [](char ch) { return (char)::toupper((unsigned char)ch); });
		list += word;
	}
	Execute(SCI_AUTOCSHOW, prefix.size(), (LPARAM)list.c_str());
}

void CAsmEditor::SetBreakpointMarker(int line, bool exists, bool enabled, bool conditional) {
	for (auto marker : { BreakpointMarker, DisabledBreakpointMarker, ConditionalBreakpointMarker, DisabledConditionalBreakpointMarker })
		MarkerDelete(line, marker);
	if (exists)
		MarkerAdd(line, conditional ? (enabled ? ConditionalBreakpointMarker : DisabledConditionalBreakpointMarker)
			: (enabled ? BreakpointMarker : DisabledBreakpointMarker));
}

void CAsmEditor::MarkErrorLines(std::vector<int> const& lines) {
	Execute(SCI_SETINDICATORCURRENT, ErrorIndicator);
	Execute(SCI_INDICATORCLEARRANGE, 0, Length());
	auto count = (int)Execute(SCI_GETLINECOUNT);
	for (auto line : lines) {
		if (line < 0 || line >= count)
			continue;
		// the text of the line (not its line break)
		auto start = PositionFromLine(line);
		auto end = (Sci_Position)Execute(SCI_GETLINEENDPOSITION, line);
		if (end > start)
			Execute(SCI_INDICATORFILLRANGE, start, end - start);
	}
}

void CAsmEditor::DeleteBreakpointMarkers() {
	for (auto marker : { BreakpointMarker, DisabledBreakpointMarker, ConditionalBreakpointMarker, DisabledConditionalBreakpointMarker })
		MarkerDeleteAll(marker);
}

void CAsmEditor::SetCurrentLine(int line) {
	Execute(SCI_SETINDICATORCURRENT, CurrentLineIndicator);
	if (m_CurrentLine >= 0) {
		MarkerDelete(m_CurrentLine, CurrentLineMarker);
		Execute(SCI_INDICATORCLEARRANGE, PositionFromLine(m_CurrentLine), GetLineLength(m_CurrentLine));
	}
	m_CurrentLine = line;
	if (line < 0)
		return;

	MarkerAdd(line, CurrentLineMarker);
	Execute(SCI_INDICATORFILLRANGE, PositionFromLine(line), GetLineLength(line));
	auto pos = PositionFromLine(line);
	Execute(SCI_SCROLLRANGE, pos, pos);
}

void CAsmEditor::SetSearchFlags(bool matchCase, bool wholeWord) {
	Execute(SCI_SETSEARCHFLAGS, (matchCase ? SCFIND_MATCHCASE : 0) | (wholeWord ? SCFIND_WHOLEWORD : 0));
}

Sci_Position CAsmEditor::SearchInRange(std::string const& text, Sci_Position from, Sci_Position to) {
	// (from > to: backwards)
	Execute(SCI_SETTARGETRANGE, from, to);
	return (Sci_Position)Execute(SCI_SEARCHINTARGET, text.size(), reinterpret_cast<LPARAM>(text.c_str()));
}

bool CAsmEditor::Find(std::string const& text, bool down, bool matchCase, bool wholeWord) {
	if (text.empty())
		return false;

	SetSearchFlags(matchCase, wholeWord);
	auto start = (Sci_Position)Execute(SCI_GETSELECTIONSTART);
	auto end = (Sci_Position)Execute(SCI_GETSELECTIONEND);
	auto length = Length();
	auto pos = down ? SearchInRange(text, end, length) : SearchInRange(text, start, 0);
	if (pos < 0)	// from the other end
		pos = down ? SearchInRange(text, 0, end) : SearchInRange(text, length, start);
	if (pos < 0)
		return false;

	Execute(SCI_SETSEL, pos, Execute(SCI_GETTARGETEND));
	return true;
}

bool CAsmEditor::Replace(std::string const& text, std::string const& replacement, bool matchCase, bool wholeWord) {
	if (text.empty() || IsReadOnly())
		return false;

	SetSearchFlags(matchCase, wholeWord);
	auto start = (Sci_Position)Execute(SCI_GETSELECTIONSTART);
	auto end = (Sci_Position)Execute(SCI_GETSELECTIONEND);
	// the selection is an occurrence (as Find selected it)
	if (start != end && SearchInRange(text, start, end) == start && (Sci_Position)Execute(SCI_GETTARGETEND) == end) {
		Execute(SCI_REPLACETARGET, replacement.size(), reinterpret_cast<LPARAM>(replacement.c_str()));
		Execute(SCI_SETSEL, start, start + (Sci_Position)replacement.size());
	}
	return Find(text, true, matchCase, wholeWord);
}

int CAsmEditor::ReplaceAll(std::string const& text, std::string const& replacement, bool matchCase, bool wholeWord) {
	if (text.empty() || IsReadOnly())
		return 0;

	SetSearchFlags(matchCase, wholeWord);
	int count = 0;
	Execute(SCI_BEGINUNDOACTION);
	for (Sci_Position pos = 0; ; count++) {
		if (SearchInRange(text, pos, Length()) < 0)
			break;
		Execute(SCI_REPLACETARGET, replacement.size(), reinterpret_cast<LPARAM>(replacement.c_str()));
		pos = (Sci_Position)Execute(SCI_GETTARGETEND);
	}
	Execute(SCI_ENDUNDOACTION);
	return count;
}
