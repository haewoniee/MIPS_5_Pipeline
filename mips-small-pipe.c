/*
 *     $Author: yeung $
 *     $Modifier: Haewon Han 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mips-small-pipe.h"


/************************************************************/
int
main(int argc, char *argv[])
{
  short i, j;
  short statusNum;
  char line[MAXLINELENGTH];
  short lowMem, highMem;
  state_t state;
  FILE *filePtr;


  if (argc != 2) {
    printf("error: usage: %s <machine-code file>\n", argv[0]);
    exit(1);
  }

  memset(&state, 0, sizeof(state_t));

  state.pc=state.cycles=0;
  state.IFID.instr = state.IDEX.instr = state.EXMEM.instr =
    state.MEMWB.instr = state.WBEND.instr = NOPINSTRUCTION; /* nop */

  /* read machine-code file into instruction/data memory (starting at address 0) */

  filePtr = fopen(argv[1], "r");
  if (filePtr == NULL) {
    printf("error: can't open file %s\n", argv[1]);
    perror("fopen");
    exit(1);
  }

  for (state.numMemory=0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
       state.numMemory++) {
    if (sscanf(line, "%x", &state.dataMem[state.numMemory]) != 1) {
      printf("error in reading address %d\n", state.numMemory);
      exit(1);
    }
    state.instrMem[state.numMemory] = state.dataMem[state.numMemory];
    printf("memory[%d]=%x\n", 
	   state.numMemory, state.dataMem[state.numMemory]);
  }

  printf("%d memory words\n", state.numMemory);

  printf("\tinstruction memory:\n");
  for (i=0; i<state.numMemory; i++) {
    printf("\t\tinstrMem[ %d ] = ", i);
    printInstruction(state.instrMem[i]);
  }

  run(&state);
}
/************************************************************/

