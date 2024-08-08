#include "sim_proc.h"
FILE *TraceFilePointer; // File handler
char *TraceFileName;    // Variable that holds trace file name;

uint32_t RobSize, IQsize, ExSize, Width, CurrentCycle;
int OpcodeType, Destination, Source1, Source2; // Variables are read from trace file
uint32_t ProgramCounter;                       // Variable holds the pc read from input file
bool EndOfTraceFlag;
// uint32_t RetireCount;

Register DEreg, RNreg, RRreg, DIreg, IQreg, EXECUTEreg, WBreg, RTreg, OUTPUTreg;
RobTable ROB;
RmtTable RMT;
bool DebugFlag = !true;

void InstrDisplay(Register Reg, string str)
{
    if (!DebugFlag)
        return;
    // else if (!(CurrentCycle >= 7230 && CurrentCycle <= 7275))
    // return;
    // uint32_t StartCycle[9], TotalCycles[9];
    // uint32_t RS1, RS2, RD, PC, ExecutionStageCount;
    // bool Rs1Ready, Rs2Ready, RS1RobFlag, RS2RobFlag;
    if (Reg.BundleSize == 0)
    {
        cout << str << "_" << Reg.BundleSize << '\n';
        return;
    }
    cout << '\n'
         << '\n'
         << str << '\t' << Reg.BundleSize << '\n';
    cout << "PC:" << '\t' << '\t' << "RD:" << '\t' << "RS1:" << '\t' << "RS2:" << '\t' << "RS1rd:" << '\t' << "RS2rd:" << '\t' << "RS1rob:" << '\t' << "RS2rob:" << '\t' << "Exec" << '\n';

    for (uint32_t SuperScalarCount = 0; SuperScalarCount < Reg.BundleSize; SuperScalarCount++)
    {
        cout << hex << Reg.Instr[SuperScalarCount].PC << '\t'
             << dec << Reg.Instr[SuperScalarCount].RD << '\t'
             << Reg.Instr[SuperScalarCount].RS1 << '\t' << '\t'
             << Reg.Instr[SuperScalarCount].RS2 << '\t' << '\t'
             << Reg.Instr[SuperScalarCount].Rs1Ready << '\t' << '\t'
             << Reg.Instr[SuperScalarCount].Rs2Ready << '\t' << '\t'
             << Reg.Instr[SuperScalarCount].RS1RobFlag << '\t' << '\t'
             << Reg.Instr[SuperScalarCount].RS2RobFlag << '\t' << '\t'
             << Reg.Instr[SuperScalarCount].ExecutionStageCount << '\t' << Reg.Instr[SuperScalarCount].ExecutionStageCountTrace << '\t';

        for (int i = 0; i < 9; i++)
        {
            cout << "SC" << i << "{" << Reg.Instr[SuperScalarCount].StartCycle[i] << "_" << Reg.Instr[SuperScalarCount].TotalCycles[i] << "}" << '\t';
        }
        cout << '\n';
    }
}

void DisplayOutput()
{
    for (uint32_t OUTregCounter = 0; OUTregCounter < OUTPUTreg.BundleSize; OUTregCounter++)
    {
        cout << OUTregCounter << " fu{" << OUTPUTreg.Instr[OUTregCounter].ExecutionStageCountTrace << "} src{";
        if (OUTPUTreg.Instr[OUTregCounter].RS1initial == 100)
            cout << "-1";
        else
            cout << OUTPUTreg.Instr[OUTregCounter].RS1initial;

        if (OUTPUTreg.Instr[OUTregCounter].RS2initial == 100)
            cout << ",-1";
        else
            cout << "," << OUTPUTreg.Instr[OUTregCounter].RS2initial;

        if (OUTPUTreg.Instr[OUTregCounter].RDinitial == 100)
            cout << "} dst{-1";
        else
            cout << "} dst{" << OUTPUTreg.Instr[OUTregCounter].RDinitial;

        cout << "} FE{" << OUTPUTreg.Instr[OUTregCounter].StartCycle[0] << "," << OUTPUTreg.Instr[OUTregCounter].TotalCycles[0] << "}";
        cout << " DE{" << OUTPUTreg.Instr[OUTregCounter].StartCycle[1] << "," << OUTPUTreg.Instr[OUTregCounter].TotalCycles[1] << "}";
        cout << " RN{" << OUTPUTreg.Instr[OUTregCounter].StartCycle[2] << "," << OUTPUTreg.Instr[OUTregCounter].TotalCycles[2] << "}";
        cout << " RR{" << OUTPUTreg.Instr[OUTregCounter].StartCycle[3] << "," << OUTPUTreg.Instr[OUTregCounter].TotalCycles[3] << "}";
        cout << " DI{" << OUTPUTreg.Instr[OUTregCounter].StartCycle[4] << "," << OUTPUTreg.Instr[OUTregCounter].TotalCycles[4] << "}";
        cout << " IS{" << OUTPUTreg.Instr[OUTregCounter].StartCycle[5] << "," << OUTPUTreg.Instr[OUTregCounter].TotalCycles[5] << "}";
        cout << " EX{" << OUTPUTreg.Instr[OUTregCounter].StartCycle[6] << "," << OUTPUTreg.Instr[OUTregCounter].TotalCycles[6] << "}";
        cout << " WB{" << OUTPUTreg.Instr[OUTregCounter].StartCycle[7] << "," << OUTPUTreg.Instr[OUTregCounter].TotalCycles[7] << "}";
        cout << " RT{" << OUTPUTreg.Instr[OUTregCounter].StartCycle[8] << "," << OUTPUTreg.Instr[OUTregCounter].TotalCycles[8] << "}" << '\n';
    }

    cout << "# === Simulator Command =========" << '\n';
    cout << "#./ sim " << RobSize << " " << IQsize << " " << Width << " " << TraceFileName << '\n';
    cout << "#=== Processor Configuration ===" << '\n';
    cout << "#ROB_SIZE = " << RobSize << '\n';
    cout << "#IQ_SIZE = " << IQsize << '\n';
    cout << "#WIDTH = " << Width << '\n';
    cout << "#=== Simulation Results ========" << '\n';
    cout << "#Dynamic Instruction Count = " << OUTPUTreg.BundleSize << '\n';
    cout << "#Cycles = " << CurrentCycle << '\n';
    cout << "#Instructions Per Cycle(IPC) = " << fixed << setprecision(2) << double(OUTPUTreg.BundleSize) / double(CurrentCycle) << '\n';
}

