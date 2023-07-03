#ifndef BYTECODEGENERATOR_INSTRUCTION_H
#define BYTECODEGENERATOR_INSTRUCTION_H

#include <string>
#include <cstdint>
#include <map>
#include <utility>

namespace Bytecode
{

enum InstructionType {
	NOP         = 0x00,
	CALL        = 0x0A, // Call compiled function
	INIT        = 0x0C, // Initialize variable
	SETARR      = 0x0D, // Set array bounds
	STORE       = 0x0E, // Store value in variable
	STOREARR    = 0x0F, // Store value in array
	LOAD        = 0x10, // Get value from variable
	LOADARR     = 0x11, // Get value froma array
	SETMON      = 0x12, // Set variable monitor
	UNSETMON    = 0x13, // Unset variable monitor
	JUMP        = 0x14, // Unconditional jump
	JNZ         = 0x15, // Conditional jump if non-zero value in specified register
	JZ          = 0x16, // Conditional jump if zero value in specified register
	POP         = 0x18, // Pop from stack to register
	PUSH        = 0x19, // Push to stack from register
	RET         = 0x1B, // Return from function
	PAUSE       = 0x1D, // Force pause
	ERRORR       = 0x1E, // Abort evaluation
	LINE        = 0x1F, // Emit line number
	REF         = 0x20, // Get reference to variable
	REFARR      = 0x21, // Get reference to array element
	SHOWREG     = 0x22, // Show register value at margin
	CLEARMARG   = 0x23, // Clear margin text from current line to specified
	SETREF      = 0x24, // Set reference value to variable
	HALT        = 0x26, // Terminate
	CTL         = 0x27, // Control VM behaviour
	INRANGE     = 0x28, // Pops 4 values ... a, b, c, x from stack and returns c>=0? a<x<=b : a<=x<b
	UPDARR      = 0x29, // Updates array bounds

	CSTORE      = 0x30, // Copy value from stack head and push it to cache
	CLOAD       = 0x31, // Pop value from cache to push it to main stack
	CDROPZ      = 0x32, // Drop cache value in case of zero value in specified register

	CACHEBEGIN  = 0x33, // Push begin marker into cache
	CACHEEND    = 0x34, // Clear cache until marker


	// Common operations -- no comments need

	SUM         = 0xF1,
	SUB         = 0xF2,
	MUL         = 0xF3,
	DIV         = 0xF4,
	POW         = 0xF5,
	NEG         = 0xF6,
	AND         = 0xF7,
	OR          = 0xF8,
	EQ          = 0xF9,
	NEQ         = 0xFA,
	LS          = 0xFB,
	GT          = 0xFC,
	LEQ         = 0xFD,
	GEQ         = 0xFE
};

enum VariableScope {
	UNDEF  = 0x00, // Undefined if not need
	CONSTT = 0x01, // Value from constants table
	LOCAL  = 0x02, // Value from locals table
	GLOBAL = 0x03  // Value from globals table
};

enum LineSpecification {
	LINE_NUMBER = 0x00,
	COLUMN_START_AND_END = 0x80
};

/* Instruction has optimal (aka serialized) size of 32 bit:
  - first byte is Instruction Type
  - second byte is Module Number (for CALL),
    register number (for POP, PUSH, JZ and JNZ)
    variable scope (for variable-access instructions) or
    undefined (e.g. value is 0x00)
  - last two bytes is instruction argument value
    (or 0x0000 if not defined)
*/

struct Instruction {
	InstructionType type;
	union {
		LineSpecification lineSpec;
		VariableScope scope;
		uint8_t module;
		uint8_t registerr;
	};
	uint16_t arg;
};

typedef std::pair<uint32_t, uint16_t> AS_Key;
// module | algorithm, id
typedef std::map<AS_Key, std::string> AS_Values;
struct AS_Helpers {
	AS_Values globals;
	AS_Values locals;
	AS_Values constants;
	AS_Values algorithms;
};

inline bool extractColumnPositionsFromLineInstruction(
	const Instruction &instr,
	uint32_t &from, uint32_t &to
) {
	if (instr.type == LINE && (instr.scope & COLUMN_START_AND_END) != 0) {
		uint32_t value = ((instr.lineSpec & 0x3F) << 16) | instr.arg;
		from = value >> 11;
		to = value & 0x7FF;
		return true;
	} else {
		from = to = 0u;
		return false;
	}
}

inline Instruction &setColumnPositionsToLineInstruction(
	Instruction &instr,
	uint32_t from, uint32_t to
) {
	uint32_t value = (from << 11) | to;
	uint16_t arg = value & 0xFFFF;
	uint8_t scope = (value >> 16) & 0xFF;
	scope = scope | COLUMN_START_AND_END;
	instr.type = LINE;
	instr.lineSpec = LineSpecification(scope);
	instr.arg = arg;
	return instr;
}

std::string instructionToString(
	const Instruction &instr,
	const AS_Helpers &helpers,
	uint8_t moduleId, uint16_t algId
);

uint32_t toUint32(const Instruction &instr);
Instruction fromUint32(uint32_t value);



} // end namespace Bytecode

#endif // BYTECODEGENERATOR_INSTRUCTION_H
