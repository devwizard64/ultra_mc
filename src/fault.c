#include <ultra64.h>

#include "main.h"
#include "fault.h"
#include "apphi.h"
#include "video.h"

#define T_LST   0x00
#define T_STR   0x01
#define I_NULL  0x00
#define I_BDST  0x00
#define I_JDST  0x01
#define I_F0    0x02
#define I_R0    0x03
#define I_R1    0x04
#define I_R2    0x05
#define I_R3    0x06
#define I_F1    0x07
#define I_IMM   0x08
#define I_CODE  0x09
#define I_RS    0x0A
#define I_RT    0x0B
#define I_RD    0x0C
#define I_C0REG 0x0D
#define I_C1CTL 0x0E
#define I_LEN   0x0F
#define I_OP    I_F0
#define I_FUNC  I_F1
#define I_SA    I_R3
#define I_CACHE I_R1
#define I_C1FMT I_R0
#define I_FT    I_R1
#define I_FS    I_R2
#define I_FD    I_R3

struct lst_t
{
    u8 t;
    u8 a0;
    u8 a1;
    u8 a2;
    const void *v0;
    const void *v1;
};

static const char *const str_cause_table[] =
{
    "Interrupt",
    "TLB modification",
    "TLB exception on load",
    "TLB exception on store",
    "Address error on load",
    "Address error on store",
    "Bus error on inst.",
    "Bus error on data",
    "System call exception",
    "Breakpoint exception",
    "Reserved instruction",
    "Coprocessor unusable",
    "Arithmetic overflow",
    "Trap exception",
    "Virtual coherency on inst.",
    "Floating point exception",
    "Watchpoint exception",
    "Virtual coherency on data",
};

static const char *const str_fpcsr_table[] =
{
    "Unimplemented operation",
    "Invalid operation",
    "Division by zero",
    "Overflow",
    "Underflow",
    "Inexact operation",
};

static const char *const str_gpr_table[] =
{
    "AT:%08X  V0:%08X  V1:%08X",
    "A0:%08X  A1:%08X  A2:%08X",
    "A3:%08X  T0:%08X  T1:%08X",
    "T2:%08X  T3:%08X  T4:%08X",
    "T5:%08X  T6:%08X  T7:%08X",
    "S0:%08X  S1:%08X  S2:%08X",
    "S3:%08X  S4:%08X  S5:%08X",
    "S6:%08X  S7:%08X  T8:%08X",
    "T9:%08X  GP:%08X  SP:%08X",
    "S8:%08X  RA:%08X  LO:%08X",
    "HI:%08X  PC:%08X  VA:%08X",
};

static const char *const str_gpr[] =
{
    "r0", "at", "v0", "v1",
    "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3",
    "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3",
    "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1",
    "gp", "sp", "s8", "ra",
};

static const char *const str_cop0_reg[] =
{
    "Index",
    "Random",
    "EntryLo0",
    "EntryLo1",
    "Context",
    "PageMask",
    "Wired",
    NULL,
    "BadVAddr",
    "Count",
    "EntryHi",
    "Compare",
    "Status",
    "Cause",
    "EPC",
    "PRId",
    "Config",
    "LLAddr",
    "WatchLo",
    "WatchHi",
    "XContext",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "PErr",
    "CacheErr",
    "TagLo",
    "TagHi",
    "ErrorEPC",
    NULL,
};

/* cop1_ctl = {0x1F: "FCSR"} */

