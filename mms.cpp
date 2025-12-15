#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <chrono>
#include <algorithm> 
#include <iomanip> 
#include "API.h"

const int MAZE_SIZE = 16;
const std::string MOUSE_NAME = "Smart_Explorer";
const std::string MAZE_NAME = "example1";
const std::string OUTPUT_FILE = "mission_log.txt";

enum RobotState {
    EXPLORING,  
    FINAL_RUN,  
    DONE
};

RobotState currentState = EXPLORING;

int x = 0;
int y = 0;
int d = 0; 

bool walls[MAZE_SIZE][MAZE_SIZE][4] = {false};
int costs[MAZE_SIZE][MAZE_SIZE];
bool visited_cells[MAZE_SIZE][MAZE_SIZE] = {false}; 

struct Coord {
    int x;
    int y;
    bool visited; 
};

std::vector<Coord> HIDDEN_FLAGS = {
    {7, 7, false}, 
    {0, 7, false}, 
    {9, 15, false}
};

auto start_time = std::chrono::high_resolution_clock::now();

void log(const std::string& text) {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = now - start_time;
    
    std::cerr << "[" << std::fixed << std::setprecision(3) << diff.count() << "s] " << text << std::endl;
}

void update_walls() {
    visited_cells[x][y] = true;
    
    if (API::wallFront()) {
        walls[x][y][d] = true;
        if (d == 0 && y < 15) walls[x][y+1][2] = true;
        if (d == 1 && x < 15) walls[x+1][y][3] = true;
        if (d == 2 && y > 0)  walls[x][y-1][0] = true;
        if (d == 3 && x > 0)  walls[x-1][y][1] = true;
    }
    if (API::wallRight()) {
        int d_right = (d + 1) % 4;
        walls[x][y][d_right] = true;
        if (d_right == 0 && y < 15) walls[x][y+1][2] = true;
        if (d_right == 1 && x < 15) walls[x+1][y][3] = true;
        if (d_right == 2 && y > 0)  walls[x][y-1][0] = true;
        if (d_right == 3 && x > 0)  walls[x-1][y][1] = true;
    }
    if (API::wallLeft()) {
        int d_left = (d + 3) % 4;
        walls[x][y][d_left] = true;
        if (d_left == 0 && y < 15) walls[x][y+1][2] = true;
        if (d_left == 1 && x < 15) walls[x+1][y][3] = true;
        if (d_left == 2 && y > 0)  walls[x][y-1][0] = true;
        if (d_left == 3 && x > 0)  walls[x-1][y][1] = true;
    }
}

void flood_fill_exploration() {
    for (int i = 0; i < MAZE_SIZE; ++i) {
        for (int j = 0; j < MAZE_SIZE; ++j) {
            costs[i][j] = 999;
        }
    }

    std::queue<Coord> q;

    for (int i = 0; i < MAZE_SIZE; ++i) {
        for (int j = 0; j < MAZE_SIZE; ++j) {
            if (!visited_cells[i][j]) {
                costs[i][j] = 0;
                q.push({i, j, false});
            }
        }
    }

    int dx[] = {0, 1, 0, -1};
    int dy[] = {1, 0, -1, 0};

    while (!q.empty()) {
        Coord current = q.front();
        q.pop();

        int current_cost = costs[current.x][current.y];

        for (int dir = 0; dir < 4; ++dir) {
            int nx = current.x + dx[dir];
            int ny = current.y + dy[dir];

            if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
                if (!walls[current.x][current.y][dir]) {
                    if (costs[nx][ny] > current_cost + 1) {
                        costs[nx][ny] = current_cost + 1;
                        q.push({nx, ny, false});
                    }
                }
            }
        }
    }
}

