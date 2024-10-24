#include "tbit.hh"
namespace gem5 {

namespace branch_prediction {

namespace ftb_pred{

TBIT::TBIT(int size) : size(size) { entrys.resize(size); }

bool TBIT::prevent(Addr pc) {
    uint32_t idx = getIdx(pc);
    uint32_t tag = getTag(pc);
    hitEntry = entrys[idx];
    bool hit = entrys[idx].tag == tag;
    return hit && entrys[idx].at == 1;
}

void TBIT::update(const FetchStream& stream) {
    Addr addr = stream.getRealStartPC();
    bool taken = stream.getTaken();
    uint32_t idx = getIdx(addr);
    uint32_t tag = getTag(addr);
    TBITEntry& entry = entrys[idx];
    bool hit = entry.tag == tag;
    if (taken) {
        if(hit){
            if(entry.at != 1){
                entry.tc++;
                if(entry.tc == 3){
                    entry.at = 1;
                    skipUpdate = true;
                }
                else{
                    skipUpdate = false;
                }
            }
            else{
                if(entry.tc < 3){
                    entry.tc++;
                    skipUpdate = true;
                }
            }
        }
        else{
            entry.at = 0;
            entry.tc = 0;
            entry.tag = tag;
            entry.target = stream.getTakenTarget();
            entry.end = stream.getEndPC();
            skipUpdate = false;
        }
    }
    else{
        if(hit){
            if(entry.at == 1){
                if(entry.tc == 3){
                    entry.tc = 0;
                    skipUpdate = false;
                }
                else{
                    entry.tag = 0;
                    skipUpdate = false;
                }
            }
            else{
                entry.tag = 0;
                skipUpdate = false;
            }
        }
        else{
            skipUpdate = false;
        }
    }
}

bool TBIT::isSkip(){
    return skipUpdate;
}

uint32_t TBIT::getIdx(Addr pc) { return (pc >> 2) & 0x7f; }

uint32_t TBIT::getTag(Addr pc) {
    return ((pc >> 10) & 0xff) ^ ((pc >> 20) & 0xff);
}

int TBIT::getEnd(){
    return hitEntry.end;
}

Addr TBIT::getTarget(){
    return hitEntry.target;
}

}
}
}