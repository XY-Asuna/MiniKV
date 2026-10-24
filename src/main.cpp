#include "ContiguousKV.h"
#include "PagedKV.h"
#include "check.h"
#include <chrono>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>

// 哈希桶数量，默认 17；可用 --buckets <n> 在运行时修改。
int bucketsSize = 17;

int allocate_failure_count = 0;
int total_token_count = 0 , total_block_count = 0;
int unused_token_count = 0;

namespace
{

enum OpType
{
    OP_ALLOC,
    OP_APPEND,
    OP_FREE,
    OP_LOOKUP,
    OP_REQUEST,
    OP_MEMORY
};

struct Op
{
    OpType type;
    std::string id;
    int count;
};

// 预先把全部操作解析进内存（不计入耗时），两种模式共用同一份只读负载，保证工作量完全一致。
void parseOps(int q, Op* ops, int& parsedCount)
{
    parsedCount = 0;
    for (int i = 0; i < q; ++i)
    {
        std::string opName;
        if (!(std::cin >> opName)) return;
        Op& op = ops[parsedCount];
        if (opName == "ALLOC" || opName == "APPEND")
        {
            if (!(std::cin >> op.id >> op.count)) return;
            op.type = opName == "ALLOC" ? OP_ALLOC : OP_APPEND;
        }
        else if (opName == "FREE" || opName == "LOOKUP" || opName == "REQUEST")
        {
            if (!(std::cin >> op.id)) return;
            op.type = opName == "FREE" ? OP_FREE : opName == "LOOKUP" ? OP_LOOKUP : OP_REQUEST;
        }
        else if (opName == "MEMORY")
            op.type = OP_MEMORY;
        else
            continue; // 与原实现一致：未知操作只消耗该 token
        ++parsedCount;
    }
}

// 单次完整评测：构造、执行全部操作、析构全部计入耗时；对象完全新建，与另一模式不复用任何状态。
// silent 为 true 时屏蔽全部中间输出（性能测试模式）：操作本身照常执行，只是不产生 stdout，保证计时段内工作量不变。
double runOnce(const std::string& mode, int n, int s, int q, const Op* ops, int parsedCount, bool silent)
{
    allocate_failure_count = 0;
    unused_token_count = 0;
    total_token_count = total_block_count = 0;
    
    int MU_count = 0 , UN_count = 0 , FM_count = 0;
    double MU_sum = 0.0 , UN_sum = 0.0 , FM_sum = 0.0;
    

    std::streambuf* savedBuf = nullptr;
    std::stringbuf savedStrBuf;
    if (silent)
    {
        savedBuf = std::cout.rdbuf();
        std::cout.rdbuf(nullptr);
    }
    const auto start = std::chrono::steady_clock::now();

    MiniKV* manager;
    if (mode == "contiguous")
        manager = new ContiguousKV(n, s);
    else
        manager = new PagedKV(n, s);
    MiniKV& kv = *manager;

    for (int i = 0; i < parsedCount; ++i)
    {
        const Op& op = ops[i];
        switch (op.type)
        {
        case OP_ALLOC:
            if (!kv.allocate(op.id, op.count))
            {
                ++allocate_failure_count;
                std::cout << "FAIL" << '\n';
            }
            else
                std::cout << "OK" << '\n';
            break;
        case OP_APPEND:
            if (!kv.append(op.id, op.count))
            {
                ++allocate_failure_count;
                std::cout << "FAIL" << '\n';
            }
            else
                std::cout << "OK" << '\n';
            break;
        case OP_FREE:
            std::cout << (kv.free(op.id) ? "OK" : "FAIL") << '\n';
            break;
        case OP_LOOKUP:
        {
            const Request* request = kv.lookup(op.id);
            if (!request) std::cout << "NOT FOUND";
            else
            {
                int blocks = 0;
                for (const BlockNode* p = request->head; p; p = p->next) ++blocks;
                std::cout << "FOUND " << request->tokenCount << ' ' << blocks;
            }
            std::cout << '\n';
            break;
        }
        case OP_REQUEST:
            if(!silent)
            {
                kv.displayRequest(op.id);
                std::cout << '\n';
            }
            break;
        case OP_MEMORY:
            if(!silent)
            {
                kv.displayMemory();
                std::cout << '\n';
            }
            break;
        }
        //采样
        if(!(i % 1000))
        {
            if(total_block_count > 0)
            {
                MU_count++ , MU_sum += 100.0 * total_token_count / total_block_count / s;
                UN_count++ , UN_sum += 100.0 * unused_token_count / total_block_count / s;
            }
            if(silent && n - total_block_count > n / 20)
            {
                std::cout.rdbuf(&savedStrBuf);
                kv.displayMemory();
                std::string memoryStr = savedStrBuf.str();
                int len = memoryStr.length();
                int max_contiguous = 0 , cur_contiguous = 0;
                for(int j=0;j<len;j++)
                {
                    if(memoryStr[j] == ' ')
                        continue;
                    if(memoryStr[j] == '-')
                        cur_contiguous++;
                    else
                    {
                        max_contiguous = std::max(max_contiguous,cur_contiguous);
                        cur_contiguous = 0;
                    }
                }
                max_contiguous = std::max(max_contiguous,cur_contiguous);
                FM_count++ , FM_sum += 100.0 * (1 - 1.0 * max_contiguous / (n - total_block_count));
                std::cout.rdbuf(nullptr);
                savedStrBuf.str("");
            }
        }
    }

    delete manager;

    const auto end = std::chrono::steady_clock::now();
    if (savedBuf) std::cout.rdbuf(savedBuf);

    if(silent)
    {
        std::cerr << std::fixed << std::setprecision(3);
        std::cerr << "---------------\n";
        std::cerr << "[mode] " << mode << '\n';
        std::cerr << "n=" << n << " blockSize=" << s << " bucketsSize=" << bucketsSize
                  << " q=" << q << '\n';
        std::cerr << "[timing] " << mode << ": " << std::chrono::duration<double, std::milli>(end - start).count() << " ms" << std::endl;
        std::cerr << "allocate failure count: " << allocate_failure_count << std::endl;
        std::cerr << "unused token count: " << unused_token_count << std::endl;
        if(MU_count > 0)
            std::cerr << "average Memory Utilization(total token/total block capacity): " << MU_sum / MU_count << "\%\n";
        if(FM_count > 0)
            std::cerr << "average Fragmentation: " << FM_sum / FM_count << "\%\n";
        if(UN_count > 0)
            std::cerr << "average Unused Token Utilization(unused token/total block capacity): " << UN_sum / UN_count << "\%\n";   
        std::cerr << "---------------\n";
    }
    return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

int main(int argc, char* argv[])
{
    // --perf：性能测试模式，屏蔽全部中间输出；--turn <n>：指定测试数据文件（缺省 gen.in）；--buckets <n>：修改哈希桶数量（缺省 17）。
    bool perfMode = false;
    char* turn = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--perf")
            perfMode = true;
        else if (std::string(argv[i]) == "--buckets")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Missing value for --buckets" << std::endl;
                return 1;
            }
            ++i;
            char* end = nullptr;
            const long value = std::strtol(argv[i], &end, 10);
            if (end == argv[i] || *end != '\0' || value <= 0 || value > INT_MAX)
            {
                std::cerr << "Invalid --buckets value: " << argv[i] << std::endl;
                return 1;
            }
            bucketsSize = static_cast<int>(value);
        }
        else if(std::string(argv[i]) == "--turn")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Missing value for --turn" << std::endl;
                return 1;
            }
            turn = argv[i + 1];
            ++i;
        }
    }

    // 依次评测 contiguous 与 paged 两种模式。
    std::string turn_str = "../data/gen";
    turn_str += turn ? std::string(turn) : "";
    turn_str += ".in";
    if (!freopen(turn_str.c_str(), "r", stdin))
    {
        std::cerr << "Cannot open " << turn_str << std::endl;
        return 1;
    }

    int n, s, q;
    if (!(std::cin >> n >> s >> q)) return 0;
    Op* ops = new Op[q > 0 ? q : 1];
    int parsedCount = 0;
    parseOps(q, ops, parsedCount);

    const double contiguousMs = runOnce("contiguous", n, s, q, ops, parsedCount, perfMode);
    const double pagedMs = runOnce("paged", n, s, q, ops, parsedCount, perfMode);

    delete[] ops;
    return 0;
}
