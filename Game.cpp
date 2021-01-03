#include "Game.hpp"

/*--------------------------------------------------------------------------------
								
--------------------------------------------------------------------------------*/

const vector<tile_record> Game::tile_hist() const { // Returns the tile timing histogram
    return m_tile_hist;
}

uint Game::thread_num() const {//Returns the effective number of running threads = min(thread_num, field_height)
    return m_thread_num;
}

const vector<tile_record> Game::histogram() const { // Returns the tile timing histogram
    return tile_finish_time_hist;
}

const vector<double> Game::gen_hist() const { // Returns the generation timing histogram
    return m_gen_hist;
}

void Game::run() {

    _init_game(); // Starts the threads and all other variables you need
    print_board("Initial Board");
    tp = std::chrono::system_clock::now();
    for (uint i = 0; i < m_gen_num; ++i) {
        //num_of_generations++;
        auto gen_start = std::chrono::system_clock::now();
        _step(i); // Iterates a single generation
        auto gen_end = std::chrono::system_clock::now();
        m_gen_hist.push_back(
                (double) std::chrono::duration_cast<std::chrono::microseconds>(gen_end - gen_start).count());
        //   printf("printing gen board\n");
        print_board(nullptr);

    } // generation loop
    // printf("printing final board\n");
    print_board("Final Board");
    // printf("destroying the game\n");
    _destroy_game();
    //printf("finished destroying\n");
}

// Create game fields - Consider using utils:read_file, utils::split
// Create & Start threads
// Testing of your implementation will presume all threads are started here
void Game::_init_game() {
    vector<string> lines = utils::read_lines(filename);
    matrix_num_rows = lines.size();
    matrix_num_cols = (utils::split(lines.at(0), ' ')).size();
    curr_matrix = bool_mat(matrix_num_rows, vector<bool>(matrix_num_cols));
    step_matrix = bool_mat(matrix_num_rows, vector<bool>(matrix_num_cols));
    m_thread_num = (num_of_threads < matrix_num_rows) ? num_of_threads : matrix_num_rows;
    tile_finish_time_hist = vector<tile_record>(m_gen_num * m_thread_num);
    for (int i = 0; i < matrix_num_rows; i++) {
        vector<string> row = utils::split(lines.at(i), ' ');
        stringToBool(i, row);
    }

    for (unsigned int i = 0; i < m_thread_num; ++i) {
        Thread *thrd = new PoolThread(i, *this);
        m_threadpool.push_back(thrd);
    }
    for (unsigned int i = 0; i < m_thread_num; ++i) {
        m_threadpool.at(i)->start();
    }
}

// Push jobs to queue
// Wait for the workers to finish calculating
// Swap pointers between current and next field
// NOTE: Threads must not be started here - doing so will lead to a heavy penalty in your grade
void Game::_step(uint curr_gen) {
    if (m_thread_num == 0) return;
    pthread_mutex_lock(&global_lock);
    int row_per_thread = (int) (matrix_num_rows / m_thread_num);
    int i;
    for (i = 0; (unsigned) i < (m_thread_num - 1); i++) {
        task tk = task(row_per_thread * i, (row_per_thread * (i + 1)) - 1);
        TasksQueue->push(tk);
    }
    task tk = task(row_per_thread * i, matrix_num_rows - 1);
    TasksQueue->push(tk);
    pthread_cond_wait(&(threadsFinished), &global_lock);
    num_of_finished_threads = 0;
    swap(curr_matrix, step_matrix);
    pthread_mutex_unlock(&global_lock);

}

// Destroys board and frees all threads and resources
// Not implemented in the Game's destructor for testing purposes.
// All threads must be joined here
void Game::_destroy_game() {
    while ((unsigned) this->TasksQueue->getWritersWaiting() < m_thread_num);
    pthread_mutex_lock(&global_lock);
    // printf("start looking here\n");
    for (unsigned int i = 0; i < (m_thread_num); i++) {
        task tk = task(-1, -1);
        TasksQueue->push(tk);
    }
    pthread_cond_wait(&(threadsFinished), &global_lock);
    pthread_mutex_unlock(&global_lock);
    for (uint i = 0; i < m_thread_num; ++i) {
        m_threadpool[i]->join();
    }
    //flag = true;
}

/*--------------------------------------------------------------------------------

--------------------------------------------------------------------------------*/
inline void Game::print_board(const char *header) {

    if (print_on) {

        // Clear the screen, to create a running animation
        if (interactive_on)
            system("clear");

        // Print small header if needed
        if (header != nullptr)
            cout << "<------------" << header << "------------>" << endl;

        // TODO: Print the board
        cout << u8"╔" << string(u8"═") * matrix_num_cols << u8"╗" << endl;
        for (uint i = 0; i < (unsigned) matrix_num_rows; ++i) {
            cout << u8"║";
            for (uint j = 0; j < (unsigned) matrix_num_cols; ++j) {
                if (curr_matrix.at(i).at(j) > 0)
                    cout << colors[curr_matrix.at(i).at(j) % 7] << u8"█" << RESET;
                else
                    cout << u8"░";
               
            }
            cout << u8"║" << endl;
        }
        cout << u8"╚" << string(u8"═") * matrix_num_cols << u8"╝" << endl;
        // Display for GEN_SLEEP_USEC micro-seconds on screen
        if (interactive_on)
            usleep(GEN_SLEEP_USEC);
    }

}


/* Function sketch to use for printing the board. You will need to decide its placement and how exactly
	to bring in the field's parameters.

		cout << u8"╔" << string(u8"═") * field_width << u8"╗" << endl;
		for (uint i = 0; i < field_height ++i) {
			cout << u8"║";
			for (uint j = 0; j < field_width; ++j) {
				cout << (field[i][j] ? u8"█" : u8"░");
			}
			cout << u8"║" << endl;
		}
		cout << u8"╚" << string(u8"═") * field_width << u8"╝" << endl;
*/ 