static const char str_cop1_fmt[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    's',  'd',  0x00, 0x00, 'w',  'l',  0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const char str_s_s_d[]     = "%s  %s  %d";
static const char str_s_s_s[]     = "%s  %s  %s";
static const char str_s[]         = "%s";
static const char str_05X[]       = "%05X";
static const char str_s_s[]       = "%s  %s";
static const char str_s_08X[]     = "%s  %08X";
static const char str_s_04X[]     = "%s  %04X";
static const char str_fd_fd_fd[]  = "f%-2d f%-2d f%d";
static const char str_fd_fd[]     = "f%-2d f%d";
static const char str_s_fd[]      = "%s  f%d";
static const char str_08X[]       = "%08X";
static const char str_s_s_08X[]   = "%s  %s  %08X";
static const char str_s_s_04X[]   = "%s  %s  %04X";
static const char str_s_04X_s[]   = "%s  %04X(%s)";
static const char str_02X_04X_s[] = "%02X  %04X(%s)";
static const char str_fd_04X_s[]  = "f%-2d %04X(%s)";

static const struct lst_t lst_special[] =
{
    {T_STR, I_RD,    I_RT,    I_SA,    "sll",          str_s_s_d},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RD,    I_RT,    I_SA,    "srl",          str_s_s_d},
    {T_STR, I_RD,    I_RT,    I_SA,    "sra",          str_s_s_d},
    {T_STR, I_RD,    I_RT,    I_RS,    "sllv",         str_s_s_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RD,    I_RT,    I_RS,    "srlv",         str_s_s_s},
    {T_STR, I_RD,    I_RT,    I_RS,    "srav",         str_s_s_s},
    {T_STR, I_RS,    I_NULL,  I_NULL,  "jr",           str_s},
    {T_STR, I_RS,    I_NULL,  I_NULL,  "jalr",         str_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_CODE,  I_NULL,  I_NULL,  "syscall",      str_05X},
    {T_STR, I_CODE,  I_NULL,  I_NULL,  "break",        str_05X},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_NULL,  I_NULL,  I_NULL,  "sync",         NULL},
    {T_STR, I_RD,    I_NULL,  I_NULL,  "mfhi",         str_s},
    {T_STR, I_RS,    I_NULL,  I_NULL,  "mthi",         str_s},
    {T_STR, I_RD,    I_NULL,  I_NULL,  "mflo",         str_s},
    {T_STR, I_RS,    I_NULL,  I_NULL,  "mtlo",         str_s},
    {T_STR, I_RD,    I_RT,    I_RS,    "dsllv",        str_s_s_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RD,    I_RT,    I_RS,    "dsrlv",        str_s_s_s},
    {T_STR, I_RD,    I_RT,    I_RS,    "dsrav",        str_s_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "mult",         str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "multu",        str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "div",          str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "divu",         str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "dmult",        str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "dmultu",       str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "ddiv",         str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "ddivu",        str_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "add",          str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "addu",         str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "sub",          str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "subu",         str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "and",          str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "or",           str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "xor",          str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "nor",          str_s_s_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RD,    I_RS,    I_RT,    "slt",          str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "sltu",         str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "dadd",         str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "daddu",        str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "dsub",         str_s_s_s},
    {T_STR, I_RD,    I_RS,    I_RT,    "dsubu",        str_s_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "tge",          str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "tgeu",         str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "tlt",          str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "tltu",         str_s_s},
    {T_STR, I_RS,    I_RT,    I_NULL,  "teq",          str_s_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RS,    I_RT,    I_NULL,  "tne",          str_s_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RD,    I_RT,    I_SA,    "dsll",         str_s_s_d},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RD,    I_RT,    I_SA,    "dsrl",         str_s_s_d},
    {T_STR, I_RD,    I_RT,    I_SA,    "dsra",         str_s_s_d},
    {T_STR, I_RD,    I_RT,    I_SA,    "dsll32",       str_s_s_d},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RD,    I_RT,    I_SA,    "dsrl32",       str_s_s_d},
    {T_STR, I_RD,    I_RT,    I_SA,    "dsra32",       str_s_s_d},
};

static const struct lst_t lst_regimm[] =
{
    {T_STR, I_RS,    I_BDST,  I_NULL,  "bltz",         str_s_08X},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "bgez",         str_s_08X},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "bltzl",        str_s_08X},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "bgezl",        str_s_08X},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RS,    I_IMM,   I_NULL,  "tgei",         str_s_04X},
    {T_STR, I_RS,    I_IMM,   I_NULL,  "tgeiu",        str_s_04X},
    {T_STR, I_RS,    I_IMM,   I_NULL,  "tlti",         str_s_04X},
    {T_STR, I_RS,    I_IMM,   I_NULL,  "tltiu",        str_s_04X},
    {T_STR, I_RS,    I_IMM,   I_NULL,  "teqi",         str_s_04X},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RS,    I_IMM,   I_NULL,  "tnei",         str_s_04X},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "bltzal",       str_s_08X},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "bgezal",       str_s_08X},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "bltzall",      str_s_08X},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "bgezall",      str_s_08X},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
};

