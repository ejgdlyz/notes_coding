#include <iostream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <mutex>

struct GameState
{
    int player_position_x;
    int player_position_y;
    // 其他游戏状态变量
};

class StateSyncServer
{
public:
    void start()
    {
        running = true;
        server_thread = std::thread(&StateSyncServer::run, this);
    }

    void stop()
    {
        running = false;
        if (server_thread.joinable())
        {
            server_thread.join();
        }
    }

    void receiveInput(int player_id, const std::string &input)
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        // 根据输入更新游戏状态
        if (input == "MoveUp")
        {
            game_states[player_id].player_position_y++;
        }
        else if (input == "MoveDown")
        {
            game_states[player_id].player_position_y--;
        }
        // 其他输入处理
    }

private:
    void run()
    {
        while (running)
        {
            auto start_time = std::chrono::steady_clock::now();

            // 广播当前的游戏状态
            broadcastGameState();

            auto end_time = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(100) - (end_time - start_time));
        }
    }

    void broadcastGameState()
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        for (const auto &state : game_states)
        {
            // 在此广播游戏状态给客户端
            std::cout << "Broadcasting state of player " << state.first << " (x: "
                      << state.second.player_position_x << ", y: "
                      << state.second.player_position_y << ")" << std::endl;
        }
    }

    bool running = false;
    std::thread server_thread;
    std::unordered_map<int, GameState> game_states;
    std::mutex state_mutex;
};

int main()
{
    StateSyncServer server;
    server.start();

    // 模拟玩家输入
    server.receiveInput(1, "MoveUp");
    server.receiveInput(2, "MoveDown");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.stop();

    return 0;
}
