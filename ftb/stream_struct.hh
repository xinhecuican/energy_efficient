#ifndef __CPU_PRED_FTB_STREAM_STRUCT_HH__
#define __CPU_PRED_FTB_STREAM_STRUCT_HH__

#include <boost/dynamic_bitset.hpp>

#include "arch/generic/pcstate.hh"
#include "base/types.hh"
#include "cpu/inst_seq.hh"
#include "cpu/pred/ftb/stream_common.hh"
#include "cpu/pred/general_arch_db.hh"
#include "cpu/static_inst.hh"
#include "debug/DecoupleBP.hh"
#include "debug/FTB.hh"
#include "debug/MDEBUG2.hh"
#include "debug/Override.hh"

// #include "cpu/pred/ftb/ftb.hh"

namespace gem5 {

namespace branch_prediction {

namespace ftb_pred {

enum EndType
{
    END_CALL=0,
    END_RET,
    END_OTHER_TAKEN,
    END_NOT_TAKEN,
    END_CONT,  // to be continued
    END_NONE
};

enum SquashType
{
    SQUASH_NONE=0,
    SQUASH_TRAP,
    SQUASH_CTRL,
    SQUASH_OTHER
};


enum BranchType
{
    ALL=0, /// 全部执行
    DIRECT, /// 全部不执行, 包含不跳转和绝对跳转
    CONDITION, /// 不执行ras ittage
    INDIRECT, /// 不执行tage-sc
    DIRECT_CALL /// 执行ras
};
typedef struct BranchInfo
{
    Addr pc;
    Addr target;
    bool isCond;
    bool isIndirect;
    bool isCall;
    bool isReturn;
    uint8_t size;
    bool isUncond() const { return !this->isCond; }
    Addr getEnd() { return this->pc + this->size; }
    BranchInfo() : pc(0), target(0), isCond(false), isIndirect(false), isCall(false), isReturn(false), size(0) {}
    BranchInfo (const Addr &control_pc,
                const Addr &target_pc,
                const StaticInstPtr &static_inst,
                unsigned size) :
        pc(control_pc),
        target(target_pc),
        isCond(static_inst->isCondCtrl()),
        isIndirect(static_inst->isIndirectCtrl()),
        isCall(static_inst->isCall()),
        isReturn(static_inst->isReturn() && !static_inst->isNonSpeculative() && !static_inst->isDirectCtrl()),
        size(size) {}

    int getType() {
        if (isCond) {
            return 0;
        } else if (!isIndirect) {
            if (isReturn) {
                fatal("jal return detected!\n");
                return 7;
            }
            if (!isCall) {
                return 1;
            } else {
                return 2;
            }
        } else {
            if (!isCall) {
                if (!isReturn) {
                    return 3; // normal indirect
                } else {
                    return 4; // indirect return
                }
            } else {
                if (!isReturn) { // indirect call
                    return 5;
                } else { // call & return
                    return 6;
                }
            }
        }
    }

    bool operator < (const BranchInfo &other) const
    {
        return this->pc < other.pc;
    }

    bool operator == (const BranchInfo &other) const
    {
        return this->pc == other.pc;
    }

    bool operator > (const BranchInfo &other) const
    {
        return this->pc > other.pc;
    }

    bool operator != (const BranchInfo &other) const
    {
        return this->pc != other.pc;
    }
}BranchInfo;

typedef struct FTBSlot : BranchInfo
{
    bool valid;
    bool alwaysTaken;
    int ctr;
    int alwaysTakenCtr=0;
    bool isHighConf = false;
    BranchType type;
    int straightValid = false;
    bool uncondValid() { return this->isUncond() && this->valid; }
    bool condValid() { return this->isCond && this->valid;}
    FTBSlot() : valid(false), type(ALL) {}
    FTBSlot(const BranchInfo &bi) : BranchInfo(bi), valid(true), alwaysTaken(true), ctr(0), type(ALL) {}
    BranchInfo getBranchInfo() { return BranchInfo(*this); }

    bool operator==(const FTBSlot& b){
        return valid == b.valid &&
                pc == b.pc;
    }

}FTBSlot;

typedef struct LFSR64
{
    uint64_t lfsr;
    LFSR64() : lfsr(0x1234567887654321UL) {}
    uint64_t get() {
        next();
        return lfsr;
    }
    void next() {
        if (lfsr == 0) {
            lfsr = 1;
        } else {
            uint64_t bit = ((lfsr >> 0) ^ (lfsr >> 1) ^ (lfsr >> 3) ^ (lfsr >> 4)) & 1;
            lfsr = (lfsr >> 1) | (bit << 63);
        }
    }
}LFSR64;

typedef struct FTBEntry
{
    /** The entry's tag. */
    Addr tag = 0;

    /** The entry's branch info. */
    std::vector<FTBSlot> slots;

    /** The entry's fallthrough address. */
    Addr fallThruAddr;
    BranchType fallThruType;
    bool fallNextIsHigh = false;
    int fallStraightValid = false;
    FTBSlot* control;
    int straightValid = 0;

    /** The entry's thread id. */
    ThreadID tid;

    /** Whether or not the entry is valid. */
    bool valid = false;
    FTBEntry(): fallThruAddr(0), fallThruType(ALL), tid(0), valid(false) {}

    int getNumCondInEntryBefore(Addr pc) {
        int num = 0;
        for (auto &slot : this->slots) {
            if (slot.condValid() && slot.pc < pc) {
                num++;
            }
        }
        return num;
    }

    int getTotalNumConds() {
        int num = 0;
        for (auto &slot : this->slots) {
            if (slot.condValid()) {
                num++;
            }
        }
        return num;
    }

    // check if the entry is reasonable with given startPC
    // every branch slot and fallThru should be in the range of (startPC, startPC+34]
    // every
    bool isReasonable(Addr start) {
        Addr min = start;
        Addr max = start+34;
        bool reasonable = true;
        for (auto &slot : slots) {
            if (slot.pc < min || slot.pc > max) {
                reasonable = false;
            }
        }
        if (fallThruAddr <= min || fallThruAddr > max) {
            reasonable = false;
        }
        return reasonable;
    }

    FTBSlot getSlot(Addr pc) {
        for (auto &slot : this->slots) {
            if (slot.pc == pc) {
                return slot;
            }
        }
        return FTBSlot();
    }


    bool operator == (const FTBEntry &other) const
    {
        // startPC and slots pc
        if (this->tag != other.tag || this->slots.size() != other.slots.size()) {
            return false;
        }
        for (int i = 0; i < this->slots.size(); i++) {
            if (this->slots[i] != other.slots[i]) {
                return false;
            }
        }
        return true;
    }

    BranchType getEntryType(bool highConf, Addr branchAddr, bool& containBranch, bool& condValid, bool& indirectValid, bool& isDirect){
        for (int i=0; i<(int)slots.size(); i++){
            FTBSlot slot = slots[i];
            if (slot.alwaysTaken && slot.isCond && slot.pc == branchAddr && slot.alwaysTakenCtr >= 3) {
                isDirect = true;
                continue;
            }
            // if (slot.isCond && slot.pc == branchAddr && slot.isHighConf){
            //     isDirect = true;
            //     continue;
            // }
            if (!condValid){
                condValid = slot.isCond;
            }
            if (!indirectValid){
                indirectValid = slot.isIndirect;
            }
            if (!isDirect){
                isDirect = !slot.isCond && !slot.isIndirect;
            }
        }
        containBranch = containBranch | condValid | indirectValid | isDirect;
        int branchTypes = condValid  + indirectValid;
        return branchTypes > 1 ? ALL :
                                condValid ? CONDITION :
                                indirectValid ? INDIRECT :
                                ((isDirect) && (branchTypes == 0)) || !containBranch ? DIRECT : ALL;
    }

    BranchType getEntryTypePredict(bool& containBranch, bool& condValid, bool& indirectValid, bool& isDirect){
        for (int i=0; i<(int)slots.size(); i++){
            FTBSlot slot = slots[i];
            int type = slot.getType();
            // if (slot.isCond && slot.isHighConf){
            //     isDirect = true;
            //     continue;
            // }
            if (!condValid){
                condValid = slot.isCond;
            }
            if (!indirectValid){
                indirectValid = slot.isIndirect;
            }
            if (!isDirect){
                isDirect = !slot.isCond && !slot.isIndirect;
            }
        }
        containBranch = containBranch | condValid | indirectValid | isDirect;
        int branchTypes = condValid  + indirectValid;
        return branchTypes > 1 ? ALL :
                                condValid ? CONDITION :
                                indirectValid ? INDIRECT :
                                ((isDirect) && (branchTypes == 0)) || !containBranch ? DIRECT : ALL;
    }
}FTBEntry;

struct BlockDecodeInfo
{
    std::vector<bool> condMask;
    BranchInfo jumpInfo;
};


using FetchStreamId = uint64_t;
using FetchTargetId = uint64_t;
using PredictionID = uint64_t;

typedef struct LoopEntry
{
    bool valid;
    int tripCnt;
    int specCnt;
    int conf;
    bool repair;
    LoopEntry() : valid(false), tripCnt(0), specCnt(0), conf(0), repair(false) {}
} LoopEntry;

typedef struct LoopRedirectInfo
{
    LoopEntry e;
    Addr branch_pc;
    bool end_loop;
} LoopRedirectInfo;

typedef struct JAEntry
{
    // jump target: indexPC + jumpAheadBlockNum * blockSize
    int jumpAheadBlockNum;
    int conf;
    JAEntry() : jumpAheadBlockNum(0), conf(0) {}
    Addr getJumpTarget(Addr indexPC, int blockSize) {
        return indexPC + jumpAheadBlockNum * blockSize;
    }
} JAEntry;

// NOTE: now this corresponds to an ftq entry in
//       XiangShan nanhu architecture
typedef struct FetchStream
{
    Addr startPC;

    // indicating whether a backing prediction has finished
    // bool predEnded;
    bool predTaken;

    // predicted stream end pc (fall through pc)
    Addr predEndPC;
    BranchInfo predBranchInfo;
    // record predicted FTB entry
    bool isHit;
    bool falseHit;
    FTBEntry predFTBEntry;

    bool sentToICache;

    // for commit, write at redirect or fetch
    // bool exeEnded;
    bool exeTaken;
    // Addr exeEndPC;
    BranchInfo exeBranchInfo;

    FTBEntry updateFTBEntry;
    bool updateIsOldEntry;
    bool resolved;

    int squashType;
    Addr squashPC;
    unsigned predSource;

    // for loop buffer
    bool fromLoopBuffer;
    bool isDouble;
    bool isExit;

    // for ja predictor
    bool jaHit;
    JAEntry jaEntry;
    int currentSentBlock;

    BranchType preBranchType = ALL;
    bool containCond = false;
    bool containIndirect = false;
    bool containDirect = false;
    bool containBranch = false;
    int straightVaild = 0;
    int straightTimes = 0;
    Addr directBranchAddr = 0;
    bool highConf = false;
    BranchType updateBranchType = ALL;
    int updateSlotNum = 0;

    // prediction metas
    // FIXME: use vec
    std::array<std::shared_ptr<void>, 6> predMetas;

    // for loop
    std::vector<LoopRedirectInfo> loopRedirectInfos;
    std::vector<bool> fixNotExits;
    std::vector<LoopRedirectInfo> unseenLoopRedirectInfos;

    Tick predTick;
    boost::dynamic_bitset<> history;

    // for profiling
    int fetchInstNum;
    int commitInstNum;

    FetchStream()
        : startPC(0),
          predTaken(false),
          predEndPC(0),
          predBranchInfo(BranchInfo()),
          isHit(false),
          falseHit(false),
          predFTBEntry(FTBEntry()),
          sentToICache(false),
          exeTaken(false),
          exeBranchInfo(BranchInfo()),
          updateFTBEntry(FTBEntry()),
          updateIsOldEntry(false),
          resolved(false),
          squashType(SquashType::SQUASH_NONE),
          predSource(0),
          fromLoopBuffer(false),
          isDouble(false),
          isExit(false),
          jaHit(false),
          jaEntry(JAEntry()),
          currentSentBlock(0),
          fetchInstNum(0),
          commitInstNum(0)
    {
    }

    // the default exe result should be consistent with prediction
    void setDefaultResolve() {
        resolved = false;
        exeBranchInfo = predBranchInfo;
        exeTaken = predTaken;
    }

    // bool getEnded() const { return resolved ? exeEnded : predEnded; }
    BranchInfo getBranchInfo() const { return resolved ? exeBranchInfo : predBranchInfo; }
    Addr getControlPC() const { return getBranchInfo().pc; }
    Addr getEndPC() const {
        if (predTaken) return predEndPC;
        else if (jaHit) return startPC + (currentSentBlock + 1) * 0x20;
        else return predEndPC;
    }
    Addr getTaken() const { return resolved ? exeTaken : predTaken; }
    Addr getTakenTarget() const { return getBranchInfo().target; }
    // Addr getFallThruPC() const { return getEndPC(); }
    // Addr getNextStreamStart() const {return getTaken() ? getTakenTarget() : getFallThruPC(); }
    // bool isCall() const { return endType == END_CALL; }
    // bool isReturn() const { return endType == END_RET; }

    // for ja hit blocks, should be the biggest addr of startPC + k*blockSize where k is interger
    Addr getRealStartPC() const {
        if (jaHit && squashType == SQUASH_CTRL) {
            Addr realStart = startPC;
            Addr squashBranchPC = exeBranchInfo.pc;
            while (realStart + 0x20 <= squashBranchPC) {
                realStart += 0x20;
            }
            return realStart;
        } else {
            return startPC;
        }
    }

    std::pair<int, bool> getHistInfoDuringSquash(Addr squash_pc, bool is_cond, bool actually_taken, unsigned maxShamt)
    {
        bool hit = isHit;
        if (!hit) {
            int shamt = is_cond ? 1 : 0;
            return std::make_pair(shamt, actually_taken);
        } else {
            int shamt = predFTBEntry.getNumCondInEntryBefore(squash_pc);
            assert(shamt <= maxShamt);
            if (is_cond) {
                if (shamt == maxShamt) {
                    // current cond should not be counted into this entry
                    return std::make_pair(2, false);
                } else {
                    return std::make_pair(shamt+1, actually_taken);
                }
            } else {
                return std::make_pair(shamt, false);
            }
        }
    }

}FetchStream;

typedef struct FullFTBPrediction
{
    Addr bbStart;
    FTBEntry ftbEntry; // for FTB
    std::vector<bool> condTakens; // for conditional branch predictors
    std::vector<bool> condIsHigh;
    std::vector<bool> ftbTaken;
    bool ftbValid;

    Addr indirectTarget; // for indirect predictor
    Addr returnTarget; // for RAS

    BranchType preBranchType = ALL;

    bool valid; // hit
    unsigned predSource;
    Tick predTick;
    boost::dynamic_bitset<> history;

    // direct stream
    bool directValid = false;
    int directTimes = 0;
    int directType = 0;
    int directAddr = 0;

    bool isTaken() {
        auto &ftbEntry = this->ftbEntry;
        if (valid) {
            int i = 0;
            for (auto &slot : ftbEntry.slots) {
                if ((slot.condValid() && condTakens[i]) ||
                    slot.uncondValid()) {
                        return true;
                    }
                i++;
            }
        }
        return false;
    }

    bool isHigh(){
        auto &ftbEntry = this->ftbEntry;
        if (valid) {
            int i = 0;
            auto takenSlot = getTakenSlot();
            for (auto &slot : ftbEntry.slots) {
                if ((takenSlot == slot) && (slot.condValid() && condIsHigh[i])) {
                    return true;
                }
                i++;
            }
        }

        return false;
    }

    bool isHighEqual(){
        auto &ftbEntry = this->ftbEntry;
        if (valid) {
            int i = 0;
            auto takenSlot = getTakenSlot();
            for (auto &slot : ftbEntry.slots) {
                if ((takenSlot == slot) && (slot.condValid() && condIsHigh[i])) {
                    return true;
                }
                i++;
            }
        }

        return false;
    }

    bool debugIsTaken(){
        auto &ftbEntry = this->ftbEntry;
        if (valid) {
            int i = 0;
            for (auto &slot : ftbEntry.slots) {
                int slotType = slot.getType();
                if ((slot.condValid() && condTakens[i]) ||
                    (((slotType == 2) || (slotType == 7)) && ((preBranchType == ALL) || (preBranchType == DIRECT_CALL))) ||
                    ((slotType == 1)) ||
                    (slot.isIndirect && ((preBranchType == ALL) || (preBranchType == INDIRECT)))) {
                        return true;
                    }
                i++;
            }
        }
        return false;
    }

    BranchType generateBranchType(){
        auto &ftbEntry = this->ftbEntry;
        BranchType branchType = ALL;
        if (valid) {
            int i = 0;
            bool hit = false;
            for (auto &slot : ftbEntry.slots) {
                if ((slot.condValid() && condTakens[i]) ||
                    slot.uncondValid()) {
                        hit = true;
                        if (slot.isIndirect && false)
                            branchType = ALL;
                        // else if (ftbEntry.straightValid && ftbEntry.control == &slot){
                        // // else if (slot.straightValid){
                        //     branchType = DIRECT;
                        // }
                        else
                            branchType = slot.type;
                        break;
                    }
                i++;
            }
            if (hit){
                return branchType;
            }
            return ftbEntry.fallThruType;
        }
        return ALL;
    }

    int isStraightValid(){
        auto &ftbEntry = this->ftbEntry;
        int32_t straightValid = false;
        if (valid) {
            int i = 0;
            bool hit = false;
            for (auto &slot : ftbEntry.slots) {
                if ((slot.condValid() && condTakens[i]) ||
                    slot.uncondValid()) {
                        hit = true;
                        // straightValid = ftbEntry.straightValid && (ftbEntry.control == &slot) && !slot.isIndirect ? ftbEntry.straightValid : 0;
                        // straightValid = slot.straightValid && !slot.isIndirect ? slot.straightValid : 0;
                        straightValid = slot.straightValid ? slot.straightValid : 0;
                        break;
                    }
                i++;
            }
            if (hit){
                return straightValid;
            }
            return ftbEntry.fallStraightValid;
        }
        return false;
    }

    int getNoBranchTimes(){
        auto &ftbEntry = this->ftbEntry;
        int noBranchTimes = 0;
        if (valid) {
            int i = 0;
            bool hit = false;
            for (auto &slot : ftbEntry.slots) {
                if ((slot.condValid() && condTakens[i]) ||
                    slot.uncondValid()) {
                        hit = true;
                        noBranchTimes = slot.straightValid == 1 ? 1 :
                                        slot.straightValid == 2 ? 2 :
                                        slot.type + 2;
                        break;
                    }
                i++;
            }
            if (hit){
                return noBranchTimes;
            }
            return ftbEntry.fallStraightValid == 1 ? 1 :
                    ftbEntry.fallStraightValid == 2 ? 2 :
                    ftbEntry.fallThruType + 2;
        }
        return 0;
    }

    bool isEqualL0() {
        auto &ftbEntry = this->ftbEntry;
        if (valid) {
            int i = 0;
            for (auto &slot : ftbEntry.slots) {
                if ((slot.condValid() && condTakens[i])) {
                        return ftbValid && ftbTaken[i];
                    }
                i++;
            }
        }
        return false;
    }

    bool setBranchType(BranchType branchType){
        preBranchType = branchType;
        auto &ftbEntry = this->ftbEntry;
        for (auto& slot: ftbEntry.slots){
            if (slot.alwaysTaken){
                return true;
            }
        }
        bool containBranch = false;
        bool condValid = false;
        bool indirectValid = false;
        bool isDirect = false;
        BranchType currentBranchType = ftbEntry.getEntryTypePredict(containBranch, condValid, indirectValid, isDirect);
        if ((branchType == DIRECT && (currentBranchType != DIRECT)) ||
            (branchType == CONDITION && ((currentBranchType == INDIRECT) || (currentBranchType == ALL))) ||
            (branchType == INDIRECT && ((currentBranchType == CONDITION) || (currentBranchType == ALL)))){
            return false;
        }
        return true;
        // switch(preBranchType){
        //     case DIRECT:{
        //         for (int i=0; i<(int)condTakens.size(); i++){
        //             condTakens[i] = 0;
        //         }
        //         indirectTarget = 0;
        //         // returnTarget = 0;
        //         break;
        //     }
        //     case CONDITION: {
        //         indirectTarget = 0;
        //         // returnTarget = 0;
        //         break;
        //     }
        //     case DIRECT_CALL: {
        //         for (int i=0; i<(int)condTakens.size(); i++){
        //             condTakens[i] = 0;
        //         }
        //         indirectTarget = 0;
        //         break;
        //     }
        //     case INDIRECT: { // 非直接跳转中很多跳转地址是会变的
        //         for (int i=0; i<(int)condTakens.size(); i++){
        //             condTakens[i] = 0;
        //         }
        //         // returnTarget = 0;
        //         break;
        //     }
        //     default:break;
        // }
    }

    FTBSlot getTakenSlot() {
        auto &ftbEntry = this->ftbEntry;
        if (valid) {
            int i = 0;
            for (auto &slot : ftbEntry.slots) {
                if ((slot.condValid() && condTakens[i]) ||
                    slot.uncondValid()) {
                        return slot;
                    }
                i++;
            }
        }
        return FTBSlot();
    }

    Addr getTarget() {
        Addr target;
        auto &ftbEntry = this->ftbEntry;
        if (valid) {
            auto slot = getTakenSlot();
            if (slot.condValid()) {
                target = slot.target;
            } else if (slot.uncondValid()) {
                target = slot.target;
                if (slot.isIndirect) {
                    target = indirectTarget;
                }
                if (slot.isReturn) {
                    target = returnTarget;
                }
            } else {
                target = ftbEntry.fallThruAddr;
            }

        } else {
            target = bbStart + 32; //TODO: +predictWidth
        }
        return target;
    }

    Addr getBranchAddr(){
        auto& ftbEntry = this->ftbEntry;
        if (valid){
            int i = 0;
            for (auto &slot : ftbEntry.slots) {
                if ((slot.condValid() && condTakens[i]) ||
                    slot.uncondValid()) {
                        return slot.pc;
                    }
                i++;
            }
        }
        return getFallThrough();
    }

    Addr getEnd() {
        if (isTaken()) {
            return getTakenSlot().getEnd();
        } else {
            return getFallThrough();
        }
    }



    Addr getFallThrough() {
        auto &ftbEntry = this->ftbEntry;
        if (valid) {
            return ftbEntry.fallThruAddr;
        } else {
            return bbStart + 32; //TODO: +predictWidth
        }
    }

    Addr controlAddr() {
        return getTakenSlot().pc;
    }

    int getTakenBranchIdx() {
        auto &ftbEntry = this->ftbEntry;
        if (valid) {
            int i = 0;
            for (auto &slot : ftbEntry.slots) {
                if ((slot.condValid() && condTakens[i]) ||
                    slot.uncondValid()) {
                        return i;
                    }
                i++;
            }
        }
        return -1;
    }

    bool match(FullFTBPrediction &other)
    {
        if (!other.valid) {
            // chosen is invalid, means all predictors invalid, match
            return true;
        } else {
            // chosen is valid
            if (!this->valid) {
                // if this is invalid and chosen is valid, sure no match
                return false;
            } else {
                bool this_taken, other_taken;
                int this_cond_num, other_cond_num;
                std::tie(this_cond_num, this_taken) = this->getHistInfo();
                std::tie(other_cond_num, other_taken) = other.getHistInfo();
                Addr this_control_addr = this->controlAddr();
                Addr other_control_addr = other.controlAddr();
                Addr this_npc = this->getTarget();
                Addr other_npc = other.getTarget();
                // both this and chosen valid
                return this_taken == other_taken &&
                       this_control_addr == other_control_addr &&
                       this_cond_num == other_cond_num &&
                       this_npc == other_npc;
            }
        }
    }

    std::vector<boost::dynamic_bitset<>> indexFoldedHist;
    std::vector<boost::dynamic_bitset<>> tagFoldedHist;

    std::pair<int, bool> getHistInfo()
    {
        int shamt = 0;
        bool taken = false;
        if (valid) {
            int i = 0;
            for (auto &slot : ftbEntry.slots) {
                DPRINTF(Override, "slot %d: condValid %d, uncondValid %d\n",
                    i, slot.condValid(), slot.uncondValid());
                DPRINTF(Override, "condTakens.size() %d\n", condTakens.size());
                if (slot.condValid()) {
                    shamt++;
                    if (condTakens[i]) {
                        taken = true;
                        break;
                    }
                }
                assert(condTakens.size() >= i+1);

                i++;
            }
        }
        return std::make_pair(shamt, taken);
    }

    bool isReasonable() {
        return !valid || ftbEntry.isReasonable(bbStart);
    }


}FullFTBPrediction;

// each entry corresponds to a 32Byte unaligned block
struct FtqEntry
{
    Addr startPC;
    Addr endPC;    // TODO: use PCState and it can be included in takenPC

