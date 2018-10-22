#include <iostream>
#include <upcxx/upcxx.hpp>

using namespace std;

const long N = 1000;

bool check_array();

int main(void){
    upcxx::init();

    

    upcxx::finalize();
}


upcxx::global_ptr<long>* arrays = nullptr;
int* lengths = nullptr;
bool check_array(long my_length, long* my_chunk){
    if(!arrays){
        arrays = (long*)malloc(sizeof(upcxx::global_ptr) * upcxx::rank_n());
    }
    if(!lengths){
        if(!upcxx::rank_me()){
            lengths = (long*)malloc(sizeof(int) * upcxx::rank_n());
        } else {
            lengths = (long*)(1);
            upcxx::rpc(0, 
        }
    }
    if(!upcxx::rank_me()){
        
            
}


