#include "RetroGBm/Pch.h"
#include "RetroGBm/Disassembler.h"

#include <algorithm>
#include <format>
#include <ostream>
#include <string_view>

namespace
{
	// SM83 opcodes in the same order as Cpu::Execute. Empty entries are illegal.
	// Operand markers determine the encoded length as well as the printed value.
	constexpr std::string_view Instructions[256] =
	{
		"nop", "ld bc, {d16}", "ld [bc], a", "inc bc", "inc b", "dec b", "ld b, {d8}", "rlca",
		"ld [{d16}], sp", "add hl, bc", "ld a, [bc]", "dec bc", "inc c", "dec c", "ld c, {d8}", "rrca",
		"stop {d8}", "ld de, {d16}", "ld [de], a", "inc de", "inc d", "dec d", "ld d, {d8}", "rla",
		"jr {r8}", "add hl, de", "ld a, [de]", "dec de", "inc e", "dec e", "ld e, {d8}", "rra",
		"jr nz, {r8}", "ld hl, {d16}", "ld [hli], a", "inc hl", "inc h", "dec h", "ld h, {d8}", "daa",
		"jr z, {r8}", "add hl, hl", "ld a, [hli]", "dec hl", "inc l", "dec l", "ld l, {d8}", "cpl",
		"jr nc, {r8}", "ld sp, {d16}", "ld [hld], a", "inc sp", "inc [hl]", "dec [hl]", "ld [hl], {d8}", "scf",
		"jr c, {r8}", "add hl, sp", "ld a, [hld]", "dec sp", "inc a", "dec a", "ld a, {d8}", "ccf",
		"ld b, b", "ld b, c", "ld b, d", "ld b, e", "ld b, h", "ld b, l", "ld b, [hl]", "ld b, a",
		"ld c, b", "ld c, c", "ld c, d", "ld c, e", "ld c, h", "ld c, l", "ld c, [hl]", "ld c, a",
		"ld d, b", "ld d, c", "ld d, d", "ld d, e", "ld d, h", "ld d, l", "ld d, [hl]", "ld d, a",
		"ld e, b", "ld e, c", "ld e, d", "ld e, e", "ld e, h", "ld e, l", "ld e, [hl]", "ld e, a",
		"ld h, b", "ld h, c", "ld h, d", "ld h, e", "ld h, h", "ld h, l", "ld h, [hl]", "ld h, a",
		"ld l, b", "ld l, c", "ld l, d", "ld l, e", "ld l, h", "ld l, l", "ld l, [hl]", "ld l, a",
		"ld [hl], b", "ld [hl], c", "ld [hl], d", "ld [hl], e", "ld [hl], h", "ld [hl], l", "halt", "ld [hl], a",
		"ld a, b", "ld a, c", "ld a, d", "ld a, e", "ld a, h", "ld a, l", "ld a, [hl]", "ld a, a",
		"add a, b", "add a, c", "add a, d", "add a, e", "add a, h", "add a, l", "add a, [hl]", "add a, a",
		"adc a, b", "adc a, c", "adc a, d", "adc a, e", "adc a, h", "adc a, l", "adc a, [hl]", "adc a, a",
		"sub b", "sub c", "sub d", "sub e", "sub h", "sub l", "sub [hl]", "sub a",
		"sbc a, b", "sbc a, c", "sbc a, d", "sbc a, e", "sbc a, h", "sbc a, l", "sbc a, [hl]", "sbc a, a",
		"and b", "and c", "and d", "and e", "and h", "and l", "and [hl]", "and a",
		"xor b", "xor c", "xor d", "xor e", "xor h", "xor l", "xor [hl]", "xor a",
		"or b", "or c", "or d", "or e", "or h", "or l", "or [hl]", "or a",
		"cp b", "cp c", "cp d", "cp e", "cp h", "cp l", "cp [hl]", "cp a",
		"ret nz", "pop bc", "jp nz, {d16}", "jp {d16}", "call nz, {d16}", "push bc", "add a, {d8}", "rst $00",
		"ret z", "ret", "jp z, {d16}", "", "call z, {d16}", "call {d16}", "adc a, {d8}", "rst $08",
		"ret nc", "pop de", "jp nc, {d16}", "", "call nc, {d16}", "push de", "sub {d8}", "rst $10",
		"ret c", "reti", "jp c, {d16}", "", "call c, {d16}", "", "sbc a, {d8}", "rst $18",
		"ldh [{a8}], a", "pop hl", "ldh [c], a", "", "", "push hl", "and {d8}", "rst $20",
		"add sp, {s8}", "jp hl", "ld [{d16}], a", "", "", "", "xor {d8}", "rst $28",
		"ldh a, [{a8}]", "pop af", "ldh a, [c]", "di", "", "push af", "or {d8}", "rst $30",
		"ld hl, sp{s8}", "ld sp, hl", "ld a, [{d16}]", "ei", "", "", "cp {d8}", "rst $38"
	};

	DisassembledInstruction DataByte(uint8_t value, std::string_view reason = {})
	{
		return { 1, std::format("db ${:02X}{}", value, reason) };
	}
}