    // When it is a taken branch, takenPC is the control (starting) PC
    // When it is yet missing, takenPC is the ``known'' PC,
    // decoupledPredict cannot goes beyond takenPC and should be blocked
    // when current PC == takenPC
    Addr takenPC;

    bool taken;
    Addr target;  // TODO: use PCState
    FetchStreamId fsqID;

    // for loop buffer
    bool inLoop;
    int iter;
    bool isExit;
    Addr loopEndPC;

    // for ja predictor
    int noPredBlocks;

    FtqEntry()
        : startPC(0)
        , endPC(0)
        , takenPC(0)
        , taken(false)
        , target(0)
        , fsqID(0)
        , inLoop(false)
        , iter(0)
        , isExit(false)
        , loopEndPC(0)
        , noPredBlocks(0) {}

    bool miss() const { return !taken; }
    // bool filledUp() const { return (endPC & fetchTargetMask) == 0; }
    // unsigned predLoopIteration;
};


struct TageMissTrace : public Record
{
    void set(uint64_t startPC, uint64_t branchPC, uint64_t lgcBank, uint64_t phyBank, uint64_t mainFound, uint64_t mainCounter, uint64_t mainUseful,
        uint64_t altCounter, uint64_t mainTable, uint64_t mainIndex, uint64_t altIndex, uint64_t tag,
        uint64_t useAlt, uint64_t predTaken, uint64_t actualTaken, uint64_t allocSuccess, uint64_t allocFailure,
        uint64_t predUseSC, uint64_t predSCDisagree, uint64_t predSCCorrect)
    {
        _tick = curTick();
        _uint64_data["startPC"] = startPC;
        _uint64_data["branchPC"] = branchPC;
        _uint64_data["lgcBank"] = lgcBank;
        _uint64_data["phyBank"] = phyBank;
        _uint64_data["mainFound"] = mainFound;
        _uint64_data["mainCounter"] = mainCounter;
        _uint64_data["mainUseful"] = mainUseful;
        _uint64_data["altCounter"] = altCounter;
        _uint64_data["mainTable"] = mainTable;
        _uint64_data["mainIndex"] = mainIndex;
        _uint64_data["altIndex"] = altIndex;
        _uint64_data["tag"] = tag;
        _uint64_data["useAlt"] = useAlt;
        _uint64_data["predTaken"] = predTaken;
        _uint64_data["actualTaken"] = actualTaken;
        _uint64_data["allocSuccess"] = allocSuccess;
        _uint64_data["allocFailure"] = allocFailure;
        _uint64_data["predUseSC"] = predUseSC;
        _uint64_data["predSCDisagree"] = predSCDisagree;
        _uint64_data["predSCCorrect"] = predSCCorrect;
    }
};

struct LoopTrace : public Record
{
    void set(uint64_t pc, uint64_t target, uint64_t mispred, uint64_t training,
        uint64_t trainSpecCnt, uint64_t trainTripCnt, uint64_t trainConf,
        uint64_t inMain, uint64_t mainTripCnt, uint64_t mainConf, uint64_t predSpecCnt,
        uint64_t predTripCnt, uint64_t predConf)
    {
        _tick = curTick();
        _uint64_data["pc"] = pc;
        _uint64_data["target"] = target;
        _uint64_data["mispred"] = mispred;
        _uint64_data["predSpecCnt"] = predSpecCnt;
        _uint64_data["predTripCnt"] = predTripCnt;
        _uint64_data["predConf"] = predConf;
        // from lp
        _uint64_data["training"] = training;
        _uint64_data["trainSpecCnt"] = trainSpecCnt;
        _uint64_data["trainTripCnt"] = trainTripCnt;
        _uint64_data["trainConf"] = trainConf;
        _uint64_data["inMain"] = inMain;
        _uint64_data["mainTripCnt"] = mainTripCnt;
        _uint64_data["mainConf"] = mainConf;
    }
    void set_in_lp(uint64_t training, uint64_t trainSpecCnt, uint64_t trainTripCnt, uint64_t trainConf,
        uint64_t inMain, uint64_t mainTripCnt, uint64_t mainConf)
    {
        _uint64_data["training"] = training;
        _uint64_data["trainSpecCnt"] = trainSpecCnt;
        _uint64_data["trainTripCnt"] = trainTripCnt;
        _uint64_data["trainConf"] = trainConf;
        _uint64_data["inMain"] = inMain;
        _uint64_data["mainTripCnt"] = mainTripCnt;
        _uint64_data["mainConf"] = mainConf;
    }
    void set_outside_lp(uint64_t pc, uint64_t target, uint64_t mispred,
        uint64_t predSpecCnt, uint64_t predTripCnt, uint64_t predConf)
    {
        _tick = curTick();
        _uint64_data["pc"] = pc;
        _uint64_data["target"] = target;
        _uint64_data["mispred"] = mispred;
        _uint64_data["predSpecCnt"] = predSpecCnt;
        _uint64_data["predTripCnt"] = predTripCnt;
        _uint64_data["predConf"] = predConf;
    }
};

}  // namespace ftb_pred

}  // namespace branch_prediction

}  // namespace gem5
#endif  // __CPU_PRED_FTB_STREAM_STRUCT_HH__
