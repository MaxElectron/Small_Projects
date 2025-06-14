#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Config {
    constexpr int MAZE_WIDTH = 29;
    constexpr int MAZE_HEIGHT = 19;

    enum class SearchAlgorithm { GreedyDFS, BestFirst, StochasticHillClimb };
    constexpr SearchAlgorithm ACTIVE_ALGORITHM = SearchAlgorithm::GreedyDFS;

    constexpr int DEEP_JUMP_DEPTH = 2; 

    constexpr double STOCHASTIC_ACCEPT_WORSE_PROBABILITY = 0.02;

    constexpr bool RANDOMIZE_CELL_ORDER = true;
    constexpr unsigned int RANDOM_SEED = 42;

    constexpr bool SHOW_PROGRESS_BAR = true;
}

namespace PathConfig {
    const std::filesystem::path OUTPUT_DIRECTORY = "maze_outputs";
    const std::filesystem::path ARCHIVE_DIRECTORY = OUTPUT_DIRECTORY / "archive";
    const std::filesystem::path LATEST_BEST_FILENAME = OUTPUT_DIRECTORY / "best_record.txt";
    const std::filesystem::path RECORDS_LOG_FILENAME = ARCHIVE_DIRECTORY / "records_log.txt";
    const std::string ARCHIVE_FILENAME_PREFIX = "maze_record_";
}

struct Point {
    int coordinate_x, coordinate_y;
    bool operator==(const Point& other) const { return coordinate_x == other.coordinate_x && coordinate_y == other.coordinate_y; }
    Point operator+(const Point& other) const { return {coordinate_x + other.coordinate_x, coordinate_y + other.coordinate_y}; }
};

struct Direction {
    Point delta;
    int priority;
    bool operator==(const Direction& other) const { return delta == other.delta; }
};

namespace Directions {
    constexpr Direction LEFT  = {{-1, 0}, 1};
    constexpr Direction UP    = {{0, -1}, 2};
    constexpr Direction RIGHT = {{1, 0}, 3};
    constexpr Direction DOWN  = {{0, 1}, 4};
    constexpr std::array<Direction, 4> ALL = {UP, DOWN, LEFT, RIGHT};
}

class Maze {
public:
    Maze() : wall_grid(Config::MAZE_HEIGHT, std::vector<bool>(Config::MAZE_WIDTH, false)) {}

    [[nodiscard]] bool is_within_bounds(const Point& point) const {
        return point.coordinate_y >= 0 && point.coordinate_y < Config::MAZE_HEIGHT 
            && point.coordinate_x >= 0 && point.coordinate_x < Config::MAZE_WIDTH;
    }

    [[nodiscard]] bool is_wall_at(const Point& point) const { return wall_grid[point.coordinate_y][point.coordinate_x]; }
    void set_wall_at(const Point& point, bool has_wall) { wall_grid[point.coordinate_y][point.coordinate_x] = has_wall; }

    [[nodiscard]] bool has_path_to_finish() const {
        if (is_wall_at(START_POINT) || is_wall_at(FINISH_POINT)) {
            return false;
        }

        std::queue<Point> q; 
        q.push(START_POINT);
        std::vector<std::vector<bool>> visited(Config::MAZE_HEIGHT, std::vector<bool>(Config::MAZE_WIDTH, false));
        visited[START_POINT.coordinate_y][START_POINT.coordinate_x] = true;

        while (!q.empty()) {
            Point current_point = q.front(); 
            q.pop(); 

            if (current_point == FINISH_POINT) {
                return true;
            }
            for (const auto& direction : Directions::ALL) {
                Point next_point = current_point + direction.delta;
                if (is_within_bounds(next_point) && !visited[next_point.coordinate_y][next_point.coordinate_x] && !is_wall_at(next_point)) {
                    visited[next_point.coordinate_y][next_point.coordinate_x] = true; 
                    q.push(next_point);
                }
            }
        }
        return false;
    }

