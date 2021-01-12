#ifndef __GAMERUN_H
#define __GAMERUN_H

#include "Headers.hpp"
#include "PCQueue.hpp"
#include "utils.hpp"
#include "Thread.hpp"
#include <cmath>
#include <string> 

/*--------------------------------------------------------------------------------
								  Species colors                                     ///NEW
--------------------------------------------------------------------------------*/
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black - 7 */
#define RED     "\033[31m"      /* Red - 1*/
#define GREEN   "\033[32m"      /* Green - 2*/
#define YELLOW  "\033[33m"      /* Yellow - 3*/
#define BLUE    "\033[34m"      /* Blue - 4*/
#define MAGENTA "\033[35m"      /* Magenta - 5*/
#define CYAN    "\033[36m"      /* Cyan - 6*/


/*--------------------------------------------------------------------------------
								  Auxiliary Structures
--------------------------------------------------------------------------------*/
struct game_params {
    // All here are derived from ARGV, the program's input parameters.
    uint n_gen;
    uint n_thread;
    string filename;
    bool interactive_on;
    bool print_on;
};

struct tile_record {
    double tile_compute_time; // Compute time for the tile
    uint thread_id; // The thread responsible for the compute

};

/*--------------------------------------------------------------------------------
									Added Class
--------------------------------------------------------------------------------*/
class task {
    int row1, row2;
public:
    task(int row1, int row2) {
        this->row1 = row1;
        this->row2 = row2;
    }

    task() {
        this->row1 = 0;
        this->row2 = 0;
    }

    int getRow1() {
        return this->row1;
    }

    int getRow2() {
        return this->row2;
    }
};

/*--------------------------------------------------------------------------------
									Class Declaration
--------------------------------------------------------------------------------*/
class Game {
public:
    friend class PoolThread;

    Game(game_params param) : num_of_threads(param.n_thread), filename(param.filename), num_of_generations(0),
                              m_gen_num(param.n_gen),
                              m_thread_num(0),
                              interactive_on(param.interactive_on), print_on(param.print_on) {
        TasksQueue = new PCQueue<task>;
        pthread_cond_init(&threadsFinished, NULL);
        pthread_mutexattr_init(&error_check_mutex);
        pthread_mutexattr_settype(&error_check_mutex, PTHREAD_MUTEX_ERRORCHECK);
        pthread_mutex_init(&global_lock, &error_check_mutex);
        matrix_num_rows = 0;
        matrix_num_cols = 0;
        num_of_finished_threads = 0;
    }

    ~Game() {
        delete TasksQueue;
        TasksQueue = NULL;
        for (int i = 0; (unsigned) i < m_thread_num; i++) {
            m_threadpool.pop_back();
        }
        pthread_cond_destroy(&threadsFinished);
        pthread_mutexattr_destroy(&error_check_mutex);
        pthread_mutex_destroy(&global_lock);

    }

/*--------------------------------------------------------------------------------
									Variables Added
--------------------------------------------------------------------------------*/
    int phase = 1;
    int num_of_finished_threads, matrix_num_cols, matrix_num_rows, num_of_threads;
    string filename;
    bool_mat step_matrix, curr_matrix;
    int num_of_generations;
    PCQueue<task> *TasksQueue;
    pthread_cond_t threadsFinished;
    pthread_mutex_t global_lock;
    pthread_mutexattr_t error_check_mutex;
    vector<tile_record> tile_finish_time_hist;
    std::chrono::time_point<std::chrono::system_clock> tp;


/*--------------------------------------------------------------------------------
									     END
--------------------------------------------------------------------------------*/
    void run(); // Runs the game
    const vector<tile_record> histogram() const;

    const vector<double> gen_hist() const; // Returns the generation timing histogram
    const vector<double> tile_hist() const;// Returns the tile timing histogram
    uint thread_num() const;//Returns the effective number of running threads = min(thread_num, field_height)


protected: // All members here are protected, instead of private for testing purposes

