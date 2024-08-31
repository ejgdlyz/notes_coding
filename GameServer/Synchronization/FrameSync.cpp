#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <queue>

class FrameSyncServer
{
public:
    FrameSyncServer(int fps) : fps(fps), frame_duration(1000 / fps), current_frame(0) {}

    void start()
    {
        running = true;
        server_thread = std::thread(&FrameSyncServer::run, this);
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
        std::lock_guard<std::mutex> lock(input_mutex);
        input_queue[player_id].push(input);
    }

private:
    void run()
    {
        while (running)
        {
            auto start_time = std::chrono::steady_clock::now();

            // Process inputs
            std::lock_guard<std::mutex> lock(input_mutex);
            for (const auto &player : input_queue)
            {
                // 将每个玩家的输入应用到游戏状态
                std::cout << "Processing input for player " << player.first << " on frame " << current_frame << std::endl;
                while (!input_queue[player.first].empty())
                {
                    std::string input = input_queue[player.first].front();
                    input_queue[player.first].pop();
                    // 处理输入
                }
            }

            // 更新游戏状态
            updateGameState();

            // 广播游戏状态
            broadcastGameState();

            current_frame++;
            auto end_time = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(frame_duration) - (end_time - start_time));
        }
    }

    void updateGameState()
    {
        // 在此更新游戏逻辑
        std::cout << "Updating game state for frame " << current_frame << std::endl;
    }

    void broadcastGameState()
    {
        // 在此广播游戏状态给所有客户端
        std::cout << "Broadcasting game state for frame " << current_frame << std::endl;
    }

    int fps;
    int frame_duration;
    int current_frame;
    bool running = false;
    std::thread server_thread;
    std::unordered_map<int, std::queue<std::string>> input_queue;
    std::mutex input_mutex;
};

int main()
{
    FrameSyncServer server(60); // 60 FPS
    server.start();

    // 模拟玩家输入
    server.receiveInput(1, "MoveUp");
    server.receiveInput(2, "MoveDown");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.stop();

    return 0;
}
