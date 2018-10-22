#include <iostream>
#include <upcxx/upcxx.hpp>

using namespace std;

const long N = 1000;
const long MAX_ITER = N * N * 2;
const long MAX_ITER = N * N * 2;
const int MAX_GEN = 100;
const int EXPECTED_VAL = MAX_GEN / 2;
const double EPSILON = 0.1;

bool check_done(double* local, int n_local, long iteration){
    double max_err = 0;
    for (int i = 0; i < n_local; i++){
        max_err = max(fabs(EXPECTED_VAL - local[i]), max_err);
    }
    max_err = upcxx::reduce_all(max_err, upcxx::op_fast_max).wait();
    if (max_err / EXPECTED_VAL <= EPSILON){
        if(!upcxx::rank_me()){
            cout << "Converged at " << iteration << ", err " << max_err << endl;
        }
        return true;
    }
    return false;
}

upcxx::global_ptr<double> ghost_block_odd_g;
upcxx::global_ptr<double> ghost_block_even_g;
int block_size;
int block_size_ghost;

upcxx::global_ptr<double> block_left_odd;
upcxx::global_ptr<double> block_right_odd;
upcxx::global_ptr<double> block_left_even;
upcxx::global_ptr<double> block_right_even;

void set_block_left(upcxx::global_ptr<double> left_odd, upcxx::global_ptr<double> left_even){
    block_left_odd = left_odd;
    block_left_even = left_even;
}

void set_block_right(upcxx::global_ptr<double> right_odd, upcxx::global_ptr<double> right_even){
    block_right_odd = right_odd;
    block_right_even = right_even;
}

int main(void){
    upcxx::init();
    block_size = N / upcxx::rank_n() / 2;
    block_size_ghost = block_size + 2;

    // Initialize array
    ghost_block_odd_g = upcxx::new_array<double>(block_size_ghost);
    ghost_block_even_g = upcxx::new_array<double>(block_size_ghost);
    double* ghost_block = ghost_block_even_g.local();
    double* normal_block = &ghost_block[1];
    mt19937_64 rgen(upcxx::rank_me() + 1);
    for(int i = 0; i < block_size; i++){
        normal_block[i] = rgen() % MAX_GEN + 0.5;
        rgen();
    }
    
    // Find left and right global pointers
    int left_index = (upcxx::rank_me() - 1 + upcxx::rank_n()) % upcxx::rank_n();
    int right_index = (upcxx::rank_me() + 1) % upcxx::rank_n();
    upcxx::rpc(left_index, set_block_right, ghost_block_odd_g, ghost_block_even_g).wait();
    upcxx::rpc(right_index, set_block_left, ghost_block_odd_g, ghost_block_even_g).wait();

    upcxx::barrier();
    for (long iteration = 0; iteration < MAX_ITER; iteration++){
        int odd = iteration % 2;
        auto block_left  = odd ? block_left_odd : block_left_even;
        auto block_right = odd ? block_right_odd : block_right_even;
        auto ghost_block_in   = odd ? ghost_block_odd_g.local() : ghost_block_even_g.local();
        auto ghost_block_out  = odd ? ghost_block_even_g.local() : ghost_block_odd_g.local();
        normal_block = &ghost_block_out[1];

        // Get ghost cells
        ghost_block_in[0] = upcxx::rget(block_left + block_size_ghost - 2).wait();
        ghost_block_in[block_size_ghost - 1] = upcxx::rget(block_right + 1).wait();

        // smooth
        for (int i = 1; i < block_size_ghost - 1; i++){
            if(!upcxx::rank_me() && i == 1){
                cout << ghost_block_out[i] << endl;
            }
            ghost_block_out[i] = (ghost_block_in[i] + ghost_block_in[i + 1]) / 2.0;
        }
        
        // Check for end
        upcxx::barrier();
        if (check_done(normal_block, block_size, iteration)){
            upcxx::finalize();
            return 0;
        }
    }
    
    cout << "Reached Max Iterations" << endl;
    upcxx::finalize();
    return 0;
}



// Bucket sort