bool AdvanceCycle()
{
    if (DebugFlag)
        cout << "DE_" << '\t' << DEreg.BundleSize << '\t' << "RN_" << '\t' << RNreg.BundleSize << '\t' << "RR_" << '\t' << RRreg.BundleSize << '\t' << "DI_" << '\t' << DIreg.BundleSize << '\t' << "IQ_" << '\t' << IQreg.BundleSize << '\t' << "EX_" << '\t' << EXECUTEreg.BundleSize << '\t' << "WB_" << '\t' << WBreg.BundleSize << '\t' << "RT_" << '\t' << RTreg.BundleSize << '\t' << "EOF_" << EndOfTraceFlag << '\n';
    bool SimulationDoneFlag = !((DEreg.BundleSize == 0) & (RNreg.BundleSize == 0) & (RRreg.BundleSize == 0) & (DIreg.BundleSize == 0) & (IQreg.BundleSize == 0) & (EXECUTEreg.BundleSize == 0) & (WBreg.BundleSize == 0) & (RTreg.BundleSize == 0) & (ROB.HeadPointer == ROB.TailPointer) & (!(ROB.RobLine[ROB.HeadPointer].Valid)) & EndOfTraceFlag);
    CurrentCycle++;

    return SimulationDoneFlag;
}

void PipelineFetch(FILE *TraceFilePointer)
{
    // Do nothing if either (1) there are no more instructions in the trace file or (2) DE is not empty (cannot accept a new decode bundle).

    // If there are more instructions in the trace file and if DE is empty (can accept a new decode bundle), then fetch up to WIDTH instructions from the trace file into DE.

    // Fewer than WIDTH instructions will be fetched only if the trace file has fewer than WIDTH instructions left.

    if ((DEreg.BundleSize == 0) && (!EndOfTraceFlag))
    {
        for (uint32_t SuperScalarCount = 0; SuperScalarCount < Width; SuperScalarCount++)
        {
            DEreg.Instr[SuperScalarCount].Initialize();
            EndOfTraceFlag = (fscanf(TraceFilePointer, "%lx %d %d %d %d", &ProgramCounter, &OpcodeType, &Destination, &Source1, &Source2) == EOF);

            if (!EndOfTraceFlag)
            {
                if (DebugFlag)
                    cout << ProgramCounter << " " << OpcodeType << " " << Destination << " " << Source1 << " " << Source2 << " " << EndOfTraceFlag << '\n';
                DEreg.Instr[SuperScalarCount].PC = ProgramCounter;
                DEreg.Instr[SuperScalarCount].StartCycle[Fetch] = CurrentCycle;
                DEreg.Instr[SuperScalarCount].TotalCycles[Fetch] = 1;
                DEreg.Instr[SuperScalarCount].StartCycle[Decode] = CurrentCycle + 1;
                DEreg.Instr[SuperScalarCount].Rs1Ready = DEreg.Instr[SuperScalarCount].Rs2Ready = false;
                DEreg.Instr[SuperScalarCount].RS1RobFlag = DEreg.Instr[SuperScalarCount].RS2RobFlag = false;
                DEreg.Instr[SuperScalarCount].ValidFlag = true;
                DEreg.Instr[SuperScalarCount].ExecutionStageCountTrace = OpcodeType;

                if (Source1 == -1)
                    DEreg.Instr[SuperScalarCount].RS1 = DEreg.Instr[SuperScalarCount].RS1initial = 100;
                else
                    DEreg.Instr[SuperScalarCount].RS1 = DEreg.Instr[SuperScalarCount].RS1initial = Source1;

                if (Source2 == -1)
                    DEreg.Instr[SuperScalarCount].RS2 = DEreg.Instr[SuperScalarCount].RS2initial = 100;
                else
                    DEreg.Instr[SuperScalarCount].RS2 = DEreg.Instr[SuperScalarCount].RS2initial = Source2;

                if (Destination == -1)
                    DEreg.Instr[SuperScalarCount].RD = DEreg.Instr[SuperScalarCount].RDinitial = 100;
                else
                    DEreg.Instr[SuperScalarCount].RD = DEreg.Instr[SuperScalarCount].RDinitial = Destination;

                if (OpcodeType == 0)
                    DEreg.Instr[SuperScalarCount].ExecutionStageCount = 1;
                else if (OpcodeType == 1)
                    DEreg.Instr[SuperScalarCount].ExecutionStageCount = 2;
                else if (OpcodeType == 2)
                    DEreg.Instr[SuperScalarCount].ExecutionStageCount = 5;

                DEreg.BundleSize++;
            }
            else
                break;
        }

        InstrDisplay(DEreg, "Fetch->Decode");
    }
}