    void save_to_file(const std::filesystem::path& path) const {
        std::ofstream os(path); 
        if (!os.is_open()) {
            return;
        }

        for (int r = 0; r < Config::MAZE_HEIGHT; ++r) {
            for (int c = 0; c < Config::MAZE_WIDTH; ++c) {
                Point p = {c, r}; 
                char s = (p == START_POINT) ? 'S' : (p == FINISH_POINT) ? 'E' : (is_wall_at(p) ? '#' : '.');
                os << s << ' ';
            } 
            os << '\n';
        }
    }

    [[nodiscard]] std::string to_string_representation() const {
        std::string repr; 
        repr.reserve(Config::MAZE_WIDTH * Config::MAZE_HEIGHT);
        for (int r = 0; r < Config::MAZE_HEIGHT; ++r) {
            for (int c = 0; c < Config::MAZE_WIDTH; ++c) {
                repr += (is_wall_at({c, r}) ? '1' : '0');
            }
        }
        return repr;
    }
private:
    std::vector<std::vector<bool>> wall_grid;
    static constexpr Point START_POINT = {0, 0};
    static constexpr Point FINISH_POINT = {Config::MAZE_WIDTH - 1, Config::MAZE_HEIGHT - 1};
};

class BugSimulator {
public:
    [[nodiscard]] static long long calculate_score(const Maze& maze) {
        if (!maze.has_path_to_finish()) {
            return -1;
        }

        std::vector<std::vector<long long>> visit_counts(Config::MAZE_HEIGHT, std::vector<long long>(Config::MAZE_WIDTH, 0));
        for (int r = 0; r < Config::MAZE_HEIGHT; ++r) {
            for (int c = 0; c < Config::MAZE_WIDTH; ++c) {
                if (maze.is_wall_at({c, r})) {
                    visit_counts[r][c] = -1;
                }
            }
        }

        Point pos = {0, 0}; 
        visit_counts[pos.coordinate_y][pos.coordinate_x] = 1; 
        long long steps = 0; 
        Direction last_dir = Directions::DOWN;

        while (pos.coordinate_x != Config::MAZE_WIDTH - 1 || pos.coordinate_y != Config::MAZE_HEIGHT - 1) {
            if (steps++ > (long long)Config::MAZE_WIDTH * Config::MAZE_HEIGHT * 1000) {
                return -2;
            }

            long long min_visits = -1; 
            std::vector<Direction> best_dirs;
            for (const auto& dir : Directions::ALL) {
                Point next_pos = pos + dir.delta; 
                if (!maze.is_within_bounds(next_pos) || visit_counts[next_pos.coordinate_y][next_pos.coordinate_x] == -1) {
                    continue;
                }

                long long neighbor_visits = visit_counts[next_pos.coordinate_y][next_pos.coordinate_x];
                if (min_visits == -1 || neighbor_visits < min_visits) {
                    min_visits = neighbor_visits; 
                    best_dirs = {dir};
                } else if (neighbor_visits == min_visits) {
                    best_dirs.push_back(dir);
                }
            }

            if (best_dirs.empty()) {
                return -1;
            }
            
            Direction chosen_direction;
            if (std::any_of(best_dirs.begin(), best_dirs.end(), [&](const auto& d) { return d == last_dir; })) {
                chosen_direction = last_dir;
            } else {
                chosen_direction = *std::max_element(best_dirs.begin(), best_dirs.end(), [](const auto& a, const auto& b) { return a.priority < b.priority; });
            }
            
            last_dir = chosen_direction; 
            pos = pos + chosen_direction.delta; 
            visit_counts[pos.coordinate_y][pos.coordinate_x]++;
        }
        return steps;
    }
};

class SolverBase {
public:
    virtual ~SolverBase() = default;
    virtual void solve() = 0;

protected:
    using SolverState = std::pair<Maze, long long>;