static const struct lst_t lst_cop0_func[] =
{
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_NULL,  I_NULL,  I_NULL,  "tlbr",         NULL},
    {T_STR, I_NULL,  I_NULL,  I_NULL,  "tlbwi",        NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_NULL,  I_NULL,  I_NULL,  "tlbwr",        NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_NULL,  I_NULL,  I_NULL,  "tlbp",         NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_NULL,  I_NULL,  I_NULL,  "eret",         NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
};

static const struct lst_t lst_cop0_rs[] =
{
    {T_STR, I_RT,    I_C0REG, I_NULL,  "mfc0",         str_s_s},
    {T_STR, I_RT,    I_C0REG, I_NULL,  "dmfc0",        str_s_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RT,    I_C0REG, I_NULL,  "mtc0",         str_s_s},
    {T_STR, I_RT,    I_C0REG, I_NULL,  "dmtc0",        str_s_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_FUNC,  I_NULL,  I_NULL,  lst_cop0_func,  NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
};

static const struct lst_t lst_bc1[] =
{
    {T_STR, I_BDST,  I_NULL,  I_NULL,  "bc1f",         str_08X},
    {T_STR, I_BDST,  I_NULL,  I_NULL,  "bc1t",         str_08X},
    {T_STR, I_BDST,  I_NULL,  I_NULL,  "bc1fl",        str_08X},
    {T_STR, I_BDST,  I_NULL,  I_NULL,  "bc1tl",        str_08X},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
};

static const struct lst_t lst_cop1_func[] =
{
    {T_STR, I_FD,    I_FS,    I_FT,    "add.%c",       str_fd_fd_fd},
    {T_STR, I_FD,    I_FS,    I_FT,    "sub.%c",       str_fd_fd_fd},
    {T_STR, I_FD,    I_FS,    I_FT,    "mul.%c",       str_fd_fd_fd},
    {T_STR, I_FD,    I_FS,    I_FT,    "div.%c",       str_fd_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "sqrt.%c",      str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "abs.%c",       str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "mov.%c",       str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "neg.%c",       str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "round.l.%c",   str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "trunc.l.%c",   str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "ceil.l.%c",    str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "floor.l.%c",   str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "round.w.%c",   str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "trunc.w.%c",   str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "ceil.w.%c",    str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "floor.w.%c",   str_fd_fd},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_FD,    I_FS,    I_NULL,  "cvt.s.%c",     str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "cvt.d.%c",     str_fd_fd},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_FD,    I_FS,    I_NULL,  "cvt.w.%c",     str_fd_fd},
    {T_STR, I_FD,    I_FS,    I_NULL,  "cvt.l.%c",     str_fd_fd},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.f.%c",       str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.un.%c",      str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.eq.%c",      str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.ueq.%c",     str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.olt.%c",     str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.ult.%c",     str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.ole.%c",     str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.ule.%c",     str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.sf.%c",      str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.ngle.%c",    str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.seq.%c",     str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.ngl.%c",     str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.lt.%c",      str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.nge.%c",     str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.le.%c",      str_fd_fd},
    {T_STR, I_FS,    I_FT,    I_NULL,  "c.ngt.%c",     str_fd_fd},
};