void PipelineDecode()
{
    // If DE contains a decode bundle:

    // If RN is not empty (cannot accept a new rename bundle), then do nothing.

    // If RN is empty (can accept a new rename bundle), then advance the decode bundle from DE to RN.

    if ((RNreg.BundleSize == 0) && (DEreg.BundleSize > 0))
    {
        for (uint32_t SuperScalarCount = 0; SuperScalarCount < DEreg.BundleSize; SuperScalarCount++)
        {
            RNreg.Instr[SuperScalarCount] = DEreg.Instr[SuperScalarCount];
            RNreg.Instr[SuperScalarCount].TotalCycles[Decode] = (CurrentCycle + 1) - RNreg.Instr[SuperScalarCount].StartCycle[Decode];
            RNreg.Instr[SuperScalarCount].StartCycle[Rename] = CurrentCycle + 1;
            RNreg.BundleSize++;
            // cout << "CurrentCycle " << CurrentCycle << '\t' << RNreg.Instr[SuperScalarCount].StartCycle[Decode] << '\t' << RNreg.Instr[SuperScalarCount].StartCycle[Rename] << '\t' << RNreg.Instr[SuperScalarCount].StartCycle[Decode] << '\n';
        }

        DEreg.Initialize();
    }
    InstrDisplay(RNreg, "Decode->RegRename");
}