    // See Game.cpp for details on these three functions
    void _init_game();

    void _step(uint curr_gen);

    void _destroy_game();

    void swap(bool_mat &m1, bool_mat &m2) {
        bool_mat temp = m2;
        m1 = temp;
       // m2 = temp;
    }

    void stringToBool(int line, vector<string> row) {
        for (unsigned int i = 0; i < row.size(); i++) {
            /*if (row[i] == "0") {
                curr_matrix.at(line).at(i) = false;
                step_matrix.at(line).at(i) = false;
            } else if (row[i] == "1") {
                curr_matrix.at(line).at(i) = true;
                step_matrix.at(line).at(i) = true;
            }*/
            curr_matrix.at(line).at(i) = stoi(row[i]);
            step_matrix.at(line).at(i) = stoi(row[i]);
        }
    }

    inline void print_board(const char *header);

    uint m_gen_num;                    // The number of generations to run
    uint m_thread_num;                    // Effective number of threads = min(thread_num, field_height)
    vector<double> m_tile_hist;    // Shared Timing history for tiles: First m_thread_num cells are the calculation durations for tiles in generation 1 and so on.
    // Note: In your implementation, all m_thread_num threads must write to this structure.
    vector<double> m_gen_hist;        // Timing history for generations: x=m_gen_hist[t] iff generation t was calculated in x microseconds
    vector<Thread *> m_threadpool;       // A storage container for your threads. This acts as the threadpool.

    bool interactive_on; // Controls interactive mode - that means, prints the board as an animation instead of a simple dump to STDOUT
    bool print_on; // Allows the printing of the board. Turn this off when you are checking performance (Dry 3, last question)

    // TODO: Add in your variables and synchronization primitives

};

/*--------------------------------------------------------------------------------
									Class PoolThreads
--------------------------------------------------------------------------------*/
class PoolThread : public Thread {
private:
    Game &gameOfLife;
    unsigned int thread_id;
   
    // should we add static something cause update cells is a helper function
    void updateCells(int row, int col) {
        
        int live_cells = 0;
        int counter = 0;
        int arr[8] = {0};  //for getting the dominant specie
        bool is_curr_live = (gameOfLife.curr_matrix.at(row).at(col)> 0);
        for (int r = -1; r < 2; r++) {
            for (int c = -1; c < 2; c++) {
                if (row + r >= 0 && row + r < gameOfLife.matrix_num_rows && col + c >= 0 &&
                    col + c < gameOfLife.matrix_num_cols) {
                    if (gameOfLife.curr_matrix.at(row + r).at(col + c)>0) {
                        live_cells++;
                        arr[((gameOfLife.curr_matrix.at(row + r).at(col + c)) )]+=1;
                        counter+=gameOfLife.curr_matrix.at(row + r).at(col + c);
                        
                    }
                }
            }
        }
        
       
           //PHASE 2 HERE.
        if(gameOfLife.phase > 1 ){  //NEW: Doing Phase 2 of the current generation
           if(is_curr_live){  //NEW: This checks if the cell is alive, if so the cell changes its specie.
             //live_cells+=is_curr_live;
             gameOfLife.step_matrix.at(row).at(col) = round(((counter))/(float)live_cells);
        
            }
            
            return;
        }
        if(gameOfLife.phase==1){
         int dominant_specie=gameOfLife.curr_matrix.at(row).at(col), sum_species=0;
        for(int i=1;i<=7;i++){
            //if(arr[i]>0) counter++;
            int sum = arr[i]*i;
           
            if(sum==sum_species){
                dominant_specie=(i<dominant_specie) ? i : dominant_specie;
                
            }else if(sum>sum_species){
                dominant_specie=i;
                sum_species=arr[i]*i;
            }
        
        }
        
     
        live_cells -= (is_curr_live) ? 1: 0;
       
        //PHASE 1 HERE ONLY
        if (!is_curr_live && live_cells == 3) { 
        
            gameOfLife.step_matrix.at(row).at(col) = dominant_specie; //NEW: becomes the dominant specie.
        } else if (!(is_curr_live && (live_cells == 2 || live_cells == 3))) {
            gameOfLife.step_matrix.at(row).at(col) = 0;
        } /*else {
            gameOfLife.step_matrix.at(row).at(col) = 0;
        }*/
        }
       
    }

public:
    PoolThread(uint n_thread_id, Game &game) : Thread(n_thread_id), gameOfLife(game) {
        thread_id = n_thread_id;
    }


