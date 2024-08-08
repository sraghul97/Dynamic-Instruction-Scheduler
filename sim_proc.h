#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;

#ifndef SIM_PROC_H
#define SIM_PROC_H

enum
{
    Fetch = 0,
    Decode,
    Rename,
    RegisterRead,
    Dispatch,
    Issue,
    Execute,
    WriteBack,
    Retire
};

struct InstructionData
{
    uint32_t StartCycle[9], TotalCycles[9];
    uint32_t RS1, RS2, RS1initial, RS2initial, RD, RDinitial, PC, ExecutionStageCount, ExecutionStageCountTrace;
    bool Rs1Ready, Rs2Ready, RS1RobFlag, RS2RobFlag, ValidFlag;

    void Initialize()
    {
        for (uint32_t PipeLineStage = 0; PipeLineStage < 9; PipeLineStage++)
        {
            StartCycle[PipeLineStage] = 0;
            TotalCycles[PipeLineStage] = 0;
        }
        Rs1Ready = Rs2Ready = RS1RobFlag = RS2RobFlag = ValidFlag = false;
        RS1 = RS2 = RS1initial = RS2initial = RD, RDinitial = PC = ExecutionStageCount = ExecutionStageCountTrace = 0;
    }
};

struct Register
{
    uint32_t BundleSize;
    uint32_t RegSizeCount;
    InstructionData *Instr;

    void Initialize(uint32_t RegSizeCount_)
    {
        RegSizeCount = RegSizeCount_;
        Instr = new InstructionData[RegSizeCount];
        Initialize();
    }

    void Initialize()
    {
        BundleSize = 0;
        for (uint32_t i = 0; i < RegSizeCount; i++)
            Instr[i].Initialize();
    }
};

struct ROBTableLine
{
    uint32_t Destination;
    bool Ready, Valid;
    uint32_t ProgramCounter;

    void Initialize()
    {
        Destination = 0;
        Ready = false;
        Valid = false;
        ProgramCounter = 0;
    }

    void SetDestination(uint32_t Destination_) { Destination = Destination_; }
    void SetReady(bool Ready_) { Ready = Ready_; }
    void SetValid(bool Valid_) { Valid = Valid_; }
    void SetProgramCounter(uint32_t ProgramCounter_) { ProgramCounter = ProgramCounter_; }

    bool GetReady() { return Ready; }
    bool GetValid() { return Valid; }
    uint32_t GetDestination() { return Destination; }
    uint32_t GetProgramCounter() { return ProgramCounter; }
};

struct RMTTableLine
{
    bool Valid;
    uint32_t RobTag;
    void Initialize()
    {
        Valid = false;
        RobTag = 0;
    }

    void SetValid(bool Valid_) { Valid = Valid_; }
    void SetRobTag(uint32_t RobTag_) { RobTag = RobTag_; }

    bool GetValid() { return Valid; }
    uint32_t GetRobTag() { return RobTag; }
};

struct RmtTable
{
    RMTTableLine *RmtLine;
    uint16_t RmtSize;
    void Initialize(uint32_t RmtSize_ = 67)
    {
        RmtSize = RmtSize_;
        RmtLine = new RMTTableLine[RmtSize];
        for (uint32_t CurrentRmtLine = 0; CurrentRmtLine < RmtSize_; CurrentRmtLine++)
            RmtLine[CurrentRmtLine].Initialize();
    }
};

struct RobTable
{
    ROBTableLine *RobLine;
    uint16_t HeadPointer, TailPointer, RobSize;

    void Initialize(uint32_t RobSize_)
    {
        RobSize = RobSize_;
        RobLine = new ROBTableLine[RobSize];
        Initialize();
    }

    void Initialize()
    {
        HeadPointer = TailPointer = 3;
        for (uint32_t CurrentRobLine = 0; CurrentRobLine < RobSize; CurrentRobLine++)
            RobLine[CurrentRobLine].Initialize();
    }
};

#endif