void PipelineRegisterRename()
{
    // If RN contains a rename bundle:

    // If either RR is not empty (cannot accept a new register-read bundle) or the ROB does not have enough free entries to accept the entire rename bundle, then do nothing.

    // If RR is empty (can accept a new register-read bundle) and the ROB has enough free entries to accept the entire rename bundle, then process (see below) the rename bundle and advance it from RN to RR.

    // Apply your learning from the class lectures/notes on the steps for renaming:
    // (1) allocate an entry in the ROB for the instruction, (2) rename its source registers, and (3) rename its destination register (if it has one).

    // Note that the rename bundle must be renamed in program order (fortunately the instructions in the rename bundle are in program order).
    uint32_t RobFreeSpace = 0;
    if (ROB.HeadPointer == ROB.TailPointer)
    {
        if (ROB.TailPointer < (RobSize - 1))
        {
            if (!(ROB.RobLine[ROB.TailPointer + 1].GetValid()))
                RobFreeSpace = RobSize;
        }
        else
        {
            if (!(ROB.RobLine[0].GetValid()))
                RobFreeSpace = RobSize;
        }
    }
    else if (ROB.HeadPointer > ROB.TailPointer)
        RobFreeSpace = ROB.HeadPointer - ROB.TailPointer;
    else if (ROB.HeadPointer < ROB.TailPointer)
        RobFreeSpace = RobSize - (ROB.TailPointer - ROB.HeadPointer);

    // RRreg.Initialize();
    if ((RRreg.BundleSize == 0) && (RNreg.BundleSize > 0) && (RobFreeSpace >= RNreg.BundleSize))
    {
        for (uint32_t SuperScalarCount = 0; SuperScalarCount < RNreg.BundleSize; SuperScalarCount++)
        {

            ROB.RobLine[ROB.TailPointer].SetDestination(RNreg.Instr[SuperScalarCount].RD);
            ROB.RobLine[ROB.TailPointer].SetReady(false);
            ROB.RobLine[ROB.TailPointer].SetProgramCounter(RNreg.Instr[SuperScalarCount].PC);
            ROB.RobLine[ROB.TailPointer].SetValid(true);
            // for (int i = 0; i < 5; i++)
            // cout << "RMT_" << i << "{" << RMT.RmtLine[i].Valid << "," << RMT.RmtLine[i].RobTag << "}" << '\n';
            if ((RMT.RmtLine[RNreg.Instr[SuperScalarCount].RS1].Valid) && (RNreg.Instr[SuperScalarCount].RS1 != 100)) // RS1 ROB
            {
                // cout << "PP" << '\t' << "RMT.RmtLine[" << RNreg.Instr[SuperScalarCount].RS1 << "] " << RMT.RmtLine[RNreg.Instr[SuperScalarCount].RS1].RobTag << '\n';

                RNreg.Instr[SuperScalarCount].RS1 = RMT.RmtLine[RNreg.Instr[SuperScalarCount].RS1].RobTag;
                RNreg.Instr[SuperScalarCount].RS1RobFlag = true;
                // RMT.RmtLine[RNreg.Instr[SuperScalarCount].RS1].Valid = true;
                // RMT.RmtLine[RNreg.Instr[SuperScalarCount].RS1].RobTag = ROB.TailPointer;
            }
            else // RS1 ARF
                RNreg.Instr[SuperScalarCount].RS1RobFlag = false;

            if ((RMT.RmtLine[RNreg.Instr[SuperScalarCount].RS2].Valid) && (RNreg.Instr[SuperScalarCount].RS2 != 100)) // RS2 ROB
            {
                RNreg.Instr[SuperScalarCount].RS2 = RMT.RmtLine[RNreg.Instr[SuperScalarCount].RS2].RobTag;
                RNreg.Instr[SuperScalarCount].RS2RobFlag = true;
                // RMT.RmtLine[RNreg.Instr[SuperScalarCount].RS2].Valid = true;
                // RMT.RmtLine[RNreg.Instr[SuperScalarCount].RS2].RobTag = ROB.TailPointer;
            }
            else // RS2 ARF
                RNreg.Instr[SuperScalarCount].RS2RobFlag = false;

            if (RNreg.Instr[SuperScalarCount].RD != 100) // Enter if Dest. Regn exists
            {
                // for (int i = 0; i < 5; i++)
                // cout << "Bef_RMT_" << i << "{" << RMT.RmtLine[i].Valid << "," << RMT.RmtLine[i].RobTag << "}" << '\n';
                // cout << "ping" << '\n';
                RMT.RmtLine[RNreg.Instr[SuperScalarCount].RDinitial].SetValid(true);
                RMT.RmtLine[RNreg.Instr[SuperScalarCount].RDinitial].SetRobTag(ROB.TailPointer);
                // cout << "RMT.RmtLine[" << RNreg.Instr[SuperScalarCount].RDinitial << "] " << ROB.TailPointer << '\n';
                // cout << "pong" << '\n';
                // for (int i = 0; i < 5; i++)
                // cout << "Aft_RMT_" << i << "{" << RMT.RmtLine[i].Valid << "," << RMT.RmtLine[i].RobTag << "}" << '\n';
            }
            RNreg.Instr[SuperScalarCount].RD = ROB.TailPointer;

            ROB.TailPointer += 1;
            if ((ROB.TailPointer) >= RobSize)
                ROB.TailPointer = 0;
            RRreg.Instr[SuperScalarCount] = RNreg.Instr[SuperScalarCount];
            RRreg.Instr[SuperScalarCount].TotalCycles[Rename] = (CurrentCycle + 1) - RRreg.Instr[SuperScalarCount].StartCycle[Rename];
            RRreg.Instr[SuperScalarCount].StartCycle[RegisterRead] = CurrentCycle + 1;
            RRreg.BundleSize++;
        }

        RNreg.Initialize();
    }
    // else
    //  cout << "RobFreeSpace:" << RobFreeSpace << '\t' << "Bundlesize:" << RNreg.BundleSize << '\t' << (RRreg.EmptyFlag) << '\t' << (!RNreg.EmptyFlag) << '\t' << (RobFreeSpace >= RNreg.BundleSize) << '\n';
    InstrDisplay(RRreg, "RegRename->RegRead");
}