DisassembledInstruction Disassembler::Decode(std::span<const uint8_t> bytes, uint16_t address)
{
	if (bytes.empty())
	{
		return {};
	}

	const uint8_t opcode = bytes[0];
	if (opcode == 0xCB)
	{
		if (bytes.size() < 2)
		{
			return DataByte(opcode, " ; truncated instruction");
		}

		constexpr std::string_view registers[] = { "b", "c", "d", "e", "h", "l", "[hl]", "a" };
		constexpr std::string_view rotations[] = { "rlc", "rrc", "rl", "rr", "sla", "sra", "swap", "srl" };
		constexpr std::string_view bitOperations[] = { "bit", "res", "set" };
		const uint8_t extended = bytes[1];
		const int group = extended >> 6;
		const int operation = (extended >> 3) & 7;
		const auto reg = registers[extended & 7];
		if (group == 0)
		{
			return { 2, std::format("{} {}", rotations[operation], reg) };
		}
		return { 2, std::format("{} {}, {}", bitOperations[group - 1], operation, reg) };
	}

	std::string text(Instructions[opcode]);
	if (text.empty())
	{
		return DataByte(opcode, " ; illegal opcode");
	}

	const auto operand = text.find('{');
	if (operand == std::string::npos)
	{
		return { 1, text };
	}

	const auto end = text.find('}', operand);
	const auto marker = text.substr(operand, end - operand + 1);
	const uint8_t length = marker == "{d16}" ? 3 : 2;
	if (bytes.size() < length)
	{
		return DataByte(opcode, " ; truncated instruction");
	}

	std::string value;
	if (marker == "{d16}")
	{
		value = std::format("${:04X}", bytes[1] | (bytes[2] << 8));
	}
	else if (marker == "{r8}")
	{
		const int displacement = bytes[1] < 0x80 ? bytes[1] : bytes[1] - 0x100;
		value = std::format("${:04X}", static_cast<uint16_t>(address + length + displacement));
	}
	else if (marker == "{s8}")
	{
		const int displacement = bytes[1] < 0x80 ? bytes[1] : bytes[1] - 0x100;
		value = opcode == 0xF8 ? std::format("{:+}", displacement) : std::to_string(displacement);
	}
	else if (marker == "{a8}")
	{
		value = std::format("${:04X}", 0xFF00 | bytes[1]);
	}
	else
	{
		value = std::format("${:02X}", bytes[1]);
	}

	text.replace(operand, marker.size(), value);
	return { length, text };
}

void Disassembler::Export(std::ostream& output, std::span<const uint8_t> rom)
{
	output << "; RetroGBm SM83 assembly listing\n"
		<< "; Assemble with rgbasm, then link with rgblink. Do not run rgbfix.\n"
		<< std::format("; Original ROM size: {} bytes.\n", rom.size())
		<< "; Linear disassembly: data outside the cartridge header is also decoded as code.\n"
		<< "; Comments: bank:address, ROM file offset, original bytes.\n";

	constexpr size_t BankSize = 0x4000;
	for (size_t bankOffset = 0; bankOffset < rom.size(); bankOffset += BankSize)
	{
		const size_t bank = bankOffset / BankSize;
		const size_t bankEnd = std::min(bankOffset + BankSize, rom.size());
		output << std::format("\n; Bank ${:03X}, ROM offset ${:06X}\n", bank, bankOffset);
		output << (bank == 0
			? "SECTION \"ROM Bank 000\", ROM0[$0000]\n"
			: std::format("SECTION \"ROM Bank {:03X}\", ROMX[$4000], BANK[${:03X}]\n", bank, bank));

		for (size_t offset = bankOffset; offset < bankEnd;)
		{
			const uint16_t address = static_cast<uint16_t>((bank == 0 ? 0 : BankSize) + offset - bankOffset);
			size_t end = bankEnd;

			// Keep the entry point and header aligned even if earlier data happened
			// to decode as an instruction spanning one of these known boundaries.
			if (bank == 0)
			{
				if (offset < 0x100) end = std::min(end, size_t{ 0x100 });
				else if (offset < 0x104) end = std::min(end, size_t{ 0x104 });
			}

			const auto instruction = bank == 0 && offset >= 0x104 && offset < 0x150
				? DataByte(rom[offset])
				: Decode(rom.subspan(offset, end - offset), address);

			std::string assembly = instruction.text;
			const uint8_t opcode = rom[offset];
			if (instruction.length == 2 && (opcode == 0x18 || opcode == 0x20 || opcode == 0x28 || opcode == 0x30 || opcode == 0x38))
			{
				const int displacement = rom[offset + 1] < 0x80 ? rom[offset + 1] : rom[offset + 1] - 0x100;
				if (address + 2 + displacement < 0 || address + 2 + displacement > 0xFFFF)
				{
					// A wrapped 16-bit address looks out of range to an assembler.
					// Use a PC-relative expression to preserve the encoded offset.
					assembly.replace(assembly.find('$'), 5, std::format("@ {:+}", 2 + displacement));
					assembly += " ; " + instruction.text;
				}
			}

			std::string raw;
			for (size_t i = 0; i < instruction.length; ++i)
			{
				if (i != 0) raw += ' ';
				raw += std::format("{:02X}", rom[offset + i]);
			}

			output << std::format("    {:<40} ; {:03X}:{:04X}  {:06X}  {}\n", assembly, bank, address, offset, raw);
			offset += instruction.length;
		}

		output << std::format("ASSERT @ == ${:04X}, \"ROM bank size changed\"\n", (bank == 0 ? 0 : BankSize) + bankEnd - bankOffset);
	}
}
