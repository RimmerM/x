#pragma once

#include "../util/types.h"
#include "../compiler/context.h"

struct Value;
struct Inst;
struct Block;
struct Type;

// A single usage of a value by an instruction.
struct Use {
    Value* value;
    Inst* user;
};

// A local register containing the result of some operation.
struct Value {
    enum Kind {
        Arg,

        FirstConst,
        ConstInt = FirstConst,
        ConstFloat,
        ConstString,
        LastConst = ConstString,

        FirstInst,

        // Primitives: conversion.
        InstTrunc,
        InstFTrunc,
        InstZExt,
        InstSExt,
        InstFExt,

        // Primitives: arithmetic.
        InstAdd,
        InstSub,
        InstMul,
        InstDiv,
        InstIDiv,
        InstRem,
        InstIRem,
        InstFAdd,
        InstFSub,
        InstFMul,
        InstFDiv,

        InstICmp,
        InstFCmp,

        InstShl,
        InstShr,
        InstSar,
        InstAnd,
        InstOr,
        InstXor,

        // Construction.
        InstRecord,
        InstTup,
        InstFun,

        // Function calls.
        InstCall,
        InstCallGen,
        InstCallDyn,
        InstCallDynGen,
        InstCallForeign,

        // Control flow.
        InstJe,
        InstJmp,
        InstRet,
        InstPhi,
    };

    Block* block;
    Type* type;

    // Each instruction that uses this value.
    Array<Use> uses;

    // Each block that uses this value.
    Array<Block*> blockUses;

    // Data for use by the code generator.
    void* codegen = nullptr;

    U32 name;
    Kind kind;
};

// A value provided through a function parameter.
struct Arg: Value {
    U32 index;
};

// An immediate value that can be used by instructions.
struct ConstInt: Value {
    I64 value;
};

struct ConstFloat: Value {
    double value;
};

struct ConstString: Value {
    const char* value;
    Size length;
};

// A single operation that can be performed inside a function block.
struct Inst: Value {
    Value** usedValues;
    Size usedCount = 0;
};

struct InstCast: Inst {
    Value* from;
};

struct InstBinary: Inst {
    Value* lhs, *rhs;
};

/*
 * Conversion instructions
 */
struct InstTrunc: InstCast {};
struct InstFTrunc: InstCast {};
struct InstZExt: InstCast {};
struct InstSExt: InstCast {};
struct InstFExt: InstCast {};

/*
 * Arithmetic instructions - these must be performed on two integers, float or vectors of the same type.
 */
struct InstAdd: InstBinary {};
struct InstSub: InstBinary {};
struct InstMul: InstBinary {};
struct InstDiv: InstBinary {};
struct InstIDiv: InstBinary {};
struct InstRem: InstBinary {};
struct InstIRem: InstBinary {};

struct InstFAdd: InstBinary {};
struct InstFSub: InstBinary {};
struct InstFMul: InstBinary {};
struct InstFDiv: InstBinary {};

enum class ICmp {
    eq, neq, gt, ge, lt, le, igt, ige, ilt, ile,
};

struct InstICmp: InstBinary {
    ICmp cmp;
};

enum class FCmp {
    eq, neq, gt, ge, lt, le,
};

struct InstFCmp: InstBinary {
    FCmp cmp;
};

/*
 * Bitwise instructions - must be performed on integer types or integer vectors
 */
struct InstShift: Inst {
    Value* arg, *amount;
};

struct InstShl: InstShift {};
struct InstShr: InstShift {};
struct InstSar: InstShift {};

struct InstAnd: InstBinary {};
struct InstOr: InstBinary {};
struct InstXor: InstBinary {};

/*
 * Value construction.
 */
struct InstRecord: Inst {
    struct Con* con;
    Value* arg;
};

struct InstTup: Inst {
    Value** fields;
    Size fieldCount;
};

struct InstFun: Inst {
    struct Function* body;
    Value** frame;
    Size frameCount;
};

/*
 * Function calls.
 */
struct InstCall: Inst {
    struct Function* fun;
    Value** args;
    Size argCount;
};

struct InstCallGen: InstCall {};

struct InstCallDyn: Inst {
    Value* fun;
    Value** args;
    Size argCount;
};

struct InstCallDynGen: InstCallDyn {};