void PipelineRegisterRead()
{
    // If RR contains a register-read bundle:

    // If DI is not empty (cannot accept a new dispatch bundle), then do nothing.

    // If DI is empty (can accept a new dispatch bundle), then process (see below) the register-read bundle and advance it from RR to DI.

    // Since values are not explicitly modeled, the sole purpose of the Register Read stage is to ascertain the readiness of the renamed source operands.

    // Apply your learning from the class lectures/notes on this topic.

    // Also take care that producers in their last cycle of execution wakeup dependent operands not just in the IQ, but also in two other stages including

    // RegRead() (this is required to avoid deadlock). See Execute() description above.
    // DIreg.Initialize();
    if ((DIreg.BundleSize == 0) && (RRreg.BundleSize > 0))
    {
        // cout << "aa" << '\t' << RRreg.BundleSize << '\n';
        for (uint32_t SuperScalarCount = 0; SuperScalarCount < RRreg.BundleSize; SuperScalarCount++)
        {
            // cout << "bb" << '\t' << RRreg.Instr[SuperScalarCount].RS1RobFlag << '\t' << SuperScalarCount << '\t' << RRreg.Instr[SuperScalarCount].PC << '\t' << RRreg.Instr[SuperScalarCount].RS1 << '\n';
            if (RRreg.Instr[SuperScalarCount].RS1RobFlag) // RS1 ROB
            {
                // RRreg.Instr[SuperScalarCount].Rs1Ready = ROB.RobLine[RRreg.Instr[SuperScalarCount].RS1].GetReady();
                if ((ROB.RobLine[RRreg.Instr[SuperScalarCount].RS1].Ready) && (ROB.RobLine[RRreg.Instr[SuperScalarCount].RS1].Valid))
                    RRreg.Instr[SuperScalarCount].Rs1Ready = ROB.RobLine[RRreg.Instr[SuperScalarCount].RS1].GetReady(); // true;
                else if ((!(ROB.RobLine[RRreg.Instr[SuperScalarCount].RS1].Valid)) && (!(ROB.RobLine[RRreg.Instr[SuperScalarCount].RS1].Ready)))
                    RRreg.Instr[SuperScalarCount].Rs1Ready = true;
            }
            else // RS1 ARF
                RRreg.Instr[SuperScalarCount].Rs1Ready = true;
            // cout << "cc" << '\t' << RRreg.Instr[SuperScalarCount].RS2RobFlag << '\t' << SuperScalarCount << '\t' << RRreg.Instr[SuperScalarCount].RS2 << '\t' << '\n';

            if (RRreg.Instr[SuperScalarCount].RS2RobFlag) // RS2 ROB
            {
                // RRreg.Instr[SuperScalarCount].Rs2Ready = ROB.RobLine[RRreg.Instr[SuperScalarCount].RS2].GetReady();
                if ((ROB.RobLine[RRreg.Instr[SuperScalarCount].RS2].Ready) && (ROB.RobLine[RRreg.Instr[SuperScalarCount].RS2].Valid))
                    RRreg.Instr[SuperScalarCount].Rs2Ready = ROB.RobLine[RRreg.Instr[SuperScalarCount].RS2].GetReady(); // true;
                else if ((!(ROB.RobLine[RRreg.Instr[SuperScalarCount].RS2].Valid)) && (!(ROB.RobLine[RRreg.Instr[SuperScalarCount].RS2].Ready)))
                    RRreg.Instr[SuperScalarCount].Rs2Ready = true;
            }
            else // RS2 ARF
                RRreg.Instr[SuperScalarCount].Rs2Ready = true;
            // cout << "dd" << '\n';

            DIreg.Instr[SuperScalarCount] = RRreg.Instr[SuperScalarCount];
            DIreg.Instr[SuperScalarCount].TotalCycles[RegisterRead] = (CurrentCycle + 1) - DIreg.Instr[SuperScalarCount].StartCycle[RegisterRead];
            DIreg.Instr[SuperScalarCount].StartCycle[Dispatch] = CurrentCycle + 1;
            DIreg.BundleSize++;
        }
        // cout << "ee" << '\n';

        RRreg.Initialize();
    }
    // else
    // cout << "{DIreg.EmptyFlag,!RRreg.EmptyFlag}_{" << DIreg.EmptyFlag << "," << (!RRreg.EmptyFlag) << "}" << '\n';
    InstrDisplay(DIreg, "RegRead->Dispatch");
}

void PipelineDispatch()
{
    // If DI contains a dispatch bundle:

    // If the number of free IQ entries is less than the size of the dispatch bundle in DI, then do nothing.

    // If the number of free IQ entries is greater than or equal to the size of the dispatch bundle in DI, then dispatch all instructions from DI to the IQ.
    uint32_t IqRegFreeEntryCount = 0;
    // cout << "A0_" << IqRegFreeEntryCount << '\n';
    for (uint32_t IqRegSearch = 0; IqRegSearch < IQsize; IqRegSearch++)
    {
        if (!(IQreg.Instr[IqRegSearch].ValidFlag))
            IqRegFreeEntryCount++;
    }
    // cout << "A1_" << IqRegFreeEntryCount << '\n';
    if ((DIreg.BundleSize > 0) && (IqRegFreeEntryCount >= DIreg.BundleSize))
    {
        // cout << "A2_" << IqRegFreeEntryCount << '\n';
        for (uint32_t SuperScalarCount = 0; SuperScalarCount < DIreg.BundleSize; SuperScalarCount++)
        {
            // cout << "A3_" << IqRegFreeEntryCount << '\n';
            for (uint32_t IqRegSearch = 0; IqRegSearch < IQsize; IqRegSearch++)
            {
                // cout << "A4_" << IqRegFreeEntryCount << '\n';
                if (!(IQreg.Instr[IqRegSearch].ValidFlag))
                {
                    // cout << "A5_" << IqRegFreeEntryCount << '\n';
                    IQreg.Instr[IqRegSearch] = DIreg.Instr[SuperScalarCount];
                    IQreg.Instr[IqRegSearch].TotalCycles[Dispatch] = (CurrentCycle + 1) - IQreg.Instr[IqRegSearch].StartCycle[Dispatch];
                    IQreg.Instr[IqRegSearch].StartCycle[Issue] = CurrentCycle + 1;
                    IQreg.BundleSize++;
                    break;
                }
            }
        }
        // cout << "A6_" << IqRegFreeEntryCount << '\n';
        DIreg.Initialize();
        // cout << "A7_" << IqRegFreeEntryCount << '\n';
    }
    // else
    // cout << "DIreg_" << (!DIreg.EmptyFlag) << "_" << (IqRegFreeEntryCount >= DIreg.BundleSize) << '\n';
    //// InstrDisplayIQ(IQreg, "Dispatch->IssueQueue");
    InstrDisplay(IQreg, "Dispatch->IssueQueue");
}