/************************************************************/
void
run(Pstate state)
{
  state_t new;
  int readRegA;
  int readRegB;
  short i;
  int haltflag = 0;

  
  memset(&new, 0, sizeof(state_t));
  int temp=0;
  while (1) {
    printState(state);
    /* copy everything so all we have to do is make changes.
       (this is primarily for the memory and reg arrays) */
    memcpy(&new, state, sizeof(state_t));
    /* --------------------- IF state --------------------- */
    // Fetch Instruction
    // if not branch inst, increment pcPlus1
    i = new.pc;
 
    new.IFID.instr = new.instrMem[i >> 2];

    //increase pc by 4 already
    new.pc += 4;
    new.IFID.pcPlus1 = new.pc;

    if (opcode(new.IFID.instr) == BEQZ_OP) {

      //Test immediate value
      int im = offset(new.IFID.instr);
      //if Immediate positive, go on
      //else if immediate negative, take branch
      if (im < 0)
      {
        new.pc = offset(new.IFID.instr) + new.IFID.pcPlus1;
      }
      else
      {
        new.pc = new.pc;
      }
    }
    /* --------------------- ID state --------------------- */
    //If there is a fetched instruction, let's decode it
    //Read registers and store into IDEX struct
    new.IDEX.instr = state->IFID.instr;
    // R type - REG_REG_OP & NOPINSTRUCTION
    new.IDEX.pcPlus1 = state->IFID.pcPlus1;
    int src1, src2;
    src1 = field_r1(new.IDEX.instr);
    src2 = field_r2(new.IDEX.instr);
    readRegA = new.reg[src1];
    readRegB = new.reg[src2];

    if (new.IDEX.instr == NOPINSTRUCTION || opcode(new.IDEX.instr) == HALT_OP)
    {
        new.IDEX.offset = offset(new.IDEX.instr);
        src1 = -1;
        src2 = -1;
        readRegA = 0;
        readRegB = 0;
    }
    else
    {
      int opc = opcode(new.IDEX.instr);
      if (opc == REG_REG_OP)
      {
        new.IDEX.offset = offset(new.IDEX.instr);
      }
      else if (opc == ADDI_OP || opc == BEQZ_OP)
      {
        new.IDEX.offset = offset(new.IDEX.instr);
        src2 = -1;
      }
      else if (opc == SW_OP)
      {
        new.IDEX.offset = offset(new.IDEX.instr);
      }
      else if (opc == LW_OP)
      {
        new.IDEX.offset = offset(new.IDEX.instr);
        src2 = -1;
      }
      else if (opc == HALT_OP)
      {
        //new.IDEX.offset = 0;
        src1 = -1;
        src2 = -1;
      }
    }
    new.IDEX.readRegA = readRegA;
    new.IDEX.readRegB = readRegB;

    //TODO! DEPENDENCY CHECK. IF DEPENDENT, BUBBLE IT
    if (opcode(state->IDEX.instr) == LW_OP)
    {
      if (src1 == field_r2(state->IDEX.instr) || src2 == field_r2(state->IDEX.instr))
      {
        //Bubbling
        new.IDEX.instr = NOPINSTRUCTION;
        new.IDEX.readRegA = 0;
        new.IDEX.readRegB = 0;
        new.IDEX.pcPlus1 = 0;
        new.IDEX.offset = offset(new.IDEX.instr);
        //replacing the prev one
        new.IFID.instr = state->IFID.instr;
        new.IFID.pcPlus1 = state->IFID.pcPlus1;
        new.pc -= 4;
      }
    }

    /* --------------------- EX state --------------------- */
    // if there is a decoded instruction, let's execute it
    new.EXMEM.instr = state->IDEX.instr;

    // I type
    if (new.EXMEM.instr == NOPINSTRUCTION || opcode(new.EXMEM.instr) == HALT_OP)
    {
      new.EXMEM.aluResult = 0;
      new.EXMEM.readRegB = 0;
    }
    else
    {
      int opc = opcode(new.EXMEM.instr);
      if (opc == LW_OP )
      {
        // if there is dependency of the base reg, then read it from pipeline
        int val = check_dependency(field_r1(state->IDEX.instr),2,state);
        if (val != -1)
          //dependent case
        {
          new.EXMEM.aluResult = val + state->IDEX.offset;
        }
        else
          //independent case
        {
          new.EXMEM.aluResult = new.reg[field_r1(state->IDEX.instr)] + state->IDEX.offset;
        }

        new.EXMEM.readRegB = new.reg[field_r2(new.EXMEM.instr)];
      }
      else if (opc == SW_OP)
      {
        int val_toStore = check_dependency(field_r2(state->IDEX.instr),2,state);
        if (val_toStore != -1)
          //dependent case
        {
          new.EXMEM.readRegB = val_toStore;
        }
        else
          //independent case
        {
          new.EXMEM.readRegB = new.reg[field_r2(new.EXMEM.instr)];
        }

        int val_reg = check_dependency(field_r1(state->IDEX.instr),2,state);
        if (val_reg != -1)
          //dependent case
        {
          new.EXMEM.aluResult = val_reg + offset(new.EXMEM.instr);
        }
        else
          //independent case
        {
          new.EXMEM.aluResult = new.reg[field_r1(new.EXMEM.instr)] + offset(new.EXMEM.instr);
        }

      }
      else if (opc == ADDI_OP)
      {
        int val = check_dependency(field_r1(state->IDEX.instr),2,state);
        if (val != -1)
          //dependent case
        {
          new.EXMEM.aluResult = val + state->IDEX.offset;
          new.EXMEM.readRegB = new.reg[field_r2(new.EXMEM.instr)];
        }
        else
          //independent case
        {
          new.EXMEM.aluResult = new.reg[field_r1(state->IDEX.instr)] + state->IDEX.offset;
          new.EXMEM.readRegB = new.reg[field_r2(new.EXMEM.instr)];
        }

      }
      else if (opc == BEQZ_OP)
      {
        int branchFlag = 1; //correct;
        //Calculate the actual target address
        new.EXMEM.readRegB = 0;
        int val = check_dependency(field_r1(state->IDEX.instr),2,state);
        if (val == -1)
          //dependent
        {
          val = new.reg[field_r1(state->IDEX.instr)];
        }

        new.EXMEM.aluResult = state->IDEX.pcPlus1 + state->IDEX.offset; 

        if (val == 0)
        {

          //incorrect case1: offset positive & predicted NT
          if (state->IDEX.offset > 0)
          {
            branchFlag = 0;
          }
        }
        else if (val != 0)
        {
          //incorrect case2: offset negative & predicted T
          if (state->IDEX.offset < 0)
          {
            branchFlag = 0;
          }
        }

        //Misprediction case
        if (branchFlag == 0)
        {
          //point to the target address
          new.pc = new.EXMEM.aluResult;

          //void prev instructions fetched
          new.IDEX.instr = NOPINSTRUCTION;
          new.IDEX.readRegA = 0;
          new.IDEX.readRegB = 0;
          new.IDEX.pcPlus1 = 0;
          new.IDEX.offset = offset(new.IDEX.instr);
          //replacing the prev one
          new.IFID.instr = NOPINSTRUCTION;
          new.IFID.pcPlus1 = 0;
        }
        

      }
      else if (opc == REG_REG_OP)
      {
        int val1 = check_dependency(field_r1(state->IDEX.instr),2,state);
        int val2 = check_dependency(field_r2(state->IDEX.instr),2,state);
        if (val1 == -1)
        {
          int inst1 = field_r1(new.EXMEM.instr);
          val1 = new.reg[field_r1(new.EXMEM.instr)];
        }
        if (val2 == -1)
        {
          int inst2 = field_r2(new.EXMEM.instr);
          val2 = new.reg[field_r2(new.EXMEM.instr)];
        }
        
        new.EXMEM.readRegB = val2;

        int f = func(state->IDEX.instr);
        if (f == ADD_FUNC) {
            new.EXMEM.aluResult = val1 + val2;
        } else if (f == SLL_FUNC) {
           new.EXMEM.aluResult = val1 << val2;
        } else if (f == SRL_FUNC) {
            new.EXMEM.aluResult = ( (unsigned int) val1) >> val2;
        } else if (f == SUB_FUNC) {
            new.EXMEM.aluResult = val1 - val2;
        } else if (f == AND_FUNC) {
            new.EXMEM.aluResult = val1 & val2;
        } else if (f == OR_FUNC) {
            new.EXMEM.aluResult = val1 | val2;
        }
      }
    }

    /* --------------------- MEM state --------------------- */
    //if there is an executed instruction, let's run mem state
    new.MEMWB.instr = state->EXMEM.instr;
    if (new.MEMWB.instr == NOPINSTRUCTION)
    {
      new.MEMWB.writeData = 0;
    }
    else
    {
      int opc = opcode(new.MEMWB.instr);
      if (opc == HALT_OP)
      {
        new.MEMWB.writeData = 0;
        haltflag = 1;
      }
      else if (opc == LW_OP || opc == SW_OP)
      {
        if (opc == SW_OP)
        {
          new.dataMem[(state->EXMEM.aluResult >> 2)] = state->EXMEM.readRegB;
          new.MEMWB.writeData = state->EXMEM.readRegB;
        }
        else
        {
          new.MEMWB.writeData = new.dataMem[state->EXMEM.aluResult >> 2];
        }
      }
      else
      {
        new.MEMWB.writeData = state->EXMEM.aluResult;
      }
    }


    /* --------------------- WB state ---------------------- */
    //if there is an MEM instruction, let's run WB state
    new.WBEND.instr = state->MEMWB.instr;
    new.WBEND.writeData = state->MEMWB.writeData;
    if (new.WBEND.instr == NOPINSTRUCTION)
    {
      new.WBEND.writeData = 0;
    }
    else
    {
      int opc = opcode(new.WBEND.instr);
      if (opc == LW_OP || opc == ADDI_OP)
      {
        new.reg[field_r2(new.WBEND.instr)] = state->MEMWB.writeData;
        new.WBEND.writeData = state->MEMWB.writeData;
      }
      else if (opc == REG_REG_OP)
      {
        new.reg[field_r3(new.WBEND.instr)] = state->MEMWB.writeData;
        new.WBEND.writeData = state->MEMWB.writeData;
      }
    }

    /* --------------------- end state --------------------- */
    //increment cycle
    new.cycles += 1;
    memcpy(state, &new, sizeof(state_t));

    if (haltflag == 1)
    {
      printState(state);
      printf("machine halted\n");
      printf("total of %d cycles executed\n", state->cycles);
      exit(0);
    }

    temp += 1;
    /* transfer new state into current state */
  }
}