struct InstCallForeign: Inst {
    struct ForeignFunction* fun;
    Value** args;
    Size argCount;
};

/*
 * Control flow.
 */

// Conditional branch to one of two blocks.
struct InstJe: Inst {
    Value* cond;
    Block* then;
    Block* otherwise;
};

// Unconditional branch to a different block.
struct InstJmp: Inst {
    Block* to;
};

// Return the provided value to the parent function.
struct InstRet: Inst {
    // This can be null if the instruction returns nothing.
    Value* value;
};

// ϕ-node, like LLVM. If any are used, they must be the first instructions in the block.
struct InstPhi: Inst {
    struct Alt {
        Block* fromBlock;
        Value* value;
    };

    Alt* alts;
    Size altCount;
};

inline bool isTerminating(Inst::Kind kind) {
    return kind == Inst::InstRet || kind == Inst::InstJe || kind == Inst::InstJmp;
}

ConstInt* constInt(Block* block, I64 value);
ConstFloat* constFloat(Block* block, double value);
ConstString* constString(Block* block, const char* value, Size length);

InstTrunc* trunc(Block* block, Id name, Value* from, Type* to);
InstFTrunc* ftrunc(Block* block, Id name, Value* from, Type* to);
InstZExt* zext(Block* block, Id name, Value* from, Type* to);
InstSExt* sext(Block* block, Id name, Value* from, Type* to);
InstFExt* fext(Block* block, Id name, Value* from, Type* to);

InstAdd* add(Block* block, Id name, Value* lhs, Value* rhs);
InstSub* sub(Block* block, Id name, Value* lhs, Value* rhs);
InstMul* mul(Block* block, Id name, Value* lhs, Value* rhs);
InstDiv* div(Block* block, Id name, Value* lhs, Value* rhs);
InstIDiv* idiv(Block* block, Id name, Value* lhs, Value* rhs);
InstRem* rem(Block* block, Id name, Value* lhs, Value* rhs);
InstIRem* irem(Block* block, Id name, Value* lhs, Value* rhs);

InstFAdd* fadd(Block* block, Id name, Value* lhs, Value* rhs);
InstFSub* fsub(Block* block, Id name, Value* lhs, Value* rhs);
InstFMul* fmul(Block* block, Id name, Value* lhs, Value* rhs);
InstFDiv* fdiv(Block* block, Id name, Value* lhs, Value* rhs);

InstICmp* icmp(Block* block, Id name, Value* lhs, Value* rhs, ICmp cmp);
InstFCmp* fcmp(Block* block, Id name, Value* lhs, Value* rhs, FCmp cmp);

InstShl* shl(Block* block, Id name, Value* arg, Value* amount);
InstShr* shr(Block* block, Id name, Value* arg, Value* amount);
InstSar* sar(Block* block, Id name, Value* arg, Value* amount);
InstAnd* bitand_(Block* block, Id name, Value* lhs, Value* rhs);
InstOr* or_(Block* block, Id name, Value* lhs, Value* rhs);
InstXor* xor_(Block* block, Id name, Value* lhs, Value* rhs);

InstRecord* record(Block* block, Id name, struct Con* con, Value* arg);
InstTup* tup(Block* block, Id name, Type* type, Size fieldCount);
InstFun* fun(Block* block, Id name, struct Function* body, Type* type, Size frameCount);

InstCall* call(Block* block, Id name, struct Function* fun, Size argCount);
InstCallGen* callGen(Block* block, Id name, struct Function* fun, Size argCount);
InstCallDyn* callDyn(Block* block, Id name, Value* fun, Size argCount);
InstCallDynGen* callDynGen(Block* block, Id name, Value* fun, Size argCount);
InstCallForeign* callForeign(Block* block, Id name, struct ForeignFunction* fun, Size argCount);

// Stores an argument into a function call instruction.
// Must be called for each argument after creation.
void setArg(Inst* inst, Value** args, Size index, Value* arg);

InstJe* je(Block* block, Value* cond, Block* then, Block* otherwise);
InstJmp* jmp(Block* block, Block* to);
InstRet* ret(Block* block, Value* value = nullptr);
InstPhi* phi(Block* block, Id name, InstPhi::Alt* alts, Size altCount);
