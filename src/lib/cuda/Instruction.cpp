#include "Instruction.hpp"

#include <string>

namespace CudaParse {

// include sm_50-sm_70 instructions
std::map<std::string, InstructionType> Instruction::opcode_types = {
  // sm_60 
  { "FCMP", INS_TYPE_FLOAT },
  { "RRO", INS_TYPE_FLOAT },
  { "DMNMX", INS_TYPE_FLOAT },
  { "DSET", INS_TYPE_FLOAT },
  // sm_70
  { "FADD", INS_TYPE_FLOAT },
  { "FADD32I", INS_TYPE_FLOAT },
  { "FCHK", INS_TYPE_FLOAT },
  { "FFMA32I", INS_TYPE_FLOAT },
  { "FFMA", INS_TYPE_FLOAT },
  { "FMNMX", INS_TYPE_FLOAT },
  { "FMUL", INS_TYPE_FLOAT },
  { "FMUL32I", INS_TYPE_FLOAT },
  { "FSEL", INS_TYPE_FLOAT },
  { "FSET", INS_TYPE_FLOAT },
  { "FSETP", INS_TYPE_FLOAT },
  { "FSWZADD", INS_TYPE_FLOAT },
  { "MUFU", INS_TYPE_FLOAT },
  { "HADD2", INS_TYPE_FLOAT },
  { "HADD2_32I", INS_TYPE_FLOAT },
  { "HFMA2", INS_TYPE_FLOAT },
  { "HFMA2_32I", INS_TYPE_FLOAT },
  { "HMMA", INS_TYPE_FLOAT },
  { "HMUL2", INS_TYPE_FLOAT },
  { "HMUL2_32I", INS_TYPE_FLOAT },
  { "HSET2", INS_TYPE_FLOAT },
  { "HSETP2", INS_TYPE_FLOAT },
  { "DADD", INS_TYPE_FLOAT },
  { "DFMA", INS_TYPE_FLOAT },
  { "DMUL", INS_TYPE_FLOAT },
  { "DSETP", INS_TYPE_FLOAT },
  // sm_60
  { "BFE", INS_TYPE_INTEGER },
  { "BFI", INS_TYPE_INTEGER },
  { "ICMP", INS_TYPE_INTEGER },
  { "IMADSP", INS_TYPE_INTEGER },
  { "ISET", INS_TYPE_INTEGER },
  { "XMAD", INS_TYPE_INTEGER },
  // sm_70
  { "BMSK", INS_TYPE_INTEGER },
  { "BREV", INS_TYPE_INTEGER },
  { "FLO", INS_TYPE_INTEGER },
  { "IABS", INS_TYPE_INTEGER },
  { "IADD", INS_TYPE_INTEGER },
  { "IADD3", INS_TYPE_INTEGER },
  { "IADD32I", INS_TYPE_INTEGER },
  { "IDP", INS_TYPE_INTEGER },
  { "IDP4A", INS_TYPE_INTEGER },
  { "IMAD", INS_TYPE_INTEGER },
  { "IMMA", INS_TYPE_INTEGER },
  { "IMNMX", INS_TYPE_INTEGER },
  { "IMUL", INS_TYPE_INTEGER },
  { "IMUL32I", INS_TYPE_INTEGER },
  { "ISCADD", INS_TYPE_INTEGER },
  { "ISCADD32I", INS_TYPE_INTEGER },
  { "ISETP", INS_TYPE_INTEGER },
  { "LEA", INS_TYPE_INTEGER },
  { "LOP", INS_TYPE_INTEGER },
  { "LOP3", INS_TYPE_INTEGER },
  { "LOP32I", INS_TYPE_INTEGER },
  { "POPC", INS_TYPE_INTEGER },
  { "SHF", INS_TYPE_INTEGER },
  { "SHL", INS_TYPE_INTEGER },
  { "SHR", INS_TYPE_INTEGER },
  { "VABSDIFF", INS_TYPE_INTEGER },
  { "VABSDIFF4", INS_TYPE_INTEGER },
  // sm_70
  { "LD", INS_TYPE_MEMORY },
  { "LDC", INS_TYPE_MEMORY },
  { "LDG", INS_TYPE_MEMORY },
  { "LDL", INS_TYPE_MEMORY },
  { "LDS", INS_TYPE_MEMORY },
  { "ST", INS_TYPE_MEMORY },
  { "STG", INS_TYPE_MEMORY },
  { "STL", INS_TYPE_MEMORY },
  { "STS", INS_TYPE_MEMORY },
  { "MATCH", INS_TYPE_MEMORY },
  { "QSPC", INS_TYPE_MEMORY },
  { "ATOM", INS_TYPE_MEMORY },
  { "ATOMS", INS_TYPE_MEMORY },
  { "ATOMG", INS_TYPE_MEMORY },
  { "RED", INS_TYPE_MEMORY },
  { "CCTL", INS_TYPE_MEMORY },
  { "CCTLL", INS_TYPE_MEMORY },
  { "CCTLT", INS_TYPE_MEMORY },
  // sm_60
  { "TEXS", INS_TYPE_TEXTRUE },
  { "TLD4S", INS_TYPE_TEXTRUE },
  { "TLDS", INS_TYPE_TEXTRUE },
  // sm_70
  { "TEX", INS_TYPE_TEXTRUE },
  { "TLD", INS_TYPE_TEXTRUE },
  { "TLD4", INS_TYPE_TEXTRUE },
  { "TMML", INS_TYPE_TEXTRUE },
  { "TXD", INS_TYPE_TEXTRUE },
  { "TXQ", INS_TYPE_TEXTRUE },
  // sm_60
  { "SSY", INS_TYPE_CONTROL },
  { "SYNC", INS_TYPE_CONTROL },
  { "CAL", INS_TYPE_CONTROL },
  { "JCAL", INS_TYPE_CONTROL },
  { "PRET", INS_TYPE_CONTROL },
  { "BRK", INS_TYPE_CONTROL },
  { "PBK", INS_TYPE_CONTROL },
  { "CONT", INS_TYPE_CONTROL },
  { "PCNT", INS_TYPE_CONTROL },
  { "PEXIT", INS_TYPE_CONTROL },
  // sm_70
  { "BMOV", INS_TYPE_CONTROL },
  { "BPT", INS_TYPE_CONTROL },
  { "BRA", INS_TYPE_CONTROL },
  { "BREAK", INS_TYPE_CONTROL },
  { "BRX", INS_TYPE_CONTROL },
  { "BSSY", INS_TYPE_CONTROL },
  { "BSYNC", INS_TYPE_CONTROL },
  { "CALL", INS_TYPE_CONTROL },
  { "EXIT", INS_TYPE_CONTROL },
  { "JMP", INS_TYPE_CONTROL },
  { "JMX", INS_TYPE_CONTROL },
  { "KILL", INS_TYPE_CONTROL },
  { "NANOSLEEP", INS_TYPE_CONTROL },
  { "RET", INS_TYPE_CONTROL },
  { "RPCMOV", INS_TYPE_CONTROL },
  { "RTT", INS_TYPE_CONTROL },
  { "WARPSYNC", INS_TYPE_CONTROL },
  { "YIELD", INS_TYPE_CONTROL },
  { "BAR", INS_TYPE_CONTROL },
  { "DEPBAR", INS_TYPE_CONTROL },
  { "ERRBAR", INS_TYPE_CONTROL },
  { "MEMBAR", INS_TYPE_CONTROL },
  // sm_60
  { "CSET", INS_TYPE_MISC },
  { "CSETP", INS_TYPE_MISC },
  { "PSET", INS_TYPE_MISC },
  // sm_70
  { "B2R", INS_TYPE_MISC },
  { "CS2R", INS_TYPE_MISC },
  { "CSMTEST", INS_TYPE_MISC },
  { "GETLMEMBASE", INS_TYPE_MISC },
  { "LEPC", INS_TYPE_MISC },
  { "NOP", INS_TYPE_MISC },
  { "PMTRIG", INS_TYPE_MISC },
  { "R2B", INS_TYPE_MISC },
  { "S2R", INS_TYPE_MISC },
  { "SETCTAID", INS_TYPE_MISC },
  { "SETLMEMBASE", INS_TYPE_MISC },
  { "VOTE", INS_TYPE_MISC },
  { "VOTE_VTG", INS_TYPE_MISC },
  { "F2F", INS_TYPE_MISC },
  { "F2I", INS_TYPE_MISC },
  { "I2F", INS_TYPE_MISC },
  { "I2I", INS_TYPE_MISC },
  { "I2IP", INS_TYPE_MISC },
  { "FRND", INS_TYPE_MISC },
  { "MOV", INS_TYPE_MISC },
  { "MOV32I", INS_TYPE_MISC },
  { "PRMT", INS_TYPE_MISC },
  { "SEL", INS_TYPE_MISC },
  { "SGXT", INS_TYPE_MISC },
  { "SHFL", INS_TYPE_MISC },
  { "PLOP3", INS_TYPE_MISC },
  { "PSETP", INS_TYPE_MISC },
  { "P2R", INS_TYPE_MISC },
  { "R2P", INS_TYPE_MISC },
  { "SUATOM", INS_TYPE_MISC },
  { "SULD", INS_TYPE_MISC },
  { "SURED", INS_TYPE_MISC },
  { "SUST", INS_TYPE_MISC }
};


std::set<std::string> Instruction::opcode_jump = {
  { "BRA" }, { "BRX" }, { "JMP" }, { "JMX" }, { "BREAK" }, { "JMXU" }
};


std::set<std::string> Instruction::opcode_call = {
  { "CAL" }, { "CALL" }
};


// avoid Barrier Set Convergence Synchronization Point, SSY, BSSY
// TODO(Keren): add more sync instructions
std::set<std::string> Instruction::opcode_sync = {
  { "SYNC" }, { "BSYNC" }
};

}