    SolverBase() : records_log_stream_(PathConfig::RECORDS_LOG_FILENAME) {
        setup_output_directories();
        Maze initial_maze;
        long long initial_score = get_score_for_maze(initial_maze);
        highest_known_score_ = initial_score;
        notify_new_record(initial_maze, initial_score);
    }

    long long get_score_for_maze(const Maze& maze) {
        std::string repr = maze.to_string_representation();
        auto cache_iterator = score_cache_.find(repr);
        if (cache_iterator != score_cache_.end()) {
            return cache_iterator->second;
        }

        long long score = BugSimulator::calculate_score(maze);
        score_cache_[repr] = score;
        return score;
    }

    void notify_new_record(const Maze& maze, long long score) {
        clear_status_line();
        std::cout << "Record found: " << score << std::endl;
        records_log_stream_ << "Record: " << score << std::endl;
        std::filesystem::path ap = PathConfig::ARCHIVE_DIRECTORY / (PathConfig::ARCHIVE_FILENAME_PREFIX + std::to_string(score) + ".txt");
        maze.save_to_file(ap); 
        maze.save_to_file(PathConfig::LATEST_BEST_FILENAME);
    }
    
    void clear_status_line() const {
        std::cout << "\r" << std::string(80, ' ') << "\r" << std::flush;
    }
    
    long long highest_known_score_;
    std::unordered_map<std::string, long long> score_cache_;
    std::unordered_set<std::string> globally_visited_layouts_;
    std::ofstream records_log_stream_;

private:
    void setup_output_directories() {
        std::filesystem::create_directories(PathConfig::ARCHIVE_DIRECTORY);
    }
};

class GreedyDfsSolver : public SolverBase {
public:
    void solve() override {
        states_to_explore_stack_.push({Maze(), highest_known_score_});
        globally_visited_layouts_.insert(Maze().to_string_representation());

        while (!states_to_explore_stack_.empty()) {
            SolverState current_state = states_to_explore_stack_.top();
            states_to_explore_stack_.pop();
            display_current_status(current_state.second);

            bool improvement_found = find_and_push_potential_next_states(current_state, 1);
            if (!improvement_found && Config::DEEP_JUMP_DEPTH > 1) {
                clear_status_line();
                std::cout << "Dead end found. Attempting deep jump..." << std::endl;
                find_and_push_potential_next_states(current_state, Config::DEEP_JUMP_DEPTH);
            }
        }
        clear_status_line();
    }

private:
    void display_progress_bar(int current, int total) const {
        if (!Config::SHOW_PROGRESS_BAR || total <= 0) {
            return;
        }

        constexpr int BAR_WIDTH = 40;
        float progress = static_cast<float>(current) / total;
        int pos = static_cast<int>(BAR_WIDTH * progress);

        std::cout << "\rEvaluating: [";
        for (int i = 0; i < BAR_WIDTH; ++i) {
            std::cout << (i < pos ? "#" : " ");
        }
        std::cout << "] " << std::fixed << std::setprecision(1) << progress * 100.0 << "%" << std::flush;
    }

    void display_current_status(long long score) const {
        std::cout << "\rCurrent best: " << highest_known_score_ << " | Processing state with score: " << score << std::string(10, ' ') << std::flush;
    }
    