void PipelineIssueQueue()
{
    // Issue up to WIDTH oldest instructions from the IQ.

    //(One approach to implement oldest-first issuing, is to make multiple passes through the IQ, each time finding the next oldest ready instruction and then issuing it.
    // One way to annotate the age of an instruction is to assign an incrementing sequence number to each instruction as it is fetched from the trace file.)

    // To issue an instruction:
    // 1) Remove the instruction from the IQ.
    // 2) Add the instruction to the execute_list.

    // Set a timer for the instruction in the execute_list that will allow you to model its execution latency.

    if (IQreg.BundleSize > 0)
    {
        for (uint32_t SuperScalarCount = 0; SuperScalarCount < Width; SuperScalarCount++)
        {
            for (uint32_t IqTableSearch = 0; IqTableSearch < IQsize; IqTableSearch++)
            {
                // cout << "IQ Check_" << IQreg.Instr[IqTableSearch].ValidFlag << "_" << IQreg.Instr[IqTableSearch].Rs1Ready << "_" << IQreg.Instr[IqTableSearch].Rs2Ready << "_" << IQreg.Instr[IqTableSearch].PC << '\n';
                if (IQreg.Instr[IqTableSearch].ValidFlag && IQreg.Instr[IqTableSearch].Rs1Ready && IQreg.Instr[IqTableSearch].Rs2Ready)
                {
                    EXECUTEreg.Instr[EXECUTEreg.BundleSize] = IQreg.Instr[IqTableSearch];
                    EXECUTEreg.Instr[EXECUTEreg.BundleSize].TotalCycles[Issue] = (CurrentCycle + 1) - EXECUTEreg.Instr[EXECUTEreg.BundleSize].StartCycle[Issue];
                    EXECUTEreg.Instr[EXECUTEreg.BundleSize].StartCycle[Execute] = CurrentCycle + 1;
                    for (uint32_t IqTableSearch1 = IqTableSearch; IqTableSearch1 < (IQsize - 1); IqTableSearch1++)
                        IQreg.Instr[IqTableSearch1] = IQreg.Instr[IqTableSearch1 + 1];
                    IQreg.Instr[IQsize - 1].Initialize();
                    IQreg.BundleSize--;
                    EXECUTEreg.BundleSize++;
                    break;
                }
            }
            if (IQreg.BundleSize == 0)
                break;
        }
    }
    // else
    // cout << "ISSUE QUEUE_" << IQreg.BundleSize << '\n';
    InstrDisplay(EXECUTEreg, "IssueQueue->Execute");
}

