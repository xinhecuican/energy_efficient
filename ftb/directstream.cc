#include "cpu/pred/ftb/directstream.hh"

#include "base/intmath.hh"
#include "base/trace.hh"
#include "cpu/o3/dyn_inst.hh"

namespace gem5
{

namespace branch_prediction
{

namespace ftb_pred
{

DirectStream::DirectStream(int numEntries, int tagWidth, int ageWidth, int timeWidth, int addrWidth){
    table.resize(numEntries);
        tableWidth = log2(numEntries);
    tableMask = numEntries - 1;
    tagMask = (1 << tagWidth) - 1;
    timeMask = (1 << timeWidth) - 1;
    ageMask = (1 << ageWidth) - 1;
    brAddrMask = (1 << addrWidth) - 1;
    hit = false;
    directTimes = 0;
    startAddr = 0;
    startCheck = false;
    preValid = true;
    preSquash = false;
}

void DirectStream::putPCHistory(Addr startAddr, const boost::dynamic_bitset<> &history,
                      std::vector<FullFTBPrediction> &stagePreds){
    streamEntry element = lookup(startAddr);
    for (int i=0; i<stagePreds.size(); i++){
        stagePreds[i].directValid = hit;
        stagePreds[i].directTimes = element.times;
        stagePreds[i].directType = element.type;
        stagePreds[i].directAddr = element.brAddr;
    }
    meta.hit = hit;
}

void DirectStream::update(const FetchStream &stream){

    bool containBranch = stream.updateSlotNum != 0;
    if (!containBranch){
        if (!startCheck && preValid){
            startAddr = preAddr;
            startBranchAddr = preBranchAddr;
            directTimes = 1;
            startCheck = true;
        }
        else if (preSquash){
            startCheck = false;
        }
        else{
            directTimes++;
        }
    }
    else{
        BranchType type = stream.updateBranchType;
        if (startCheck && !preSquash){
        // if (startCheck){
            int index = getIndex(startAddr);
            int tag = getTag(startAddr);
                        int brAddr = getBrAddr(startBranchAddr);
            if (table[index].en &&
                                (table[index].tag == tag) &&
                                (table[index].brAddr == brAddr)){
                table[index].age = table[index].age > ageMask ? ageMask + 1 : table[index].age + 1;
                // table[index].age = (table[index].age + 1) & ageMask;
            }
            else{
                int age = table[index].age > 0 ? table[index].age - 1 : 0;
                if (age == 0){
                    table[index].tag = tag;
                    table[index].brAddr = brAddr;
                    table[index].times = directTimes > timeMask ? timeMask + 1 : directTimes;
                    // table[index].times = (directTimes + 1) & timeMask;
                    table[index].type = type;
                    table[index].age = 0;
                    table[index].en = true;
                }
            }
        }
        startCheck = false;
   }
   preSquash = (stream.squashType == SQUASH_CTRL && !stream.exeTaken) ||
                (stream.squashType == SQUASH_TRAP) ||
                (stream.squashType == SQUASH_OTHER);
    if (preSquash ||
        stream.exeBranchInfo.isIndirect){
        preValid = false;
    }
    else{
        preValid = true;
    }
    preAddr = stream.getRealStartPC();
    preBranchAddr = stream.squashType == SQUASH_CTRL && stream.exeTaken ?
                    stream.exeBranchInfo.pc : stream.directBranchAddr;
}

DirectStream::streamEntry DirectStream::lookup(Addr addr){
    Addr index = getIndex(addr);
    Addr tag = getTag(addr);
    hit = false;
    if (table[index].tag == tag && table[index].en){
        hit = true;
        return table[index];
    }
    return streamEntry();
}

Addr DirectStream::getIndex(Addr pc){
    return ((pc >> 1) ^ (pc >> (tableWidth + 1))) & tableMask;
}

Addr DirectStream::getTag(Addr pc){
    return (pc >> (1+(int)log2(tableMask+1))) & tagMask;
}

bool DirectStream::compare(Addr addr1, Addr addr2){
    return addr1 == getBrAddr(addr2);
}

void DirectStream::clear(Addr addr){
    Addr index = getIndex(addr);
    Addr tag = getTag(addr);
    if (table[index].tag == tag && table[index].en){
        table[index].en = false;
    }
}

void DirectStream::updateType(Addr addr, int type){
    Addr index = getIndex(addr);
    Addr tag = getTag(addr);
    if (table[index].tag == tag && table[index].en){
        table[index].type = type;
    }
}

Addr DirectStream::getBrAddr(Addr pc){
        return (pc) & brAddrMask;
}



}
}
}