    std::vector<Maze> generate_unique_candidates(const Maze& initial_maze, int jump_depth) {
        std::vector<Maze> candidates;
        std::queue<std::pair<Maze, int>> bfs_queue;
        bfs_queue.push({initial_maze, 0});
        std::unordered_set<std::string> local_visited;
        local_visited.insert(initial_maze.to_string_representation());

        while(!bfs_queue.empty()) {
            auto [current_maze, depth] = bfs_queue.front(); 
            bfs_queue.pop();
            if (depth >= jump_depth) {
                continue;
            }

            for (int y = 0; y < Config::MAZE_HEIGHT; ++y) {
                for (int x = 0; x < Config::MAZE_WIDTH; ++x) {
                    Point p = {x, y};
                    if ((x == 0 && y == 0) || (x == Config::MAZE_WIDTH - 1 && y == Config::MAZE_HEIGHT - 1) || current_maze.is_wall_at(p)) {
                        continue;
                    }

                    Maze next_maze = current_maze; 
                    next_maze.set_wall_at(p, true);
                    std::string repr = next_maze.to_string_representation();

                    if (local_visited.count(repr)) {
                        continue;
                    }
                    if (globally_visited_layouts_.find(repr) == globally_visited_layouts_.end()) {
                        candidates.push_back(next_maze);
                    }
                    
                    bfs_queue.push({next_maze, depth + 1});
                    local_visited.insert(std::move(repr));
                }
            }
        }
        return candidates;
    }

