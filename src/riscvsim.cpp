#include "machine.h"
#include "utils.h"

#define T_OPCODE 1            /*fetch different parts in an instruction in maskInstr()*/
#define T_RD 2
#define T_FUNCT3 3
#define T_RS1 4
#define T_RS2 5
#define T_FUNCT7 6
#define T_IMMI 7          
#define T_IMMU 8
#define T_IMMUJ 9
#define T_IMMSB 10
#define T_IMMS 11



enum instrName{
    Iadd, Imul, Isub, Isll, Imulh,Islt, Ixor, Idiv, Isrl, Isra,Ior, Irem, Iand, Ilb, Ilh,Ilw, Ild, Iaddi, Islli, Islti,
    Ixori, Isrli, Israi, Iori, Iandi,Iaddiw, Ijalr, Iecall, Isb, Ish,Isw, Isd, Ibeq, Ibne, Iblt,Ibge, Iauipc, Ilui, Ijal, Ibltu,
    Ibgeu, Ilbu, Ilhu, Ilwu, Isltiu,Isltu, Islliw, Isrliw, Israiw, Iaddw,Isubw, Isllw, Isrlw, Israw, Inop
};


enum instrType{
    R_type, I_type, S_type, SB_type, U_type, UJ_type
};
    
int64_t sig64ext(int len, uint32_t val){
    if((val & (1 << len)) > 0)
        val = val | (0xffffffff << (len + 1));

    return (int64_t)((int32_t)val);
}

int64_t maskInstr(int type, uint32_t ival){
    uint32_t uintRes = 0;
    int64_t ires = 0;

    uint32_t p20;   
    uint32_t p10_1;
    uint32_t p11;
    uint32_t p19_12;
    uint32_t p4_1;
    uint32_t p__11;
    uint32_t p10_5;
    uint32_t p12;
    uint32_t p0_4;
    uint32_t p11_5;

    switch (type){
        case 1:
            uintRes = ival & 0x7f;                 /*opcode*/
            ires = sig64ext(31, uintRes);
            break;
        case 2:
            uintRes = (ival >> 7) & 0x1f;          /*rd*/
            ires = sig64ext(31, uintRes);
            break;
        case 3:
            uintRes = (ival >> 12) & 0x7;          /*funct3*/
            ires = sig64ext(31, uintRes);
            break;
        case 4:
            uintRes = (ival >> 15) & 0x1f;         /*rs1*/
            ires = sig64ext(31, uintRes);
            break;
        case 5:
            uintRes = (ival >> 20) & 0x1f;         /*rs2*/
            ires = sig64ext(31, uintRes);
            break;
        case 6:
            uintRes = (ival >> 25) & 0x7f;         /*funct7*/
            ires = sig64ext(31, uintRes);
            break;
        case 7:
            uintRes = (ival >> 20) & 0xfff;        /*imm[11:0]*/
            ires = sig64ext(11, uintRes);
            break;
        case 8:
            uintRes = ((ival >> 12) & 0xfffff) << 12;      /*imm[31:12]*/
            ires = sig64ext(31, uintRes);
            break;
        case 9:                                 /*imm UJ*/
            p20 = (ival >> 31) & 0x1;   
            p10_1 = (ival >> 21) & 0x3ff;
            p11 = (ival >> 20) & 0x1;
            p19_12 = (ival >> 12) & 0xff;
            uintRes = (p10_1 << 1) | (p11 << 11) | (p19_12 << 12) | (p20 << 20);
            ires = sig64ext(20, uintRes);
            break;
        case 10:                                 /*imm SB*/
            p4_1 = (ival >> 8) & 0xf;
            p__11 = (ival >> 7) & 0x1;
            p10_5 = (ival >> 25) & 0x3f;
            p12 = (ival >> 31) & 0x1;
            uintRes = (p4_1 << 1) | (p10_5 << 5) | (p__11 << 11) | (p12 << 12);
            ires = sig64ext(12, uintRes);
            break;
        case 11:                                  /*imm S*/
            p0_4 = (ival >> 7) & 0x1f;
            p11_5 = (ival >> 25) & 0x7f;
            uintRes = (p0_4 << 0) | (p11_5 << 5);
            ires = sig64ext(11, uintRes);
            break;
        default :
            printf("Wrong args passed to maskInstr!\n");
            ASSERT(false);
            break;
    }
    return ires;
}