static const struct lst_t lst_cop1_rs[] =
{
    {T_STR, I_RT,    I_FS,    I_NULL,  "mfc1",         str_s_fd},
    {T_STR, I_RT,    I_FS,    I_NULL,  "dmfc1",        str_s_fd},
    {T_STR, I_RT,    I_C1CTL, I_NULL,  "cfc1",         str_s_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RT,    I_FS,    I_NULL,  "mtc1",         str_s_fd},
    {T_STR, I_RT,    I_FS,    I_NULL,  "dmtc1",        str_s_fd},
    {T_STR, I_RT,    I_C1CTL, I_NULL,  "ctc1",         str_s_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_R1,    I_NULL,  I_NULL,  lst_bc1,        NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_FUNC,  I_NULL,  I_NULL,  lst_cop1_func,  NULL},
    {T_LST, I_FUNC,  I_NULL,  I_NULL,  lst_cop1_func,  NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_FUNC,  I_NULL,  I_NULL,  lst_cop1_func,  NULL},
    {T_LST, I_FUNC,  I_NULL,  I_NULL,  lst_cop1_func,  NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
};

static const struct lst_t lst_op[] =
{
    {T_LST, I_FUNC,  I_NULL,  I_NULL,  lst_special,    NULL},
    {T_LST, I_R1,    I_NULL,  I_NULL,  lst_regimm,     NULL},
    {T_STR, I_JDST,  I_NULL,  I_NULL,  "j",            str_08X},
    {T_STR, I_JDST,  I_NULL,  I_NULL,  "jal",          str_08X},
    {T_STR, I_RS,    I_RT,    I_BDST,  "beq",          str_s_s_08X},
    {T_STR, I_RS,    I_RT,    I_BDST,  "bne",          str_s_s_08X},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "blez",         str_s_08X},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "bgtz",         str_s_08X},
    {T_STR, I_RT,    I_RS,    I_IMM,   "addi",         str_s_s_04X},
    {T_STR, I_RT,    I_RS,    I_IMM,   "addiu",        str_s_s_04X},
    {T_STR, I_RT,    I_RS,    I_IMM,   "slti",         str_s_s_04X},
    {T_STR, I_RT,    I_RS,    I_IMM,   "sltiu",        str_s_s_04X},
    {T_STR, I_RT,    I_RS,    I_IMM,   "andi",         str_s_s_04X},
    {T_STR, I_RT,    I_RS,    I_IMM,   "ori",          str_s_s_04X},
    {T_STR, I_RT,    I_RS,    I_IMM,   "xori",         str_s_s_04X},
    {T_STR, I_RT,    I_IMM,   I_NULL,  "lui",          str_s_04X},
    {T_LST, I_R0,    I_NULL,  I_NULL,  lst_cop0_rs,    NULL},
    {T_LST, I_R0,    I_NULL,  I_NULL,  lst_cop1_rs,    NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RS,    I_RT,    I_BDST,  "beql",         str_s_s_08X},
    {T_STR, I_RS,    I_RT,    I_BDST,  "bnel",         str_s_s_08X},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "blezl",        str_s_08X},
    {T_STR, I_RS,    I_BDST,  I_NULL,  "bgtzl",        str_s_08X},
    {T_STR, I_RT,    I_RS,    I_IMM,   "daddi",        str_s_s_04X},
    {T_STR, I_RT,    I_RS,    I_IMM,   "daddiu",       str_s_s_04X},
    {T_STR, I_RT,    I_IMM,   I_RS,    "ldl",          str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "ldr",          str_s_04X_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RT,    I_IMM,   I_RS,    "lb",           str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "lh",           str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "lwl",          str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "lw",           str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "lbu",          str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "lhu",          str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "lwr",          str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "lwu",          str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "sb",           str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "sh",           str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "swl",          str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "sw",           str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "sdl",          str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "sdr",          str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "swr",          str_s_04X_s},
    {T_STR, I_CACHE, I_IMM,   I_RS,    "cache",        str_02X_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "ll",           str_s_04X_s},
    {T_STR, I_FT,    I_IMM,   I_RS,    "lwc1",         str_fd_04X_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RT,    I_IMM,   I_RS,    "lld",          str_s_04X_s},
    {T_STR, I_FT,    I_IMM,   I_RS,    "ldc1",         str_fd_04X_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RT,    I_IMM,   I_RS,    "ld",           str_s_04X_s},
    {T_STR, I_RT,    I_IMM,   I_RS,    "sc",           str_s_04X_s},
    {T_STR, I_FT,    I_IMM,   I_RS,    "swc1",         str_fd_04X_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RT,    I_IMM,   I_RS,    "scd",          str_s_04X_s},
    {T_STR, I_FT,    I_IMM,   I_RS,    "sdc1",         str_fd_04X_s},
    {T_LST, I_NULL,  I_NULL,  I_NULL,  NULL,           NULL},
    {T_STR, I_RT,    I_IMM,   I_RS,    "sd",           str_s_04X_s},
};