    ~PoolThread() {
    }

    void thread_workload() override {
        while (1) {

            task t = gameOfLife.TasksQueue->pop();
            pthread_mutex_lock(&(gameOfLife.global_lock));
           // tile_record tile_2;
            if (t.getRow1() == -1 && t.getRow2() == -1) {
                gameOfLife.num_of_finished_threads++;
                if ((unsigned) gameOfLife.num_of_finished_threads == gameOfLife.m_thread_num) {
                  //  printf("thread finished1\n");
                    pthread_cond_signal(&(gameOfLife.threadsFinished));
                }
                pthread_mutex_unlock(&(gameOfLife.global_lock));
                break;
            }
            pthread_mutex_unlock(&(gameOfLife.global_lock));

            pthread_mutex_lock(&(gameOfLife.global_lock));
            auto gen_start = std::chrono::system_clock::now();
             //     auto duration_in_seconds = std::chrono::duration_cast<std::chrono::microseconds>(
       //             std::chrono::system_clock::now().time_since_epoch()).count();
          //  double tile_start_time = (double) std::chrono::duration_cast<std::chrono::microseconds>(
          //           gen_start - gameOfLife.tp).count();
            pthread_mutex_unlock(&(gameOfLife.global_lock));

            int first = t.getRow1(), last = t.getRow2();

            for (int i = first; i <= last; i++) {
                for (int j = 0; j < gameOfLife.matrix_num_cols; j++) {
                    updateCells(i, j);
                }
            }

            pthread_mutex_lock(&(gameOfLife.global_lock));
            auto gen_end = std::chrono::system_clock::now();
            //            duration_in_seconds = std::chrono::duration_cast<std::chrono::microseconds>(
//                    std::chrono::system_clock::now().time_since_epoch()).count();
           // double tile_end_time = (double) std::chrono::duration_cast<std::chrono::microseconds>(
           //         gen_end - gameOfLife.tp).count();
            pthread_mutex_unlock(&(gameOfLife.global_lock));


            pthread_mutex_lock(&(gameOfLife.global_lock));
            tile_record tile;


            tile.tile_compute_time = (double) std::chrono::duration_cast<std::chrono::microseconds>(
                    gen_end - gen_start).count();
            gameOfLife.m_tile_hist.push_back(tile.tile_compute_time);//NEW DOUBLE
            tile.thread_id = this->thread_id;

            gameOfLife.num_of_finished_threads++;
            if ((unsigned) gameOfLife.num_of_finished_threads == gameOfLife.m_thread_num) {
                //printf("thread finished\n");
                //printf("num_of_finished_threads is %d\n",(gameOfLife.num_of_finished_threads));
                pthread_cond_signal(&(gameOfLife.threadsFinished));
            }
            // int indx =  gameOfLife.m_tile_hist.size();
            pthread_mutex_unlock(&(gameOfLife.global_lock));
            /*gameOfLife.tile_finish_time_hist.at(indx-1).thread_id = this->thread_id;
            gameOfLife.tile_finish_time_hist.at(indx-1).tile_end_time = tile_end_time;
            gameOfLife.tile_finish_time_hist.at(indx-1).tile_start_time = tile_start_time;*/

        }
    }

};

#endif