void
Machine::Fetch(){
    /*read instr*/
    if(FReg.bubble){
        DRegO.bubble = true;
        DRegO.stall = false;
        return;
    }

    uint64_t instrAddr = translateAddr(this->predPC);
    Instruction instr;
    readBytes(instrAddr, 4, &instr.ival);
    instr.addr = this->predPC;

    /*update PC*/
    this->PC = this->predPC;

    /*update predPC*/
    if(maskInstr(T_OPCODE, instr.ival) == 0x6f){
        /*jal*/
        int64_t jalImm = maskInstr(T_IMMUJ, instr.ival);
        this->predPC += (uint64_t)jalImm;
    }
    else if(maskInstr(T_OPCODE, instr.ival) == 0x63){
        /*bne beq ...*/
            int64_t tarPC = this->PC + maskInstr(T_IMMSB, instr.ival);
            if(MyPred.Predict(instr.addr)){
                this->predPC = tarPC;
                DRegO.predJ = true;
            }
            else{
                this->predPC += 4;
                DRegO.predJ = false;
            }
        }
        else
            this->predPC += 4;
    /*output signal*/
    DRegO.instr = instr;
    DRegO.bubble = false;
    DRegO.stall = false;
}

void
Machine::Decode(){
    /*deal with bubble and stall*/
    ERegO = DReg;
    ERegO.bubble = false;
    ERegO.stall = false;
    if(DReg.bubble){
        ERegO.stall = false;
        ERegO.bubble = true;
        return;
    }
    if(DReg.stall){
        ERegO.stall = false;
        ERegO.bubble = true;
        return;
    }

    /*decode instruction and read valA and valB from register*/
    Instruction instr = DReg.instr;
    uint32_t opcode = 0x7f & instr.ival;
    instr.opcode = opcode;

    int64_t sA = 0;
    int64_t sB = 0;
    int64_t vA = 0;
    int64_t vB = 0;
    int64_t imm = 0;

    int64_t funct3 = 0;
    int64_t funct7 = 0;

    switch(opcode){
        case 0x37:
            instr.name = Ilui;
            instr.type = U_type;
            break;
        case 0x17:
            instr.name = Iauipc;
            instr.type = U_type;
            break;
        case 0x6f:
            instr.name = Ijal;
            instr.type = UJ_type;
            break;
        case 0x67:
            instr.name = Ijalr;
            instr.type = I_type;
            break;
        case 0x63:
            funct3 = maskInstr(T_FUNCT3, instr.ival);
            switch (funct3){
                case 0x0:
                    instr.name = Ibeq;
                    break;
                case 0x1:
                    instr.name = Ibne;
                    break;
                case 0x4:
                    instr.name = Iblt;
                    break;
                case 0x5:
                    instr.name = Ibge;
                    break;
                case 0x6:
                    instr.name = Ibltu;
                    break;
                case 0x7:
                    instr.name = Ibgeu;
                    break;
                default:
                    ASSERT(false);
                    break;
            }
            instr.type = SB_type;
            break;
        case 0x3:
            funct3 = maskInstr(T_FUNCT3, instr.ival);
            switch (funct3){
                case 0x0:
                    instr.name = Ilb;
                    break;
                case 0x1:
                    instr.name = Ilh;
                    break; 
                case 0x2:
                    instr.name = Ilw;
                    break;
                case 0x4:
                    instr.name = Ilbu;
                    break;
                case 0x5:
                    instr.name = Ilhu;
                    break;
                case 0x6:
                    instr.name = Ilwu;
                    break;
                case 0x3:
                    instr.name = Ild;
                    break;
                default:
                    ASSERT(false);
                    break;
            }
            instr.type = I_type;
            break;
        case 0x23:
            funct3 = maskInstr(T_FUNCT3, instr.ival);
            switch (funct3){
                case 0x0:
                    instr.name = Isb;
                    break;
                case 0x1:
                    instr.name = Ish;
                    break;
                case 0x2:
                    instr.name = Isw;
                    break;
                case 0x3:
                    instr.name = Isd;
                    break;
                default:
                    ASSERT(false);
                    break;
            }
            instr.type = S_type;
            break;
        case 0x13:
            funct3 = maskInstr(T_FUNCT3, instr.ival);
            switch (funct3){
                case 0x0:
                    instr.name = Iaddi;
                    break;
                case 0x1:
                    instr.name = Islli;
                    break;
                case 0x2:
                    instr.name = Islti;
                    break;
                case 0x3:
                    instr.name = Isltiu;
                    break;
                case 0x4:
                    instr.name = Ixori;
                    break;
                case 0x5:
                    if(maskInstr(T_FUNCT7, instr.ival) > 0)
                        instr.name = Israi;
                    else
                        instr.name = Isrli;
                    break;
                case 0x6:
                    instr.name = Iori;
                    break;
                case 0x7:
                    instr.name = Iandi;
                    break;
                default:
                    ASSERT(false);
                    break;
            }
            instr.type = I_type;
            break;
        case 0x33:
            funct3 = maskInstr(T_FUNCT3, instr.ival);
            funct7 = maskInstr(T_FUNCT7, instr.ival);
            switch (funct3){
                case 0x0:
                    if(funct7 == 0x0)
                        instr.name = Iadd;
                    else
                        instr.name = Isub;
                    break;
                case 0x1:
                    instr.name = Isll;
                    break;
                case 0x2:
                    instr.name = Islt;
                    break;
                case 0x3:
                    instr.name = Isltu;
                    break;
                case 0x4:
                    instr.name = Ixor;
                    break;
                case 0x5:
                    if(funct7 == 0x0)
                        instr.name = Isrl;
                    else 
                        instr.name = Isra;
                    break;
                case 0x6:
                    instr.name = Ior;
                    break;
                case 0x7:
                    instr.name = Iand;
                    break;
                default:
                    ASSERT(false);
                    break;
            }
            instr.type = R_type;
            break;
        case 0x1b:
            funct3 = maskInstr(T_FUNCT3, instr.ival);
            funct7 = maskInstr(T_FUNCT7, instr.ival);
            switch (funct3){
                case 0x0:
                    instr.name = Iaddiw;
                    break;
                case 0x1:
                    instr.name = Islliw;
                    break;
                case 0x5:
                    if(funct7 == 0x0)
                        instr.name = Isrliw;
                    else
                        instr.name = Israiw;
                    break;
                default:
                    ASSERT(false);
                    break;
            }
            instr.type = I_type;
            break;
        case 0x3b:
            funct3 = maskInstr(T_FUNCT3, instr.ival);
            funct7 = maskInstr(T_FUNCT7, instr.ival);
            switch (funct3){
                case 0x0:
                    if(funct7 == 0x0)
                        instr.name = Iaddw;
                    else 
                        instr.name = Isubw;
                    break;
                case 0x1:
                    instr.name = Isllw;
                    break;
                case 0x5:
                    if(funct7 == 0)
                        instr.name = Isrlw;
                    else 
                        instr.name = Israw;
                    break;
                default:
                    ASSERT(false);
                    break;
            }
            instr.type = R_type;
            break;
        case 0x73:
            instr.name = Iecall;
            instr.type = R_type;
            break;
        case 0x0:
            instr.name = Inop;
            instr.type = R_type;
            break;
        default:
            printf("unknown instr opcode:%x\n", opcode);
            printf("instruction addr:%llx val:%x\n", instr.addr, instr.ival);
            fflush(stdout);
            ASSERT(false);
            break;
    }

    switch (instr.type){
        case R_type:
            sA = maskInstr(T_RS1, instr.ival);
            sB = maskInstr(T_RS2, instr.ival);
            vA = registers[sA];
            vB = registers[sB];
            break;
        case I_type:
            sA = maskInstr(4, instr.ival);
            sB = 0;
            vA = registers[sA];
            imm = maskInstr(T_IMMI, instr.ival);
            break;
        case S_type:
            imm = maskInstr(T_IMMS, instr.ival);
            sA = maskInstr(T_RS1, instr.ival);
            sB = maskInstr(T_RS2, instr.ival);
            vA = registers[sA];
            vB = registers[sB];
            break;
        case SB_type:
            imm = maskInstr(T_IMMSB, instr.ival);
            sA = maskInstr(T_RS1, instr.ival);
            sB = maskInstr(T_RS2, instr.ival);
            vA = registers[sA];
            vB = registers[sB];
            break;
        case U_type:
            imm = maskInstr(T_IMMU, instr.ival);
            break;
        case UJ_type: 
            imm = maskInstr(T_IMMUJ, instr.ival);
            break;

        default:
            printf("unknown instruction type!\n");
            ASSERT(false);
    }


    /*deal with dstE and dstM*/
    int64_t dE = 0;
    int64_t dM = 0;
    if((instr.type == R_type || instr.type == I_type ||                      
            instr.type == U_type || instr.type == UJ_type)){
        int64_t rd = maskInstr(T_RD, instr.ival);
        switch (instr.name){
            case Ilb:
            case Ilbu:
            case Ilh:
            case Ilhu:
            case Ilw:
            case Ilwu:
            case Ild:
                dM = rd;
                break;
            default:
                dE = rd;
                break;
        }
    }

    if(instr.name == Iecall){
        FReg.stall = true;
        DRegO.bubble = true;
    }

    ERegO.instr = instr; 
    ERegO.srcA = sA;
    ERegO.srcB = sB;
    ERegO.valA = vA;
    ERegO.valB = vB;  
    ERegO.dstE = dE;
    ERegO.dstM = dM;
    ERegO.imm = imm;
    ERegO.stall = false;
    ERegO.bubble = false;
}