static const struct lst_t lst_start =
    {T_LST, I_OP,    I_NULL,  I_NULL,  lst_op,         NULL};

static OSMesgQueue mq_fault;
static OSMesg msg_fault[4];

/* 0x803359B4 */ extern OSThread *__osFaultedThread;

static void fault_draw_context(u16 *c, OSThread *thread)
{
    const char *str;
    u32 i;
    u32 *r;
    c += 3*(45-38);
    i = thread->context.cause >> 2 & 0x1F;
    if (i == 0x17)
    {
        i = 0x10;
    }
    else if (i == 0x1F)
    {
        i = 0x11;
    }
    str = str_cause_table[i];
    if (i == 0x0F)
    {
        u32 m;
        for (i = 0, m = 0x00020000; i < 6; i++, m >>= 1)
        {
            if (thread->context.fpcsr & m)
            {
                str = str_fpcsr_table[i];
                break;
            }
        }
    }
    fault_printf(c, "THREAD:%d  (%s)", thread->id, str);
    c += VIDEO_W*SP_H;
    r = (u32 *)&thread->context.at;
    for (i = 0; i < lenof(str_gpr_table); i++)
    {
        fault_printf(c, str_gpr_table[i], r[1], r[3], r[5]);
        c += VIDEO_W*LN_H;
        r += 6;
    }
    c += VIDEO_W*(SP_H-LN_H);
    r = (u32 *)&thread->context.fp0.d;
    for (i = 0; i < 30; i += 6)
    {
        fault_printf(
            c, "F%02d:%08X F%02d:%08X F%02d:%08X",
            i, r[0], i+2, r[2], i+4, r[4]
        );
        c += VIDEO_W*LN_H;
        r += 6;
    }
    fault_printf(c, "F30:%08X FPCSR:%08X", r[0], thread->context.fpcsr);
}

static void fault_draw_rdram(u16 *c, OSThread *thread, u32 addr, u32 page)
{
    u32 i;
    for (i = 0; i < 20; i++)
    {
        u32 mask = page == 1 ? ~0x0F : ~0x03;
        const char *str;
        if (addr == (thread->context.pc & mask))
        {
            str = "X";
        }
        else if (addr == (thread->context.badvaddr & mask))
        {
            str = "A";
        }
        else if (addr == (thread->context.sp & mask))
        {
            str = "S";
        }
        else
        {
            goto nomark;
        }
        fault_print(c - 6*1, str);
    nomark:
        fault_printf(c, "%08X:", addr);
        c += 6*10;
        if (osVirtualToPhysical((void *)addr) != 0xFFFFFFFF)
        {
            if (page == 1)
            {
                fault_printf(
                    c, "%08X %08X %08X %08X",
                    ((u32 *)addr)[0], ((u32 *)addr)[1],
                    ((u32 *)addr)[2], ((u32 *)addr)[3]
                );
            }
            else
            {
                u32 inst = *(u32 *)addr;
                if (inst == 0x00000000)
                {
                    fault_print(c, "nop");
                }
                else
                {
                    union
                    {
                        const char *str;
                        u32 iu;
                    }
                    fmt[I_LEN];
                    const struct lst_t *lst;
                    fmt[I_BDST].iu = addr+4 + ((s32)inst << 16 >> 14);
                    fmt[I_JDST].iu =
                        (addr & 0xF0000000) | (inst << 2 & 0x0FFFFFFC);
                    fmt[I_F0  ].iu = inst >> 26 & 0x3F;
                    fmt[I_R0  ].iu = inst >> 21 & 0x1F;
                    fmt[I_R1  ].iu = inst >> 16 & 0x1F;
                    fmt[I_R2  ].iu = inst >> 11 & 0x1F;
                    fmt[I_R3  ].iu = inst >>  6 & 0x1F;
                    fmt[I_F1  ].iu = inst >>  0 & 0x3F;
                    fmt[I_IMM ].iu = inst >>  0 & 0xFFFF;
                    fmt[I_CODE].iu = inst >>  6 & 0x000FFFFF;
                    fmt[I_RS   ].str = str_gpr[fmt[I_R0].iu];
                    fmt[I_RT   ].str = str_gpr[fmt[I_R1].iu];
                    fmt[I_RD   ].str = str_gpr[fmt[I_R2].iu];
                    fmt[I_C0REG].str = str_cop0_reg[fmt[I_R2].iu];
                    fmt[I_C1CTL].str =
                        fmt[I_R2].iu == 0x1F ? "FCSR" : "(invalid)";
                    lst = &lst_start;
                    do
                    {
                        lst = (const struct lst_t *)lst->v0 + fmt[lst->a0].iu;
                    }
                    while (lst->t == T_LST && lst->v0 != NULL);
                    if (lst->v0 != NULL)
                    {
                        fault_printf(c, lst->v0, str_cop1_fmt[fmt[I_C1FMT].iu]);
                        if (lst->v1 != NULL)
                        {
                            fault_printf(
                                c + 6*12, lst->v1, fmt[lst->a0].iu,
                                fmt[lst->a1].iu, fmt[lst->a2].iu
                            );
                        }
                    }
                    else
                    {
                        fault_printf(c, "invalid     %08X", inst);
                    }
                }
            }
        }
        else
        {
            fault_printf(
                c,
                page == 1 ?
                    "-------- -------- -------- --------" :
                    "----------------------------"
            );
        }
        c += VIDEO_W*LN_H - 6*10;
        if (page == 1)
        {
            addr += 0x10;
        }
        else
        {
            addr += 0x04;
        }
    }
}