void PipelineExecute()
{
    // From the execute_list, check for instructions that are finishing execution this cycle, and:
    // 1) Remove the instruction from the execute_list.
    // 2) Add the instruction to WB.
    // 3) Wakeup dependent instructions (set their source operand ready flags) in the IQ, DI (the dispatch bundle), and RR (the register-read bundle).

    // cout << "aa_" << EXECUTEreg.BundleSize << '\n';
    if (EXECUTEreg.BundleSize > 0)
    {
        // cout << "bb_" << EXECUTEreg.BundleSize << "_" << EXECUTEreg.ExecutionSize << '\n';
        for (uint32_t ExecListSearch = 0; ExecListSearch < EXECUTEreg.RegSizeCount; ExecListSearch++)
        {
            // cout << "bbb_" << ExecListSearch << '\n';
            if (EXECUTEreg.Instr[ExecListSearch].ValidFlag)
            {
                EXECUTEreg.Instr[ExecListSearch].ExecutionStageCount--;
                // cout << "cc_" << IQsize << "_" << EXECUTEreg.Instr[ExecListSearch].ExecutionStageCount << '\t' << hex << EXECUTEreg.Instr[ExecListSearch].PC << dec << '\n';
                if (EXECUTEreg.Instr[ExecListSearch].ExecutionStageCount == 0)
                {
                    // cout << dec << "dd_WBbundleSize_" << WBreg.BundleSize << '\n';
                    WBreg.Instr[WBreg.BundleSize] = EXECUTEreg.Instr[ExecListSearch];

                    for (uint32_t RRsearch = 0; RRsearch < RRreg.BundleSize; RRsearch++)
                    {
                        if (RRreg.Instr[RRsearch].RS1 == EXECUTEreg.Instr[ExecListSearch].RD)
                            RRreg.Instr[RRsearch].Rs1Ready = true;

                        if (RRreg.Instr[RRsearch].RS2 == EXECUTEreg.Instr[ExecListSearch].RD)
                            RRreg.Instr[RRsearch].Rs2Ready = true;
                    }

                    for (uint32_t DIsearch = 0; DIsearch < DIreg.BundleSize; DIsearch++)
                    {
                        if (DIreg.Instr[DIsearch].RS1 == EXECUTEreg.Instr[ExecListSearch].RD)
                            DIreg.Instr[DIsearch].Rs1Ready = true;

                        if (DIreg.Instr[DIsearch].RS2 == EXECUTEreg.Instr[ExecListSearch].RD)
                            DIreg.Instr[DIsearch].Rs2Ready = true;
                    }

                    for (uint32_t IqSearch = 0; IqSearch < IQsize; IqSearch++)
                    {
                        if (IQreg.Instr[IqSearch].RS1 == EXECUTEreg.Instr[ExecListSearch].RD)
                            IQreg.Instr[IqSearch].Rs1Ready = true;

                        if (IQreg.Instr[IqSearch].RS2 == EXECUTEreg.Instr[ExecListSearch].RD)
                            IQreg.Instr[IqSearch].Rs2Ready = true;
                    }

                    for (uint32_t ExecListSearch1 = ExecListSearch; ExecListSearch1 < (EXECUTEreg.RegSizeCount - 1); ExecListSearch1++)
                        EXECUTEreg.Instr[ExecListSearch1] = EXECUTEreg.Instr[ExecListSearch1 + 1];
                    EXECUTEreg.Instr[EXECUTEreg.RegSizeCount - 1].Initialize();
                    WBreg.Instr[WBreg.BundleSize].TotalCycles[Execute] = (CurrentCycle + 1) - WBreg.Instr[WBreg.BundleSize].StartCycle[Execute];
                    WBreg.Instr[WBreg.BundleSize].StartCycle[WriteBack] = CurrentCycle + 1;
                    EXECUTEreg.BundleSize--;
                    ExecListSearch--;
                    WBreg.BundleSize++;

                    // break;
                }
            }
        }

        // cout << "ee_" << '\n';

        // cout << "ff_" << '\n';
    }
    InstrDisplay(WBreg, "Execute->WB");
}

void PipelineWriteBack()
{
    // Process the writeback bundle in WB:

    // For each instruction in WB, mark the instruction as “ready” in its entry in the ROB.

    // uint32_t RtRegFreeEntryCount = 0;
    // cout << "WB0_" << '\n';
    if (WBreg.BundleSize > 0)
    {
        // cout << "WB1_" << '\n';
        /*    for (uint32_t RtRegSearch = 0; RtRegSearch < IQsize; RtRegSearch++)
            {
                if (!(RTreg.Instr[RtRegSearch].ValidFlag))
                    RtRegFreeEntryCount++;
            }
        */
        // cout << "WB2_" << '\n';
        for (uint32_t WBsearch = 0; WBsearch < WBreg.BundleSize; WBsearch++)
        {
            // cout << "WBWB3_" << WBreg.Instr[WBsearch].ValidFlag << '\t' << WBsearch << '\t' << WBreg.BundleSize << '\t' << WBreg.Instr[WBsearch].RD << '\t' << WBreg.Instr[WBsearch].RDinitial << '\n';
            if (WBreg.Instr[WBsearch].ValidFlag)
            {
                ROB.RobLine[WBreg.Instr[WBsearch].RD].SetReady(true);
                RTreg.Instr[RTreg.BundleSize] = WBreg.Instr[WBsearch];
                RTreg.Instr[RTreg.BundleSize].TotalCycles[WriteBack] = (CurrentCycle + 1) - RTreg.Instr[RTreg.BundleSize].StartCycle[WriteBack];
                RTreg.Instr[RTreg.BundleSize].StartCycle[Retire] = CurrentCycle + 1;
                RTreg.BundleSize++;
            }
            // break;
        }

        WBreg.Initialize();
    }
    InstrDisplay(RTreg, "WB->Retire");
}