void 
Machine::Execute(){
    MRegO.bubble = false;
    MRegO.stall = false;
    if(EReg.bubble){
        MRegO.stall = false;
        MRegO.bubble = true;
        return;
    }
    if(EReg.stall){
        MRegO.stall = false;
        MRegO.bubble = true;
        return;
    }
    Instruction instr = EReg.instr;
    int64_t sA = EReg.srcA;
    int64_t sb = EReg.srcB;
    int64_t vA = EReg.valA;
    int64_t vB = EReg.valB;
    int64_t imm = EReg.imm;
    int64_t dE = 0;
    int64_t dM = 0;
    int64_t vE = 0;
    int64_t vC = 0; /*for updating predPC*/

    uint32_t tmp = 0;


    switch(instr.name){
        case Iadd:
            vE = vA + vB;
            break;
        case Imul:
            vE = vA * vB;
            break;
        case Isub:
            vE = vA - vB;
            break;
        case Isll:
            vE = vA << vB;
            break;
        case Imulh:
            vE =  (int64_t)( ((__int128_t)vA * (__int128_t)vB) >> 64);
            break;
        case Islt:
            vE = (vA < vB) ? 1 : 0;
            break;
        case Isltu:
            vE = ((uint64_t)vA < (uint64_t)vB) ? 1 : 0;
            break;
        case Ixor:
            vE = vA ^ vB;
            break;
        case Idiv:
            vE = vA / vB;
            break;
        case Isrl:
            vE = (int64_t)((uint64_t)vA >> vB);
            break;
        case Isra:
            vE = vA >> vB;
            break;
        case Ior:
            vE = vA | vB;
            break;
        case Irem:
            vE = vA % vB;
            break;
        case Iand:
            vE = vA & vB;
            break;
        case Ilb:
        case Ilbu:
            vE = vA + imm;
            break;
        case Ilh:
        case Ilhu:
            vE = vA + imm;
            break;
        case Ilw:
        case Ilwu:
            vE = vA + imm;
            break;
        case Ild:
            vE = vA + imm;
            break;
        case Iaddi:
            vE = vA + imm;
            break;
        case Islli:
            vE = vA << (imm & 0x3f);
            break;
    
        case Islti:
            vE = (vA < imm) ? 1 : 0;
            break;
        case Isltiu:
            vE = ((uint64_t)vA < (uint64_t)imm) ? 1 : 0;
            break;
        case Ixori:
            vE = vA ^ imm;
            break;
        case Isrli:
            vE = (int64_t)((uint64_t)vA >> (imm & 0x3f));
            break;

        case Israi:
            vE = vA >> (imm & 0x3f);
            break;
        case Iori:
            vE = vA | imm;
            break;
        case Iandi:
            vE = vA & imm;
            break;
        /*jalr needs to set two bubble to wash away wrong instructions*/
        case Ijalr:                            
            vE = instr.addr + 4;
            vC = (vA + imm) & (-1ll ^ 0x1);
            predPC = vC;
            FReg.stall = false;
            FReg.bubble = false;
            DRegO.bubble = true;
            DRegO.stall = false;
            DReg.bubble = true;
            DReg.stall = false;
            ERegO.bubble = true;
            ERegO.stall = false;
            machineStats.controlHazard ++;
            break;
        case Iecall:
            break;
        case Isb:
            vE = vA + imm;
            break;
        case Ish:
            vE = vA + imm;
            break;
        case Isw:
            vE = vA + imm;
            break;
        case Isd:
            vE = vA + imm;
            break;
        case Ibeq:
            vE = (vA == vB);
            vC = instr.addr + imm;
            break;
        case Ibne:
            vE = (vA != vB);
            vC = instr.addr + imm;
            break;
        case Iblt:
            vE = (vA < vB);
            vC = instr.addr + imm;
            break;
        case Ibge:
            vE = (vA >= vB);
            vC = instr.addr + imm;
            break;
        case Ibltu:
            vE = ((uint64_t)vA < (uint64_t)vB);
            vC = instr.addr + imm;
            break;
        case Ibgeu:
            vE = ((uint64_t)vA >= (uint64_t)vB);
            vC = instr.addr + imm;
            break;
        case Ijal:
            vE = instr.addr + 4;
            break;

        case Isrliw:
            vE = (int64_t)((uint32_t)vA >> (imm & 0x3f));
            break;
        case Islliw:
            vE = (uint32_t)(vA << (imm & 0x1f));
            break;
        case Iaddiw:
            tmp = (uint32_t)(vA + imm);
            vE = sig64ext(31, tmp);
            break;    
        case Israiw:
            vE = (int64_t)((int32_t)vA >> (imm & 0x1f));
            break;
        case Iaddw:
            vE = (int64_t)((int32_t)vA + (int32_t)vB);
            break;
        case Isubw:
            vE = (int64_t)((int32_t)vA + (int32_t)vB);
            break;
        case Isllw:
            vE = (int64_t)((int32_t)vA << (int32_t)vB);
            break;
        case Isrlw:
            vE = (int64_t)((uint32_t)vA << (uint32_t)vB);
            break;
        case Israw:
            vE = (int64_t)((int32_t)vA << (int32_t)vB);
            break;
        case Iauipc:
            vE = instr.addr + imm;
            break;
        case Ilui:
            vE = imm;
            break;
        case Inop:
            break;
        default:
            ASSERT(false);
            break;
    }

    /*deal with wrong branch prediction*/
    if(instr.type == SB_type){
        if(EReg.predJ != vE){
            predPC = vE ? vC : (instr.addr + 4);

            FReg.bubble = false;
            FReg.stall = false;

            DRegO.bubble = true;
            DRegO.stall = false;
            DReg.bubble = true;
            DReg.stall = false;

            ERegO.bubble = true;
            ERegO.stall = false;

            machineStats.misPrediction ++;
            machineStats.controlHazard ++;
        }
        else
            machineStats.sucPrediction ++;

        MyPred.update(EReg.predJ == vE, EReg.instr.addr);
    }

    /*data hazard ---- fowarding*/
    if(EReg.dstE != 0){
        if(EReg.dstE == ERegO.srcA){
            ERegO.valA = vE;
            forwardA = true;

            if(debug)
                printf("Execute data forwardA:%d val:%llx\n", EReg.dstE, vE);

            machineStats.dataHazard ++;
        }
        if(EReg.dstE == ERegO.srcB){
            ERegO.valB = vE;
            forwardB = true;

            if(debug)
                printf("Execute data forwardB:%d val:%llx\n", EReg.dstE, vE);

            machineStats.dataHazard ++;
        }
    }

    /*-----------------------------------------------
      load-use hazard 
      can't get value needed by next instruction until phase MemStage.
    -------------------------------------------------*/
    if(EReg.dstM != 0 ){
        if(ERegO.srcA == EReg.dstM || ERegO.srcB == EReg.dstM){
            FReg.stall = true;
            FReg.bubble = false;

            DReg.stall = true;
            DReg.bubble = false;

            ERegO.bubble = true;
            ERegO.stall = false;

            if(debug){
                printf("\nload-use hazard\n");
                printf("instr:%x dstM:%lld\n", instr.ival, EReg.dstM);
            }

            machineStats.loadUseHazard ++;
        }
    }

    if(EReg.instr.name == Iecall){
        FReg.stall = true;
        DReg.stall = true;
        ERegO.bubble = true;
    }

    if(instrPfm[EReg.instr.name] > TicksPerCycle)
        TicksPerCycle = instrPfm[EReg.instr.name];

    MRegO = EReg;
    MRegO.valE = vE;
    MRegO.valC = vC;
}

