#include "PagedKV.h"
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

PagedKV::PagedKV(int blockCount, int blockSize):n_(blockCount),s_(blockSize)
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
PagedKV::~PagedKV()
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

bool PagedKV::allocate(const std::string& id, int tokenCount)
{
    if(tokenCount <= 0) return 0;
    //判断是否已经存在
    Request* target_ = const_cast<Request*>(lookup(id));
    if(target_) return 0;
    //判断是否空间足够
    int available_count = 0;
    int* to_use_index = new int[n_];
    for(int i=0;i<n_;i++)
    {
        if(memory_[i].owner=="")
            to_use_index[available_count++]=i;
        if(s_ * available_count >= tokenCount)
            break;
    }
    if(s_ * available_count < tokenCount)
    {
        delete[] to_use_index;
        return 0;
    }
    //允许创建
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
    
    BlockNode* ptr=new_temp->head;
    for(int i=0;i<available_count;i++)
    {
        memory_[to_use_index[i]].owner = id;
        memory_[to_use_index[i]].used = min(s_ , tokenCount);
        if(tokenCount >= s_) tokenCount -= s_;
        else tokenCount = 0;
        BlockNode* temp_node = new BlockNode;
        temp_node->index=to_use_index[i];
        temp_node->next=nullptr;
        if(ptr)
            ptr->next=temp_node;
        else
            new_temp->head=temp_node;
        ptr=temp_node;
    }
    new_temp->tail=ptr;
    delete[] to_use_index;
    return 1;
}
bool PagedKV::append(const std::string& id, int newTokenCount)
{
    if(newTokenCount <= 0) return 0;
    //判断是否已经存在
    Request* target_ = const_cast<Request*>(lookup(id));
    if(!target_) return 0;

    int last_left = s_ - memory_[target_->tail->index].used;
    if(last_left >= newTokenCount)
    {
        target_->tokenCount += newTokenCount;
        memory_[target_->tail->index].used += newTokenCount;

        return 1;
    }
    //判断是否空间足够
    int available_count = 0;
    int* to_use_index = new int[n_];
    for(int i=0;i<n_;i++)
    {
        if(memory_[i].owner == "")
            to_use_index[available_count++] = i;
        if(last_left + s_ * available_count >= newTokenCount)
            break;
    }
    if(last_left + s_ * available_count < newTokenCount)
    {
        delete[] to_use_index;
        return 0;
    }
    target_->tokenCount += newTokenCount;
    newTokenCount -= last_left;
    BlockNode* ptr = target_->tail;
    for(int i=0;i<available_count;i++)
    {
        BlockNode* temp_node = new BlockNode;
        memory_[to_use_index[i]].owner = id;
        memory_[to_use_index[i]].used = min(s_ , newTokenCount);
        if(newTokenCount >= s_) newTokenCount -= s_;
        else newTokenCount = 0;
        temp_node->index = to_use_index[i];
        temp_node->next = ptr->next;
        ptr->next = temp_node;
        ptr = ptr->next;
    }
    target_->tail = ptr;
    delete[] to_use_index;
    return 1;
}
const Request* PagedKV::lookup(const std::string& id) const
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
bool PagedKV::free(const std::string& id)
{
    Request* target_ = const_cast<Request*>(lookup(id));
    if(!target_) return 0;
    BlockNode* ptr = target_->head;
    while(ptr)
    {
        int index = ptr->index;
        memory_[index].owner="";
        memory_[index].used=0;
        BlockNode* temp=ptr;
        ptr=ptr->next;
        delete temp; 
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
void PagedKV::displayMemory() const
{
    for(int i=0;i<n_;i++)
    {
        if(memory_[i].owner=="")
            std::cout<<"-";
        else    std::cout<<memory_[i].owner;
        if(i!=n_-1) std::cout<<" ";
    }
}
void PagedKV::displayRequest(const std::string& id) const
{
    Request* target_ = const_cast<Request*>(lookup(id));
    if(!target_)
    {
        std::cout<<"NOT FOUND";
        return ;
    }
    std::cout<<target_->tokenCount<<" "<<(target_->tokenCount-1)/s_+1<<" ";
    BlockNode* ptr = target_->head;
    while(ptr)
    {
        int index = ptr->index;
        std::cout<<index;
        ptr=ptr->next;
        if(ptr) std::cout<<" ";
    }
}