/************************************************************/

int
check_dependency(int srcReg, int stageNum, Pstate state)
{
  int dest = -1;
  if (stageNum == 2)
  {
    //Request from EX Stage - need to check EXMEM stage
    if (state->EXMEM.instr == NOPINSTRUCTION)
    {
      dest = -1;
    }
    else
    {
      if (opcode(state->EXMEM.instr) == ADDI_OP || opcode(state->EXMEM.instr) == LW_OP)
      {
        dest = field_r2(state->EXMEM.instr);
      }
      else if (opcode(state->EXMEM.instr) == REG_REG_OP)
      {
        dest = field_r3(state->EXMEM.instr);
      }
    }
    if (dest == srcReg)
    {
        return state->EXMEM.aluResult;
    }


    //Request from EX Stage - need to check MEMWB stage
    if (state->MEMWB.instr == NOPINSTRUCTION)
    {
      dest = -1;
    }
    else
    {
      if (opcode(state->MEMWB.instr) == ADDI_OP || opcode(state->MEMWB.instr) == LW_OP)
      {
        dest = field_r2(state->MEMWB.instr);
      }
      else if (opcode(state->MEMWB.instr) == REG_REG_OP)
      {
        dest = field_r3(state->MEMWB.instr);
      }
    }
    if (dest == srcReg)
    {
      return state->MEMWB.writeData;
    }


    //Request from EX Stage - need to check WBEND stage
    if (state->WBEND.instr == NOPINSTRUCTION)
    {
      dest = -1;
    }
    else
    {
      if (opcode(state->WBEND.instr) == ADDI_OP || opcode(state->WBEND.instr) == LW_OP)
      {
        dest = field_r2(state->WBEND.instr);
      }
      else if (opcode(state->WBEND.instr) == REG_REG_OP)
      {
        dest = field_r3(state->WBEND.instr);
      }
    }
    if (dest == srcReg)
    {
      return state->WBEND.writeData;
    }    else 
    {
      dest = -1;
    }

  }
  return dest;
}

