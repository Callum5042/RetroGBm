#include "CppUnitTest.h"
#include <RetroGBm/Disassembler.h>

#include <array>
#include <sstream>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
	TEST_CLASS(DisassemblerTests)
	{
		static void Check(std::initializer_list<uint8_t> bytes, uint16_t address,
			const char* text, int length)
		{
			const auto instruction = Disassembler::Decode(std::span(bytes.begin(), bytes.size()), address);
			Assert::AreEqual(std::string(text), instruction.text);
			Assert::AreEqual(length, static_cast<int>(instruction.length));
		}

	public:
		TEST_METHOD(Decode_BaseOpcodes_HaveExpectedLengths)
		{
			// Zero marks the eleven undefined SM83 opcodes, emitted as single bytes.
			constexpr int lengths[256] =
			{
				1,3,1,1,1,1,2,1,3,1,1,1,1,1,2,1,
				2,3,1,1,1,1,2,1,2,1,1,1,1,1,2,1,
				2,3,1,1,1,1,2,1,2,1,1,1,1,1,2,1,
				2,3,1,1,1,1,2,1,2,1,1,1,1,1,2,1,
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
				1,1,3,3,3,1,2,1,1,1,3,2,3,3,2,1,
				1,1,3,0,3,1,2,1,1,1,3,0,3,0,2,1,
				2,1,1,0,0,1,2,1,2,1,3,0,0,0,2,1,
				2,1,1,1,0,1,2,1,2,1,3,1,0,0,2,1
			};
			for (int opcode = 0; opcode < 256; ++opcode)
			{
				const std::array<uint8_t, 3> bytes = { static_cast<uint8_t>(opcode), 0x34, 0x12 };
				const auto instruction = Disassembler::Decode(bytes, 0x100);
				Assert::AreEqual(lengths[opcode] == 0 ? 1 : lengths[opcode], static_cast<int>(instruction.length));
				Assert::IsFalse(instruction.text.empty());
				Assert::IsTrue(instruction.text.find('{') == std::string::npos);
				Assert::AreEqual(lengths[opcode] == 0, instruction.text.starts_with("db "));
			}
		}

		TEST_METHOD(Decode_Operands_UseLittleEndianAndSm83Mnemonics)
		{
			Check({ 0x01, 0x34, 0x12 }, 0, "ld bc, $1234", 3);
			Check({ 0x08, 0xFE, 0xFF }, 0, "ld [$FFFE], sp", 3);
			Check({ 0x3E, 0x42 }, 0, "ld a, $42", 2);
			Check({ 0x76 }, 0, "halt", 1);
			Check({ 0xAE }, 0, "xor [hl]", 1);
			Check({ 0xC4, 0x50, 0x01 }, 0, "call nz, $0150", 3);
			Check({ 0xE0, 0x42 }, 0, "ldh [$FF42], a", 2);
			Check({ 0xF0, 0x80 }, 0, "ldh a, [$FF80]", 2);
			Check({ 0xF2 }, 0, "ldh a, [c]", 1);
			Check({ 0xFF }, 0, "rst $38", 1);
		}

		TEST_METHOD(Decode_RelativeBranches_ResolveSignedOffsetsAndWrap)
		{
			Check({ 0x18, 0xFE }, 0x150, "jr $0150", 2);
			Check({ 0x20, 0x7F }, 0x100, "jr nz, $0181", 2);
			Check({ 0x28, 0x80 }, 0x100, "jr z, $0082", 2);
			Check({ 0x30, 0xFC }, 0, "jr nc, $FFFE", 2);
			Check({ 0x38, 0x01 }, 0xFFFE, "jr c, $0001", 2);
		}

		TEST_METHOD(Decode_SignedStackOperands_IncludeCorrectSigns)
		{
			Check({ 0xE8, 0x80 }, 0, "add sp, -128", 2);
			Check({ 0xE8, 0x7F }, 0, "add sp, 127", 2);
			Check({ 0xF8, 0xFE }, 0, "ld hl, sp-2", 2);
			Check({ 0xF8, 0x02 }, 0, "ld hl, sp+2", 2);
			Check({ 0xF8, 0x00 }, 0, "ld hl, sp+0", 2);
		}

		TEST_METHOD(Decode_ExtendedInstructions_CoverRotationAndBitGroups)
		{
			Check({ 0xCB, 0x00 }, 0, "rlc b", 2);
			Check({ 0xCB, 0x0F }, 0, "rrc a", 2);
			Check({ 0xCB, 0x16 }, 0, "rl [hl]", 2);
			Check({ 0xCB, 0x19 }, 0, "rr c", 2);
			Check({ 0xCB, 0x22 }, 0, "sla d", 2);
			Check({ 0xCB, 0x2B }, 0, "sra e", 2);
			Check({ 0xCB, 0x34 }, 0, "swap h", 2);
			Check({ 0xCB, 0x3D }, 0, "srl l", 2);
			Check({ 0xCB, 0x46 }, 0, "bit 0, [hl]", 2);
			Check({ 0xCB, 0x7F }, 0, "bit 7, a", 2);
			Check({ 0xCB, 0x80 }, 0, "res 0, b", 2);
			Check({ 0xCB, 0xBE }, 0, "res 7, [hl]", 2);
			Check({ 0xCB, 0xC0 }, 0, "set 0, b", 2);
			Check({ 0xCB, 0xFF }, 0, "set 7, a", 2);
			for (int opcode = 0; opcode < 256; ++opcode)
			{
				const std::array<uint8_t, 2> bytes = { 0xCB, static_cast<uint8_t>(opcode) };
				const auto instruction = Disassembler::Decode(bytes, 0);
				Assert::AreEqual(2, static_cast<int>(instruction.length));
				Assert::IsFalse(instruction.text.starts_with("db "));
			}
		}

		TEST_METHOD(Decode_Stop_ConsumesPaddingByte)
		{
			Check({ 0x10, 0x00 }, 0, "stop $00", 2);
			Check({ 0x10, 0x42 }, 0, "stop $42", 2);
		}

		TEST_METHOD(Decode_IncompleteOrIllegalInput_PreservesBytes)
		{
			const auto empty = Disassembler::Decode({}, 0);
			Assert::AreEqual(0, static_cast<int>(empty.length));
			Check({ 0xD3 }, 0, "db $D3 ; illegal opcode", 1);
			Check({ 0x01 }, 0, "db $01 ; truncated instruction", 1);
			Check({ 0x01, 0x34 }, 0, "db $01 ; truncated instruction", 1);
			Check({ 0xCB }, 0, "db $CB ; truncated instruction", 1);
			Check({ 0x10 }, 0, "db $10 ; truncated instruction", 1);
		}

		TEST_METHOD(Export_Banks_DoNotDecodeAcrossPhysicalBoundaries)
		{
			std::vector<uint8_t> rom(0x8002, 0);
			rom[0x3FFF] = 0x01;
			rom[0x4000] = 0x3E;
			rom[0x4001] = 0x42;
			rom[0x7FFF] = 0xCB;
			rom[0x8000] = 0x18;
			rom[0x8001] = 0xFE;
			std::ostringstream output;
			Disassembler::Export(output, rom);
			const auto text = output.str();
			Assert::IsTrue(text.find("db $01 ; truncated instruction") != std::string::npos);
			Assert::IsTrue(text.find("db $CB ; truncated instruction") != std::string::npos);
			Assert::IsTrue(text.find("ld a, $42") != std::string::npos);
			Assert::IsTrue(text.find("001:4000  004000  3E 42") != std::string::npos);
			Assert::IsTrue(text.find("jr $4000") != std::string::npos);
			Assert::IsTrue(text.find("002:4000  008000  18 FE") != std::string::npos);
			Assert::IsTrue(text.find("SECTION \"ROM Bank 000\", ROM0[$0000]") != std::string::npos);
			Assert::IsTrue(text.find("SECTION \"ROM Bank 002\", ROMX[$4000], BANK[$002]") != std::string::npos);
		}

		TEST_METHOD(Export_WrappedRelativeBranch_UsesAssemblablePcExpression)
		{
			const std::array<uint8_t, 2> rom = { 0x30, 0xFC };
			std::ostringstream output;
			Disassembler::Export(output, rom);
			Assert::IsTrue(output.str().find("jr nc, @ -2 ; jr nc, $FFFE") != std::string::npos);
		}

		TEST_METHOD(Export_HeaderAndEntryPoint_KeepKnownBoundaries)
		{
			std::vector<uint8_t> rom(0x153, 0);
			rom[0xFF] = 0x01;
			rom[0x100] = 0xC3;
			rom[0x101] = 0x50;
			rom[0x102] = 0x01;
			rom[0x103] = 0x01;
			rom[0x104] = 0xCB;
			rom[0x14F] = 0x01;
			rom[0x150] = 0x31;
			rom[0x151] = 0xFE;
			rom[0x152] = 0xFF;
			std::ostringstream output;
			Disassembler::Export(output, rom);
			const auto text = output.str();
			Assert::IsTrue(text.find("jp $0150") != std::string::npos);
			Assert::IsTrue(text.find("000:0100  000100  C3 50 01") != std::string::npos);
			Assert::IsTrue(text.find("db $CB ") != std::string::npos);
			Assert::IsTrue(text.find("000:014F  00014F  01") != std::string::npos);
			Assert::IsTrue(text.find("ld sp, $FFFE") != std::string::npos);
		}

		TEST_METHOD(Export_EmptyOrTruncatedRom_Completes)
		{
			std::ostringstream output;
			Disassembler::Export(output, {});
			Assert::IsTrue(output.str().find("; Bank") == std::string::npos);
			const std::array<uint8_t, 2> rom = { 0xC3, 0x34 };
			output.str("");
			Disassembler::Export(output, rom);
			Assert::IsTrue(output.str().find("000:0001  000001  34") != std::string::npos);
		}
	};
}