    bool evaluate_and_process_candidates(const std::vector<Maze>& candidates, const SolverState& state, bool is_deep) {
        if (is_deep) { 
            clear_status_line(); 
            std::cout << "Evaluating " << candidates.size() << " new states..." << std::endl; 
        }

        std::vector<SolverState> improvements;
        for (size_t i = 0; i < candidates.size(); ++i) {
            if (is_deep) {
                display_progress_bar(i + 1, candidates.size());
            }

            long long score = get_score_for_maze(candidates[i]);
            if (score > state.second) {
                improvements.push_back({candidates[i], score});
            }
        }

        if (is_deep) { 
            clear_status_line(); 
            if (improvements.empty()) {
                std::cout << "Jump unsuccessful: returning." << std::endl;
            } else {
                std::cout << "Jump successful: found " << improvements.size() << " improvements." << std::endl;
            }
        }

        if (!improvements.empty()) {
            std::sort(improvements.begin(), improvements.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
            for (const auto& s : improvements) {
                if (s.second > highest_known_score_) { 
                    highest_known_score_ = s.second; 
                    notify_new_record(s.first, s.second); 
                }
                states_to_explore_stack_.push(s);
                globally_visited_layouts_.insert(s.first.to_string_representation());
            }
            return true;
        }
        return false;
    }

    bool find_and_push_potential_next_states(const SolverState& state, int depth) {
        std::vector<Maze> candidates = generate_unique_candidates(state.first, depth);
        return !candidates.empty() && evaluate_and_process_candidates(candidates, state, depth > 1);
    }
    
    std::stack<SolverState> states_to_explore_stack_;
};

class BestFirstSolver : public SolverBase {
private:
    struct StateComparator {
        bool operator()(const SolverState& a, const SolverState& b) const { return a.second < b.second; }
    };
    char spinner_chars_[4] = {'|', '/', '-', '\\'};
    int spinner_index_ = 0;

public:
    void solve() override {
        std::priority_queue<SolverState, std::vector<SolverState>, StateComparator> pq;
        Maze initial_maze;
        pq.push({initial_maze, highest_known_score_});
        globally_visited_layouts_.insert(initial_maze.to_string_representation());
        
        std::mt19937 rng(Config::RANDOM_SEED);

        while (!pq.empty()) {
            SolverState current_state = pq.top(); 
            pq.pop();
            display_current_status(current_state.second);

            std::vector<Point> placeable_cells;
            for (int y = 0; y < Config::MAZE_HEIGHT; ++y) {
                for (int x = 0; x < Config::MAZE_WIDTH; ++x) {
                    Point p = {x, y};
                    if (!((x == 0 && y == 0) || (x == Config::MAZE_WIDTH - 1 && y == Config::MAZE_HEIGHT - 1) || current_state.first.is_wall_at(p))) {
                        placeable_cells.push_back(p);
                    }
                }
            }

            if (Config::RANDOMIZE_CELL_ORDER) {
                std::shuffle(placeable_cells.begin(), placeable_cells.end(), rng);
            }

            for (const auto& p : placeable_cells) {
                Maze next_maze = current_state.first; 
                next_maze.set_wall_at(p, true);
                std::string repr = next_maze.to_string_representation();
                if (globally_visited_layouts_.count(repr)) {
                    continue;
                }

                long long score = get_score_for_maze(next_maze);
                if (score > highest_known_score_) { 
                    highest_known_score_ = score; 
                    notify_new_record(next_maze, score); 
                }
                pq.push({next_maze, score});
                globally_visited_layouts_.insert(repr);
            }
        }
        clear_status_line();
    }

private:
     void display_current_status(long long score) {
        spinner_index_ = (spinner_index_ + 1) % 4;
        std::cout << "\r[" << spinner_chars_[spinner_index_] << "] Current best: " << highest_known_score_ << " | Processing state with score: " << score << std::string(10, ' ') << std::flush;
    }
};

class StochasticHillClimber : public SolverBase {
private:
    char spinner_chars_[4] = {'|', '/', '-', '\\'};
    int spinner_index_ = 0;

public:
    void solve() override {
        std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<> dist(0.0, 1.0);
        Maze current_maze;
        long long run_counter = 0;

        while (true) {
            display_current_status(++run_counter);

            std::vector<Point> placeable_cells;
            for (int y = 0; y < Config::MAZE_HEIGHT; ++y) {
                for (int x = 0; x < Config::MAZE_WIDTH; ++x) {
                    Point p = {x, y};
                    if (!((x == 0 && y == 0) || (x == Config::MAZE_WIDTH - 1 && y == Config::MAZE_HEIGHT - 1) || current_maze.is_wall_at(p))) {
                        placeable_cells.push_back(p);
                    }
                }
            }

            if (placeable_cells.empty()) {
                current_maze = Maze();
                continue;
            }
            
            if (Config::RANDOMIZE_CELL_ORDER) {
                std::shuffle(placeable_cells.begin(), placeable_cells.end(), rng);
            }

            if (dist(rng) < Config::STOCHASTIC_ACCEPT_WORSE_PROBABILITY) {
                std::uniform_int_distribution<size_t> random_cell_dist(0, placeable_cells.size() - 1);
                current_maze.set_wall_at(placeable_cells[random_cell_dist(rng)], true);
            } else {
                SolverState best_next_state = {current_maze, -1};
                for (const auto& p : placeable_cells) {
                    Maze next_maze = current_maze; 
                    next_maze.set_wall_at(p, true);
                    long long score = get_score_for_maze(next_maze);
                    if (score > best_next_state.second) {
                        best_next_state = {next_maze, score};
                    }
                }

                if (best_next_state.second == -1) {
                    current_maze = Maze();
                    continue;
                }
                current_maze = best_next_state.first;
            }

            long long current_score = get_score_for_maze(current_maze);
            if (current_score > highest_known_score_) { 
                highest_known_score_ = current_score; 
                notify_new_record(current_maze, current_score); 
            }
        }
        clear_status_line();
    }
private:
    void display_current_status(long long run) {
        spinner_index_ = (spinner_index_ + 1) % 4;
        std::cout << "\r[" << spinner_chars_[spinner_index_] << "] Current best: " << highest_known_score_ << " | Run: " << run << std::string(10, ' ') << std::flush;
    }
};

int main() {
    std::unique_ptr<SolverBase> solver;

    switch (Config::ACTIVE_ALGORITHM) {
        case Config::SearchAlgorithm::GreedyDFS:
            solver = std::make_unique<GreedyDfsSolver>();
            break;
        case Config::SearchAlgorithm::BestFirst:
            solver = std::make_unique<BestFirstSolver>();
            break;
        case Config::SearchAlgorithm::StochasticHillClimb:
            solver = std::make_unique<StochasticHillClimber>();
            break;
    }

    try {
        if (solver) {
            solver->solve();
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}