/************************************************************/
int
opcode(int instruction)
{
  return (instruction >> OP_SHIFT) & OP_MASK;
}
/************************************************************/

/************************************************************/
int
func(int instruction)
{
  return (instruction & FUNC_MASK);
}
/************************************************************/

/************************************************************/
int
field_r1(int instruction)
{
  return (instruction >> R1_SHIFT) & REG_MASK;
}
/************************************************************/

/************************************************************/
int
field_r2(int instruction)
{
  return (instruction >> R2_SHIFT) & REG_MASK;
}
/************************************************************/

/************************************************************/
int
field_r3(int instruction)
{
  return (instruction >> R3_SHIFT) & REG_MASK;
}
/************************************************************/

/************************************************************/
int
field_imm(int instruction)
{
  return (instruction & IMMEDIATE_MASK);
}
/************************************************************/

/************************************************************/
int
offset(int instruction)
{
  /* only used for lw, sw, beqz */
  return convertNum(field_imm(instruction));
}
/************************************************************/

/************************************************************/
int
convertNum(int num)
{
  /* convert a 16 bit number into a 32-bit Sun number */
  if (num & 0x8000) {
    num -= 65536;
  }
  return(num);
}
/************************************************************/

/************************************************************/
void
printState(Pstate state)
{
  short i;
  printf("@@@\nstate before cycle %d starts\n", state->cycles);
  printf("\tpc %d\n", state->pc);

  printf("\tdata memory:\n");
  for (i=0; i<state->numMemory; i++) {
    printf("\t\tdataMem[ %d ] %d\n", 
	   i, state->dataMem[i]);
  }
  printf("\tregisters:\n");
  for (i=0; i<NUMREGS; i++) {
    printf("\t\treg[ %d ] %d\n", 
	   i, state->reg[i]);
  }
  printf("\tIFID:\n");
  printf("\t\tinstruction ");
  printInstruction(state->IFID.instr);
  printf("\t\tpcPlus1 %d\n", state->IFID.pcPlus1);
  printf("\tIDEX:\n");
  printf("\t\tinstruction ");
  printInstruction(state->IDEX.instr);
  printf("\t\tpcPlus1 %d\n", state->IDEX.pcPlus1);
  printf("\t\treadRegA %d\n", state->IDEX.readRegA);
  printf("\t\treadRegB %d\n", state->IDEX.readRegB);
  printf("\t\toffset %d\n", state->IDEX.offset);
  printf("\tEXMEM:\n");
  printf("\t\tinstruction ");
  printInstruction(state->EXMEM.instr);
  printf("\t\taluResult %d\n", state->EXMEM.aluResult);
  printf("\t\treadRegB %d\n", state->EXMEM.readRegB);
  printf("\tMEMWB:\n");
  printf("\t\tinstruction ");
  printInstruction(state->MEMWB.instr);
  printf("\t\twriteData %d\n", state->MEMWB.writeData);
  printf("\tWBEND:\n");
  printf("\t\tinstruction ");
  printInstruction(state->WBEND.instr);
  printf("\t\twriteData %d\n", state->WBEND.writeData);
}
/************************************************************/

