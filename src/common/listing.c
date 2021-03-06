
#include "common.h"

void show_label(unsigned long addr, FILE* fp)
{
    for(Value* val = resetVars(); val != NULL; val = iterateVars()) {
        if(val->type == ADDRESS && val->data.addr == addr) {
            Index idx = getVarName();
            //fprintf(stderr, "index: %d\n", idx);
            fprintf(fp, "\n      %s:\n", getStr(idx));
            return;
        }
    }
    // for(uint32_t i = 0; i < vs->len; i++) {
    //     Value* val = getVal(i);
    //     if(vs->list[i].val.type == ADDRESS && vs->list[i].val.data.addr == addr) {
    //         fprintf(fp, "\n      %s:\n", getStr(vs->list[i].name));
    //         return;
    //     }
    // }
}

void showListing(FILE* outf)
{
    uint8_t op;
    bool finished = false;

    fprintf(outf, "--------- Listing of %s --------- \n", getVmName());
    //fprintf(outf, "--------- Listing --------- \n");

    while(!finished) {

        unsigned long addr = getInstrIndex();
        show_label(addr, outf);
        READ_VM_OBJ(uint8_t, op);
        fprintf(outf, "%04ld: %s\t", addr, opToStr(op));
        finished = isInstrEnded();

        switch(op) {

            // exit the VM with a error.
            case OP_ABORT: {
                    // Immediate is a Index
                    Index idx;
                    READ_VM_OBJ(Index, idx);
                    printVal(getVar(idx), outf);
                }
                break;

            case OP_ABORTI: {
                    // immediate is a Value
                    Value val;
                    READ_VM_OBJ(Value, val);
                    printVal(&val, outf);
                }
                break;

            case OP_ABORTR: {
                    // immediate is a register
                    uint8_t reg;
                    READ_VM_OBJ(uint8_t, reg);
                    printVal(getReg(reg), outf);
                }
                break;


            case OP_EXIT:
            case OP_NOP:
            case OP_RETURN:
                // no parameters
                break;

            case OP_CALL: {
                    // operand is a Value index
                    Index idx;
                    READ_VM_OBJ(Index, idx);
                    fprintf(outf, "<%d>\t", idx);
                    printVal(getVar(idx), outf);
                }
                break;

            case OP_CALLI:{
                    // operand is an immediate Value
                    Value val;
                    READ_VM_OBJ(Value, val);
                    printVal(&val, outf);
                }
                break;

            case OP_CALLR:{
                    // operand is a single register
                    uint8_t reg;
                    READ_VM_OBJ(uint8_t, reg);
                    fprintf(outf, "<%s>\t", regToStr(reg));
                    printVal(getReg(reg), outf);
                }
                break;

            case OP_TRAP: {
                    // operand is a uint16_t
                    uint16_t tno;
                    READ_VM_OBJ(uint16_t, tno);
                    fprintf(outf, "<%d>\t", tno);
                }
                break;

            case OP_JMP:{
                    // operand is a Value index
                    Index idx;
                    READ_VM_OBJ(Index, idx);
                    fprintf(outf, "<%d>\t", idx);
                    printVal(getVar(idx), outf);
                }
                break;

            case OP_JMPI:{
                    // operand is an immediate Value
                    Value val;
                    READ_VM_OBJ(Value, val);
                    printVal(&val, outf);
                }
                break;

            case OP_JMPR:{
                    // operand is a single register
                    RegNumType reg;
                    READ_VM_OBJ(RegNumType, reg);
                    fprintf(outf, "<%s>\t", regToStr(reg));
                    printVal(getReg(reg), outf);
                }
                break;

            case OP_BR:{
                    // operand is a Value Index
                    Index idx;
                    READ_VM_OBJ(Index, idx);
                    fprintf(outf, "<%d>\t", idx);
                    printVal(getVar(idx), outf);
                }
                break;

            case OP_BRI: {
                    // operand is an immediate Value
                    Value val;
                    READ_VM_OBJ(Value, val);
                    printVal(&val, outf);
                }
                break;

            case OP_BRR: {
                    // operand is a single register
                    RegNumType reg;
                    READ_VM_OBJ(RegNumType, reg);
                    fprintf(outf, "<%s>\t", regToStr(reg));
                    printVal(getReg(reg), outf);
                }
                break;

            case OP_PUSH: {
                    // operand is a Value index
                    Index idx;
                    READ_VM_OBJ(Index, idx);
                    fprintf(outf, "<%d>\t", idx);
                    printVal(getVar(idx), outf);
                }
                break;

            case OP_PUSHI: {
                    // operand is an immediate Value
                    Value val;
                    READ_VM_OBJ(Value, val);
                    printVal(&val, outf);
                }
                break;

            case OP_PUSHR: {
                    // operand is a single register
                    RegNumType reg;
                    READ_VM_OBJ(RegNumType, reg);
                    fprintf(outf, "<%s>\t", regToStr(reg));
                }
                break;

            case OP_POP: {
                    // operand is a single register
                    uint8_t reg;
                    READ_VM_OBJ(uint8_t, reg);
                    fprintf(outf, "<%s>\t", regToStr(reg));
                }
                break;

            case OP_LOAD: {
                    // operand is a single register
                    // operand is a Value index
                    RegNumType reg;
                    READ_VM_OBJ(RegNumType, reg);
                    Index idx;
                    READ_VM_OBJ(Index, idx);
                    fprintf(outf, "<%s>,<%d>\t", regToStr(reg), idx);
                    //printVal(getVar(&vm->vstore, idx), outf);
                    printObj(idx, outf);
                }
                break;

            case OP_LOADI: {
                    // operand is a single register
                    // operand is an immediate Value
                    RegNumType reg;
                    READ_VM_OBJ(RegNumType, reg);
                    fprintf(outf, "<%s>\t", regToStr(reg));
                    Value val;
                    READ_VM_OBJ(Value, val);
                    printVal(&val, outf);
                }
                break;

            case OP_LOADR: {
                    // operand is 2 registers
                    RegNumType regs;
                    READ_VM_OBJ(RegNumType, regs);
                    fprintf(outf, "<%s>", regToStr((regs >> 4) & 0xF));
                    fprintf(outf, ",<%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_STORE: {
                    // operand is a Value index
                    // operand is a single register
                    Index idx;
                    READ_VM_OBJ(Index, idx);
                    fprintf(outf, "<%d>\t", idx);
                    printVal(getVar(idx), outf);
                    RegNumType reg;
                    READ_VM_OBJ(RegNumType, reg);
                    fprintf(outf, "<%s>\t", regToStr(reg));
                }
                break;

            case OP_NOT: {
                    // operand is single register (result in zero flag)
                    RegNumType reg;
                    READ_VM_OBJ(RegNumType, reg);
                    fprintf(outf, "<%s>\t", regToStr(reg));
                    //printVal(&vm->registers[reg], outf);
                }
                break;

            case OP_EQ: {
                    // operand is 2 registers (result in zero flag)
                    RegNumType regs;
                    READ_VM_OBJ(RegNumType, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_NEQ: {
                    // operand is 2 registers (result in zero flag)
                    RegNumType regs;
                    READ_VM_OBJ(RegNumType, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_LEQ: {
                    // operand is 2 registers (result in zero flag)
                    RegNumType regs;
                    READ_VM_OBJ(RegNumType, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_GEQ: {
                    // operand is 2 registers (result in zero flag)
                    RegNumType regs;
                    READ_VM_OBJ(RegNumType, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_LESS: {
                    // operand is 2 registers (result in zero flag)
                    RegNumType regs;
                    READ_VM_OBJ(RegNumType, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_GTR: {
                    // operand is 2 registers (result in zero flag)
                    RegNumType regs;
                    READ_VM_OBJ(RegNumType, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_NEG: {
                    // operand is 2 registers
                    RegNumType regs;
                    READ_VM_OBJ(RegNumType, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_ADD: {
                    // operand is 3 registers
                    uint16_t regs;
                    READ_VM_OBJ(uint16_t, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 8) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_SUB: {
                    // operand is 3 registers
                    uint16_t regs;
                    READ_VM_OBJ(uint16_t, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 8) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_MUL: {
                    // operand is 3 registers
                    uint16_t regs;
                    READ_VM_OBJ(uint16_t, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 8) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_DIV: {
                    // operand is 3 registers
                    uint16_t regs;
                    READ_VM_OBJ(uint16_t, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 8) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            case OP_MOD: {
                    // operand is 3 registers
                    uint16_t regs;
                    READ_VM_OBJ(uint16_t, regs);
                    fprintf(outf, "<%s> ", regToStr((regs >> 8) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr((regs >> 4) & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, ", <%s>", regToStr(regs & 0xF));
                    //printVal(&vm->registers[regs], outf);
                    fprintf(outf, "\t");
                }
                break;

            default:
                fprintf(stderr, "error: unknown instruction: 0x%02X\n", op);
                return;
        }
        fprintf(outf, "\n");
    }
    fprintf(outf, "\n--------- end listing ----------\n\n");
}
