#include <upcxx/upcxx.hpp>

#include "bucket_sort_includes.hpp"

using namespace std;

const long N = 1000*32;
#define BLOCK_SIZE (N/upcxx::rank_n())
const long SORT_MIN = numeric_limits<long>::lowest();
const long SORT_MAX = numeric_limits<long>::max();

#ifdef DEBUG_
#define DEBUG_PRINT(x) (cout<<(x)<<endl)
#else 
#define DEBUG_PRINT(x) 
#endif

#define RANK_STR "Rank("<<upcxx::rank_me()<<"):"


upcxx::global_ptr<long> section;
void init_array();
long find_bucket(long);
void clean_up();
void allocate_recv_buckets();
void sort_recv_buckets();
upcxx::global_ptr<long> get_recv_bucket(long sending_rank, long send_amount);

vector<long> sorted;
bool check_arrays();

int main(void){
    upcxx::init();
    init_array();      
    
    // Create Buckets
    DEBUG_PRINT("Create Buckets");
    vector<vector<long>> buckets(upcxx::rank_n());

    // Sort local buckets
    DEBUG_PRINT("Sort local Buckets");
    for(long i = 0; i < BLOCK_SIZE; i++){
        long correct_bucket = find_bucket(section.local()[i]);
        //cout << i << " " << section.local()[i] << " correct_bucket=" << correct_bucket << endl;
        buckets[correct_bucket].push_back(section.local()[i]);
    }

    // Allocate receive buckets
    DEBUG_PRINT("Allocate Receive Buckets");
    allocate_recv_buckets();
    upcxx::barrier();

    // Send buckets to correct rank
    DEBUG_PRINT("Send Buckets to correct rank");
    upcxx::future<> future = upcxx::make_future();
    for(long i = 0; i < upcxx::rank_n(); i++){
        upcxx::global_ptr<long> recv_bucket = upcxx::rpc(i, get_recv_bucket, upcxx::rank_me(), buckets[i].size()).wait();
        future = when_all(future, rput(&buckets[i][0], recv_bucket, buckets[i].size()));
    }
    future.wait();
    upcxx::barrier();

    // Sort received buckets
    DEBUG_PRINT("Sort received Buckets");
    sort_recv_buckets();
    upcxx::barrier();

    // Verify order
    DEBUG_PRINT("Verify Order");
    bool correct = check_arrays();
    correct = upcxx::reduce_one(correct, upcxx::op_fast_bit_and, 0).wait();
    if(upcxx::rank_me() == 0){
        if(correct){
            cout << "Sort completed correctly" << endl;
        } else {
            cout << "Sort failed" << endl;
        }
    }

    // Cleanup
    DEBUG_PRINT("Clean Up");
    clean_up();
    upcxx::finalize();
}

void init_array(){
    section = upcxx::new_array<long>(BLOCK_SIZE);
    default_random_engine generator;
    uniform_int_distribution<long> distribution(SORT_MIN, SORT_MAX);
    for(long i = 0; i < BLOCK_SIZE; i++){
        section.local()[i] = distribution(generator);
    }
}

vector<upcxx::global_ptr<long>> recv_buckets;
vector<long> recv_buckets_lengths;
void allocate_recv_buckets(){
    recv_buckets.resize(upcxx::rank_n());
    recv_buckets_lengths.resize(upcxx::rank_n());
}

void deallocate_recv_buckets(){
    for(auto& bucket : recv_buckets){
        upcxx::delete_array(bucket);
    }
}

upcxx::global_ptr<long> get_recv_bucket(long sending_rank, long send_amount){
    recv_buckets[sending_rank] = upcxx::new_array<long>(send_amount);
    recv_buckets_lengths[sending_rank] = send_amount;
    return recv_buckets[sending_rank];
}

void sort_recv_buckets(){
    for(long i = 0; i < upcxx::rank_n(); i++){
        for(long j = 0; j < recv_buckets_lengths[i]; j++){
            sorted.push_back(recv_buckets[i].local()[j]);
        }
    }
    sort(sorted.begin(), sorted.end());
}

long get_max_sorted(){
    return sorted.back();
}
long get_min_sorted(){
    return sorted.front();
}
bool check_arrays(){
    if(upcxx::rank_me() > 1){
        long prev_max = upcxx::rpc(upcxx::rank_me() - 1, get_max_sorted).wait();
        if(prev_max >= get_min_sorted()){
            cout << RANK_STR << "Sort Failed, " << prev_max << ">=" << get_min_sorted() << endl;
            return false;
        }
    }
    if(upcxx::rank_me() < upcxx::rank_n() - 1){
        long next_min = upcxx::rpc(upcxx::rank_me() + 1, get_min_sorted).wait();
        if(next_min <= get_max_sorted()){
            cout << RANK_STR << "Sort Failed, " << next_min << "<=" << get_max_sorted() << endl;
            return false;
        }
    }
    return true;
}

void clean_up(){
    deallocate_recv_buckets();
}

long find_bucket(long val){
    long step = (SORT_MAX / upcxx::rank_n() - SORT_MIN / upcxx::rank_n());
    for(long i = 0; i < upcxx::rank_n(); i++){
        if((val >= SORT_MIN + (i * step)) && (val < SORT_MAX + ((i+1) * step))){
            return i;
        }
    }
    return upcxx::rank_n() - 1;
}
