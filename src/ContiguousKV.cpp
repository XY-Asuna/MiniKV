#include "ContiguousKV.h"
namespace
{
    const int bucketsSize = 17;
    int fnv1a(const std::string& s)
    {
        long long H=2166136261;
        for(char c : s)
            H^=c,H*=16777619,H%=bucketsSize;
        H = ((H % bucketsSize) + bucketsSize) % bucketsSize;
        return H;
    }
    int min(const int x,const int y)
    {
        return x<y?x:y;
    }
}
ContiguousKV::ContiguousKV(int blockCount, int blockSize):n_(blockCount),s_(blockSize)
{
    memory_ = new Block[blockCount];
    for(int i=0;i<blockCount;i++)
    {
        memory_[i].owner="";
        memory_[i].used=0;
    }
    buckets_ = new Request*[bucketsSize];
    for(int i=0;i<bucketsSize;i++)
    {
        buckets_[i]=new Request; // 空头节点
        buckets_[i]->hashNext=nullptr;
        buckets_[i]->head=buckets_[i]->tail=nullptr;
        buckets_[i]->id="";
        buckets_[i]->tokenCount=-1;
    }
}
ContiguousKV::~ContiguousKV()
{
    delete[] memory_;
    for(int i=0;i<bucketsSize;i++)
    {
        Request* p=buckets_[i];
        while(p)
        {
            BlockNode* ptr=p->head;
            while(ptr)
            {
                BlockNode* temp=ptr;
                ptr=ptr->next;
                delete temp;
            }
            Request* temp=p;
            p=p->hashNext;
            delete temp;
        }
    }
    delete[] buckets_;
}

bool ContiguousKV::allocate(const std::string& id, int tokenCount) 
//allocate 在 ID 已存在、tokenCount <= 0 或无法获得所需空间时返回 false
{
    if(tokenCount <= 0) return 0;
    //判断是否已经存在
    Request* target_ = const_cast<Request*>(lookup(id));
    if(target_) return 0;
    //寻找连续空间
    int start = 0, block_count=0;
    while(start < n_)
    {
        if(memory_[start].owner=="")
            block_count++;
        else    block_count=0;
        if(s_*block_count >= tokenCount)
        {
            start -= block_count-1;
            break;
        }
        start++;
    }
    if(start >= n_) return 0;
    //允许分配 
    
    int buckets_id = fnv1a(id);
    Request* p = buckets_[buckets_id];
    while(p->hashNext)
        p=p->hashNext;
    Request* new_temp = new Request;
    new_temp->hashNext=p->hashNext;
    p->hashNext = new_temp;
    new_temp->id = id;
    new_temp->tokenCount = tokenCount;
    new_temp->head = nullptr;
    //new_temp->tail = nullptr;
    BlockNode* ptr=new_temp->head;
    for(int i=start;i<start+block_count;i++)
    {
        memory_[i].used = min(tokenCount,s_);
        memory_[i].owner=id;
        if(tokenCount >= s_) tokenCount-=s_;
        BlockNode* temp_node = new BlockNode;
        temp_node->index=i;
        temp_node->next=nullptr;
        if(ptr)
            ptr->next=temp_node;
        else
            new_temp->head=temp_node;
        ptr=temp_node;
    }
    new_temp->tail=ptr;

    return 1;
}
bool ContiguousKV::append(const std::string& id, int newTokenCount) 
//append 在 ID 不存在、newTokenCount <= 0 或空间不足时返回 false。
{
    if(newTokenCount<=0) return 0;
    Request* target_ = const_cast<Request*>(lookup(id));
    if(!target_) return 0;
    //判断空间是否足够
    int start = target_->tail->index;
    int last_left = s_ - memory_[start].used;
    if(newTokenCount <= last_left)
    {
        memory_[start].used += newTokenCount;
        target_->tokenCount += newTokenCount;
        
        return 1;
    }
    
    int block_count = 0;
    for(int i=start+1;i<n_;i++)
    {
        if(memory_[i].owner=="")
            block_count++;
        else
            return 0;
        if(last_left + s_ * block_count >= newTokenCount)
            break;
    }
    if(last_left + s_ * block_count < newTokenCount) return 0;
    //补齐
    memory_[start].used += last_left;
    target_->tokenCount += newTokenCount;
    newTokenCount -= last_left;
    BlockNode* ptr = target_->tail;
    for(int i=start+1;i<start+1+block_count;i++)
    {
        memory_[i].owner = id;
        memory_[i].used = min(newTokenCount , s_);
        if(newTokenCount>=s_) newTokenCount-=s_;
        BlockNode* temp_node = new BlockNode;
        temp_node->index=i;
        temp_node->next=nullptr;
        ptr->next=temp_node;
        ptr=temp_node;
    }
    target_->tail = ptr;
    return 1;
}
const Request* ContiguousKV::lookup(const std::string& id) const 
{
    int buckets_id = fnv1a(id);
    Request* p = buckets_[buckets_id]->hashNext;
    while(p)
    {
        if(p->id == id) break;
        p=p->hashNext;
    }
    return p;
}
bool ContiguousKV::free(const std::string& id) 
//free 在 ID 不存在时返回 false。成功释放的 ID 可重新使用。
{
    Request* target_ = const_cast<Request*>(lookup(id));
    if(!target_) return 0;
    int start = target_->head->index , end = target_->tail->index;
    BlockNode* ptr = target_->head;
    for(int i=start;i<=end;i++)
    {
        memory_[i].owner="";
        memory_[i].used=0;
        BlockNode* temp = ptr->next;
        delete ptr;
        ptr = temp;
    }
    int buckets_id = fnv1a(id);
    Request* p = buckets_[buckets_id];
    while(p->hashNext)
    {
        if(p->hashNext->id == id) break;
        p=p->hashNext;
    }
    Request* delete_temp = p->hashNext;
    p->hashNext = p->hashNext->hashNext;
    delete delete_temp;
    return 1;
}
void ContiguousKV::displayMemory() const 
{
    for(int i=0;i<n_;i++)
    {
        if(memory_[i].owner=="")
            std::cout<<"-";
        else    std::cout<<memory_[i].owner;
        if(i!=n_-1) std::cout<<" ";
    }
}
void ContiguousKV::displayRequest(const std::string& id) const 
{
    Request* target_ = const_cast<Request*>(lookup(id));
    if(!target_)
    {
        std::cout<<"NOT FOUND";
        return ;
    }
    int start = target_->head->index , end = target_->tail->index;
    std::cout<<target_->tokenCount<<" "<<end - start + 1<<" ";
    for(int i=start;i<=end;i++)
    {
        std::cout<<i;
        if(i!=end) std::cout<<" ";
    }
        
}