void
Machine::MemStage(){
    WRegO.bubble = false;
    WRegO.stall = false;
    if(MReg.bubble){
        WRegO.stall = false;
        WRegO.bubble = true;
        return;
    }
    if(MReg.stall){
        WRegO.stall = false;
        WRegO.bubble = true;
        return;
    }

    Instruction instr = MReg.instr;
    int64_t sA = MReg.srcA;
    int64_t sb = MReg.srcB;
    int64_t vA = MReg.valA;
    int64_t vB = MReg.valB;
    int64_t vE = MReg.valE;
    int64_t imm = MReg.imm;

    int64_t vM = 0;
    int64_t dE = 0;
    int64_t dM = 0;


    switch(instr.name){
        case Ilb:
            readBytes(translateAddr(vE), 1, &vM);
            vM = (int64_t)((int8_t)vM);
            break;
        case Ilh:
            readBytes(translateAddr(vE), 2, &vM);
            vM = (int64_t)((int16_t)vM);
            break;
        case Ilw:
            readBytes(translateAddr(vE), 4, &vM);
            vM = (int64_t)((int32_t)vM);
            break;
        case Ild:
            readBytes(translateAddr(vE), 8, &vM);
            vM = (int64_t)((int64_t)vM);
            break;
        case Ilbu:
            readBytes(translateAddr(vE), 1, &vM);
            break;
        case Ilhu:
            readBytes(translateAddr(vE), 2, &vM);
            break;
        case Ilwu:
            readBytes(translateAddr(vE), 4, &vM);
            break;
        case Isb:
            writeBytes(translateAddr(vE), 1, &vB);
            break;
        case Ish:
            writeBytes(translateAddr(vE), 2, &vB);
            break;
        case Isw:
            writeBytes(translateAddr(vE), 4, &vB);
            break;
        case Isd:
            writeBytes(translateAddr(vE), 8, &vB);
            break;

    }

    /*data hazard*/
    if(MReg.dstE != 0){
        if(ERegO.srcA == MReg.dstE && forwardA == false){
            ERegO.valA = vE;
            forwardA = true;
            if(debug)
                printf("MemStage data forwardA:%d val:%llx\n", MReg.dstE, vE);

            machineStats.dataHazard ++;
        }
        if(ERegO.srcB == MReg.dstE && forwardB == false){
            ERegO.valB = vE;
            forwardB = true;
            if(debug)
                printf("MemStage data forwardB:%d val:%llx\n", MReg.dstE, vE);

            machineStats.dataHazard ++;
        }
        
    }

    /*
        load-use hazard 
        data forwarding
    */
    if(MReg.dstM != 0){
        if(ERegO.srcA == MReg.dstM){
            ERegO.valA = vM;
            forwardA = true;
            if(debug)
                printf("MemStage load-use forwardA:%d val:%llx\n", MReg.dstM, vM);

            machineStats.dataHazard ++;
        }
        if(ERegO.srcB == MReg.dstM){
            ERegO.valB = vM;
            forwardB = true;
            if(debug)
                printf("MemStage load-use forwardB:%d val:%llx\n", MReg.dstM, vM);

            machineStats.dataHazard ++;
        }
    }

    if(MReg.instr.name == Iecall){
        FReg.stall = true;
        DReg.stall = true;
        EReg.stall = true;
        MRegO.bubble = true;
    }
/*
    switch (MReg.instr.name){
            case Ilb:
            case Ilbu:
            case Ilh:
            case Ilhu:
            case Ilw:
            case Ilwu:
            case Ild:
                if(memPfm[RMEM] > TicksPerCycle)
                    TicksPerCycle = memPfm[RMEM];
                break;
            case Isb:
            case Ish:
            case Isw:
            case Isd:
                if(memPfm[WMEM] > TicksPerCycle)
                    TicksPerCycle = memPfm[WMEM];
                break;
            default:
                break;
        }*/

    WRegO = MReg;
    WRegO.valM = vM;
}

