#ifndef __CPU_PRED_FTB_TBIT_HH__
#define __CPU_PRED_FTB_TBIT_HH__
#include "base/types.hh"
#include "cpu/inst_seq.hh"
#include <stdint.h>
#include <vector>
#include "cpu/pred/ftb/stream_struct.hh"
namespace gem5
{

namespace branch_prediction
{

namespace ftb_pred
{
class TBIT {
public:
    struct TBITEntry {
        int tag;
        uint8_t tc;
        uint8_t at;
        Addr target;
        int end;
    };
    TBIT(int size = 128);
    bool prevent(Addr pc);
    void update(const FetchStream &entry);
    bool isSkip();
    int getEnd();
    Addr getTarget();

private:
    uint32_t getIdx(Addr pc);
    uint32_t getTag(Addr pc);

private:
    std::vector<TBITEntry> entrys;
    int size;
    bool skipUpdate;
    TBITEntry hitEntry;
};
}
}
}
#endif