void fault_main(unused void *arg)
{
    OSThread *thread;
    u32 buf;
    u32 page;
    u32 addr;
    u32 timer;
    osCreateMesgQueue(MQ(fault));
    osSetEventMesg(OS_EVENT_CPU_BREAK, &mq_fault, (OSMesg)0),
    osSetEventMesg(OS_EVENT_FAULT,     &mq_fault, (OSMesg)0),
    osRecvMesg(&mq_fault, NULL, OS_MESG_BLOCK);
    thread = __osFaultedThread;
    osDestroyThread(&thread_video);
    vi_init(OS_VI_NTSC_LPN1);
    buf  = 0;
    page = 0;
    addr = (thread->context.pc-0x04*8) & ~0x0F;
    timer = 0;
    while (true)
    {
        u16 *c;
        u16 down;
        input_update();
        buf ^= 1;
        c = video_cimg[buf];
        bzero(c, sizeof(*c)*320*240);
        c += VIDEO_W*TEXT_Y + TEXT_X;
        down = controller_table[0].held;
        if (down == 0)
        {
            timer = 0;
        }
        else if (++timer < 10)
        {
            down = 0;
        }
        down |= controller_table[0].down;
        if ((down & L_TRIG) && page > 0)
        {
            page--;
        }
        if ((down & R_TRIG) && page < 2)
        {
            page++;
        }
        fault_printf(c, "devwizard - sm64mc  " _TIME "  page:%d", page);
        c += VIDEO_W*LN_H*2;
        if (page == 0)
        {
            fault_draw_context(c, thread);
        }
        else
        {
            if (down & U_JPAD)
            {
                addr -= 0x10;
            }
            if (down & D_JPAD)
            {
                addr += 0x10;
            }
            if (down & L_JPAD)
            {
                addr -= 0x1000;
            }
            if (down & R_JPAD)
            {
                addr += 0x1000;
            }
            if (down & L_CBUTTONS)
            {
                addr = (thread->context.sp-0x04*8) & ~0x0F;
            }
            if (down & R_CBUTTONS)
            {
                addr = (thread->context.pc-0x04*8) & ~0x0F;
            }
            fault_draw_rdram(c, thread, addr, page);
        }
        osWritebackDCacheAll();
        osRecvMesg(&mq_apphi_vi, NULL, OS_MESG_BLOCK);
        osViSwapBuffer(video_cimg[buf]);
        osRecvMesg(&mq_apphi_vi, NULL, OS_MESG_BLOCK);
        osViBlack(false);
    }
}
