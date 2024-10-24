#ifndef __CPU_PRED_FTB_DIRECTSTREAM_HH__
#define __CPU_PRED_FTB_DIRECTSTREAM_HH__
#include "arch/generic/pcstate.hh"
#include "base/logging.hh"
#include "base/types.hh"
#include "cpu/pred/ftb/stream_struct.hh"
#include "cpu/pred/ftb/timed_base_pred.hh"

namespace gem5
{

namespace branch_prediction
{

namespace ftb_pred
{

using DynInstPtr = o3::DynInstPtr;

class DirectStream
{
public:
    DirectStream(int numEntries=128, int tagWidth=7, int ageWidth=2, int timeWidth=3, int addrWidth=5);
    void putPCHistory(Addr startAddr, const boost::dynamic_bitset<> &history,
                      std::vector<FullFTBPrediction> &stagePreds) ;
    void update(const FetchStream &stream);
    bool compare(Addr addr1, Addr addr2);
    void clear(Addr addr);
    void updateType(Addr addr, int type);
private:
    struct streamEntry
    {
        bool en;
        Addr tag;
        Addr brAddr;
        int times;
        int type;
        int age;
    };
    struct DirectStreamMeta
    {
        bool hit;
    };
    DirectStreamMeta meta;
    std::vector<streamEntry> table;
    int tableMask;
    int tagMask;
    int timeMask;
    int ageMask;
    int brAddrMask;
        int tableWidth;
    bool hit;
    int directTimes;
    Addr startAddr;
    Addr startBranchAddr;
    bool startCheck;
    Addr preAddr;
    Addr preBranchAddr;
    bool preValid;
    bool preSquash;

public:
    streamEntry lookup(Addr addr);
    Addr getIndex(Addr pc);
    Addr getTag(Addr pc);
        Addr getBrAddr(Addr pc);
};

}
}
}

#endif
