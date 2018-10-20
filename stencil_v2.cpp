#include <iostream>
#include <upcxx/upcxx.hpp>

using namespace std;

const long N = 1000;
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

upcxx::global_ptr<double> block_left;
upcxx::global_ptr<double> block_right;
upcxx::global_ptr<double> ghost_block_g;
int block_size;
int block_size_ghost;

void set_block_left(upcxx::global_ptr<double> left){
    block_left = left;
}

void set_block_right(upcxx::global_ptr<double> right){
    block_right = right;
}

int main(void){
    upcxx::init();
    block_size = N / upcxx::rank_n();
    block_size_ghost = block_size + 2;

    // Initialize array
    ghost_block_g = upcxx::new_array<double>(block_size_ghost);
    double* ghost_block = ghost_block_g.local();
    double* normal_block = &ghost_block[1];
    mt19937_64 rgen(upcxx::rank_me() + 1);
    for(int i = 0; i < block_size; i++){
        normal_block[i] = rgen() % MAX_GEN + 0.5;
    }
    
    // Find left and right global pointers
    int left_index = (upcxx::rank_me() - 1 + upcxx::rank_n()) % upcxx::rank_n();
    int right_index = (upcxx::rank_me() + 1) % upcxx::rank_n();
    upcxx::rpc(left_index, set_block_right, ghost_block_g);
    upcxx::rpc(right_index, set_block_left, ghost_block_g);

    upcxx::barrier();
    for (long iteration = 0; iteration < MAX_ITER; iteration++){
        int start = iteration % 2;

        // Get ghost cells
        if(!start) {
            ghost_block[0] = upcxx::rget(block_left + block_size_ghost - 2).wait();
        } else {
            ghost_block[block_size_ghost - 1] = upcxx::rget(block_right + 1).wait();
        }

        // smooth
        for (int i = start + 1; i < block_size_ghost - 1; i += 2){
            ghost_block[i] = (ghost_block[i - 1] + ghost_block[i + 1]) / 2.0;
            if(!upcxx::rank_me() && i == start+1){
                cout << ghost_block[i] << endl;
            }
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