void flood_fill_to_target(int tx, int ty) {
    for (int i = 0; i < MAZE_SIZE; ++i) {
        for (int j = 0; j < MAZE_SIZE; ++j) {
            costs[i][j] = 999;
        }
    }
    std::queue<Coord> q;
    costs[tx][ty] = 0;
    q.push({tx, ty, false});

    int dx[] = {0, 1, 0, -1};
    int dy[] = {1, 0, -1, 0};

    while (!q.empty()) {
        Coord current = q.front();
        q.pop();
        int current_cost = costs[current.x][current.y];

        for (int dir = 0; dir < 4; ++dir) {
            int nx = current.x + dx[dir];
            int ny = current.y + dy[dir];

            if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
                if (!walls[current.x][current.y][dir]) {
                    if (costs[nx][ny] > current_cost + 1) {
                        costs[nx][ny] = current_cost + 1;
                        q.push({nx, ny, false});
                    }
                }
            }
        }
    }
}

void move_robot() {
    int min_val = 999;
    int best_dir = -1;
    int dx[] = {0, 1, 0, -1};
    int dy[] = {1, 0, -1, 0};

    for (int dir = 0; dir < 4; ++dir) {
        int nx = x + dx[dir];
        int ny = y + dy[dir];
        if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
            if (!walls[x][y][dir]) {
                if (costs[nx][ny] < min_val) {
                    min_val = costs[nx][ny];
                    best_dir = dir;
                }
            }
        }
    }

    if (best_dir == -1) return;

    if (best_dir == d) {
    } else if (best_dir == (d + 1) % 4) {
        API::turnRight();
        d = (d + 1) % 4;
    } else if (best_dir == (d + 3) % 4) {
        API::turnLeft();
        d = (d + 3) % 4;
    } else {
        API::turnRight();
        API::turnRight();
        d = (d + 2) % 4;
    }

    API::moveForward();
    if (d == 0) y += 1;
    if (d == 1) x += 1;
    if (d == 2) y -= 1;
    if (d == 3) x -= 1;
}

Coord calculate_final_destination() {
    int sum_x = 0;
    int sum_y = 0;
    log("--------------------------------");
    log("ALL FLAGS FOUND! CALCULATING...");
    for(const auto& p : HIDDEN_FLAGS) {
        sum_x += p.x;
        sum_y += p.y;
    }
    int final_x = sum_x % 12;
    int final_y = sum_y % 12;

    log("Sum X: " + std::to_string(sum_x) + " | Sum Y: " + std::to_string(sum_y));
    log("Target: (" + std::to_string(final_x) + ", " + std::to_string(final_y) + ")");
    log("--------------------------------");
    return {final_x, final_y, false};
}

int main() {
    start_time = std::chrono::high_resolution_clock::now();
    
    log("Mission: SMART EXPLORER");
    API::setColor(0, 0, 'G');
    API::setText(0, 0, "START");

    for (const auto& p : HIDDEN_FLAGS) {
        API::setColor(p.x, p.y, 'G'); 
    }
    
    visited_cells[0][0] = true;

    Coord final_target = {0, 0, false};

    while (true) {
        update_walls();
        
        if (currentState == EXPLORING) {
            flood_fill_exploration();
            if (costs[x][y] == 999) {
                 log("Maze fully explored. Mission Failed (Flags not found).");
                 break;
            }
        }
        else if (currentState == FINAL_RUN) {
            flood_fill_to_target(final_target.x, final_target.y);
        }

        API::setText(x, y, std::to_string(costs[x][y]));

        int visited_count = 0;
        for (auto& p : HIDDEN_FLAGS) {
            if (!p.visited && x == p.x && y == p.y) {
                p.visited = true;
                log("FOUND FLAG: (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                API::setColor(x, y, 'B'); // Turn Blue
                API::setText(x, y, "FOUND");
            }
            if (p.visited) visited_count++;
        }

        if (currentState == EXPLORING && visited_count == HIDDEN_FLAGS.size()) {
            final_target = calculate_final_destination();
            
            API::setColor(final_target.x, final_target.y, 'R');
            API::setText(final_target.x, final_target.y, "FINAL");
            
            currentState = FINAL_RUN;
            continue; 
        }

        if (currentState == FINAL_RUN && costs[x][y] == 0) {
            log("FINAL STOP REACHED: (" + std::to_string(x) + ", " + std::to_string(y) + ")");
            log("MISSION COMPLETE.");
            API::setColor(x, y, 'R');
            API::setText(x, y, "WIN");
            currentState = DONE;
            break;
        }

        move_robot();
    }
}