void
Machine::Writeback(){
    if(WReg.bubble | WReg.stall){
        return;
    }

    Instruction instr = WReg.instr;   
    int64_t vM = WReg.valM;
    int64_t vE = WReg.valE;
    int64_t dE = WReg.dstE;
    int64_t dM = WReg.dstM;

    if(instr.name == Iecall){
        machineStats.ecallNum ++;
        syscall();
        return;
    }

    if(dM != 0)
        registers[dM] = vM;
    if(dE != 0)
        registers[dE] = vE;

    /*data hazard*/
    if(dE != 0){
        if(ERegO.srcA == dE && forwardA == false){
            ERegO.valA = vE;
            if(debug)
                printf("Writeback data forwardA:%d val:%llx\n", dE, vE);
            machineStats.dataHazard ++;
        }
        /*no need to set forward*/
        if(ERegO.srcB == dE && forwardB == false){
            ERegO.valB = vE;
            if(debug)
                printf("Writeback data forwardB:%d val:%llx\n", dE, vE);
            machineStats.dataHazard ++;
        }
    }

    /*load use hazard*/
    if(dM != 0){
        if(ERegO.srcA == dM && forwardA == false){
            ERegO.valA = vM;
            if(debug)
                printf("Writeback load-use forwardA:%d val:%llx\n", dM, vM);

            machineStats.dataHazard ++;
        }
        if(ERegO.srcB == dM && forwardB == false){
            ERegO.valB = vM;
            if(debug)
                printf("Writeback load-use forwardB:%d val:%llx\n", dM, vM);

            machineStats.dataHazard ++;
        }
    }

    machineStats.instrCnt ++;

}

void
Machine::syscall(){
    int64_t a7 = registers[A7Reg];
    int64_t a0 = registers[A0Reg];
    char c;
    int len = 0;
    int x = 0;
    switch(a7){
        case 88:
            printf("program syscall sleep %lld\n", a0);
            break;
        case 89:
            printf("program syscall printf int\n");
            printf("%lld\n", a0);
            break;
        case 90:
            printf("program syscall printf char\n");
            printf("%c\n", (char)a0);
            break;
        case 91:
            printf("program syscall printf str\n");

            while(true){
                readBytes(translateAddr(a0), 1, (void *)c);
                if(c == '\0')
                    break;

                printf("%c", c);
                len ++;

                if(len > 100){
                    printf("\nprintf str too long!\n");
                    break;
                }
            }
            printf("\n");
            break;
        case 92:
            printf("program syscall scanf int\n");
            printf("please type an integer\n");
            scanf("%d", &x);
            registers[A0Reg] = x;
            break; 
        case 93:    /*exit(0)*/
            printf("program syscall exit with exit code:%d\n", a0);
            Halt();
        break;
    }
}