void PipelineRetire()
{
    // Retire up to WIDTH consecutive “ready” instructions from the head of the ROB.
    for (uint32_t SuperScalarCount = 0; SuperScalarCount < Width; SuperScalarCount++)
    {
        // cout << "RTaa" << '\t' << ROB.HeadPointer << '\t' << ROB.TailPointer << '\t' << (ROB.RobLine[ROB.HeadPointer].Ready) << '\t' << (ROB.HeadPointer != ROB.TailPointer) << '\n';

        //!(ROB.RobLine[ROB.TailPointer - 1].GetValid()
        if (ROB.HeadPointer == ROB.TailPointer)
        {
            if (ROB.HeadPointer < RobSize - 1)
            {
                if (!(ROB.RobLine[ROB.HeadPointer + 1].GetValid()))
                    return;
            }
            else
            {
                if (!(ROB.RobLine[0].GetValid()))
                    return;
            }
        }

        if ((ROB.RobLine[ROB.HeadPointer].Ready))
        {
            // cout << "RTa" << '\n';
            if (RMT.RmtLine[ROB.RobLine[ROB.HeadPointer].Destination].Valid)
            {
                if (RMT.RmtLine[ROB.RobLine[ROB.HeadPointer].Destination].RobTag == ROB.HeadPointer)
                    RMT.RmtLine[ROB.RobLine[ROB.HeadPointer].Destination].Initialize();
            }
            // cout << "RTb" << '\n';
            for (uint32_t RTSearch = 0; RTSearch < RTreg.BundleSize; RTSearch++)
            {
                if ((RTreg.Instr[RTSearch].RD == ROB.HeadPointer) && (RTreg.Instr[RTSearch].PC == ROB.RobLine[ROB.HeadPointer].ProgramCounter))
                {
                    RTreg.Instr[RTSearch].TotalCycles[Retire] = (CurrentCycle + 1) - RTreg.Instr[RTSearch].StartCycle[Retire];

                    OUTPUTreg.Instr[OUTPUTreg.BundleSize] = RTreg.Instr[RTSearch];
                    OUTPUTreg.BundleSize++;
                    // cout << "RTc" << '\n';
                    // cout << RetireCount << " fu{" << RTreg.Instr[0].ExecutionStageCountTrace << "} src{" << RTreg.Instr[0].RS1initial << "," << RTreg.Instr[0].RS2initial << "} dst{";

                    for (uint32_t RTSearch1 = RTSearch; RTSearch1 < (RobSize - 1); RTSearch1++)
                        RTreg.Instr[RTSearch1] = RTreg.Instr[RTSearch1 + 1];
                    RTreg.Instr[RobSize - 1].Initialize();
                    ROB.RobLine[ROB.HeadPointer].Initialize();
                    ROB.HeadPointer++;
                    if (ROB.HeadPointer > (RobSize - 1))
                        ROB.HeadPointer = 0;
                    RTreg.BundleSize--;
                    // RetireCount++;
                    break;
                }
            }
        }
        else
            break;
    }
    InstrDisplay(RTreg, "Retire->ARF");
}

int main(int argc, char *argv[])
{
    void DisplayOutput();
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    RobSize = strtoul(argv[1], NULL, 10);
    IQsize = strtoul(argv[2], NULL, 10);
    Width = strtoul(argv[3], NULL, 10);
    TraceFileName = argv[4];
    CurrentCycle = 0;
    // RetireCount = 0;
    EndOfTraceFlag = false;
    ExSize = Width * 5;

    ROB.Initialize(RobSize);
    RMT.Initialize(67);
    DEreg.Initialize(Width);
    RNreg.Initialize(Width);
    RRreg.Initialize(Width);
    DIreg.Initialize(Width);
    IQreg.Initialize(IQsize);
    EXECUTEreg.Initialize(ExSize);
    WBreg.Initialize(ExSize);
    RTreg.Initialize(RobSize);
    OUTPUTreg.Initialize(25000);

    // Open TraceFileName in read mode
    TraceFilePointer = fopen(TraceFileName, "r");
    if (TraceFilePointer == NULL)
    {
        printf("Error: Unable to open file %s\n", TraceFileName);
        exit(EXIT_FAILURE);
    }
    // cout << "e" << '\n';
    do
    {
        if (DebugFlag)
            cout << "START**************************************************************************************************************************" << CurrentCycle << '\n';
        //  cout << "ASDF1_" << CurrentCycle << '\n';
        PipelineRetire();
        // cout << "ASDF2_" << CurrentCycle << '\n';
        PipelineWriteBack();
        // cout << "ASDF3_" << CurrentCycle << '\n';
        PipelineExecute();
        // cout << "ASDF4_" << CurrentCycle << '\n';
        PipelineIssueQueue();
        // cout << "ASDF5_" << CurrentCycle << '\n';
        PipelineDispatch();
        // cout << "ASDF6_" << CurrentCycle << '\n';
        PipelineRegisterRead();
        // cout << "ASDF7_" << CurrentCycle << '\n';
        PipelineRegisterRename();
        // cout << "ASDF8_" << CurrentCycle << '\n';
        PipelineDecode();
        //// cout << "1" << '\n';
        // cout << "ASDF9_" << CurrentCycle << '\n';
        PipelineFetch(TraceFilePointer);
        // cout << "ASDF10_" << CurrentCycle << '\n';
        //// cout << "2" << '\n';
        // cout << "END*******************************************************************************************************************************" << CurrentCycle << '\n';

        // if (CurrentCycle > 20)
        // break;
    } while (AdvanceCycle());
    // cout << "f" << '\n';
    DisplayOutput();
    // while (fscanf(TraceFilePointer, "%lx %d %d %d %d", &ProgramCounter, &OpcodeType, &Destination, &Source1, &Source2) != EOF)
    //     printf("%lx %d %d %d %d\n", ProgramCounter, OpcodeType, Destination, Source1, Source2); // Print to check if inputs have been read correctly

    /* cout
         << "rob_size:" << RobSize << '\n'
         << "iq_size:" << IQsize << '\n'
         << "width:" << Width << '\n'
         << TraceFileName << '\n';  */
    return 0;
}