/************************************************************/
void
printInstruction(int instr)
{

  if (opcode(instr) == REG_REG_OP) {

    if (func(instr) == ADD_FUNC) {
      print_rtype(instr, "add");
    } else if (func(instr) == SLL_FUNC) {
      print_rtype(instr, "sll");
    } else if (func(instr) == SRL_FUNC) {
      print_rtype(instr, "srl");
    } else if (func(instr) == SUB_FUNC) {
      print_rtype(instr, "sub");
    } else if (func(instr) == AND_FUNC) {
      print_rtype(instr, "and");
    } else if (func(instr) == OR_FUNC) {
      print_rtype(instr, "or");
    } else {
      printf("data: %d\n", instr);
    }

  } else if (opcode(instr) == ADDI_OP) {
    print_itype(instr, "addi");
  } else if (opcode(instr) == LW_OP) {
    print_itype(instr, "lw");
  } else if (opcode(instr) == SW_OP) {
    print_itype(instr, "sw");
  } else if (opcode(instr) == BEQZ_OP) {
    print_itype(instr, "beqz");
  } else if (opcode(instr) == HALT_OP) {
    printf("halt\n");
  } else {
    printf("data: %d\n", instr);
  }
}
/************************************************************/

/************************************************************/
void
print_rtype(int instr, char *name)
{
  printf("%s %d %d %d\n", 
	 name, field_r3(instr), field_r1(instr), field_r2(instr));
}
/************************************************************/

/************************************************************/
void
print_itype(int instr, char *name)
{
  printf("%s %d %d %d\n", 
	 name, field_r2(instr), field_r1(instr), offset(instr));
}
/************************************************************/
