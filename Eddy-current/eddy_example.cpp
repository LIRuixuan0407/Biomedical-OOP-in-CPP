#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <iomanip>
#include <sstream>

#include "debug.h"
#include "terminal_graphics.h"

// 函数声明：计算预测梯度波形
std::vector<double> compute_predicted(const std::vector<double>& input,
                                      const std::vector<double>& amplitudes,
                                      const std::vector<double>& rate_constants);

// 主逻辑函数
void run(std::vector<std::string>& args) {
    debug::verbose = false;  // 默认关闭调试模式
    
    // 处理 -v 参数：开启调试模式
    debug::verbose = std::erase(args, "-v");  // 使用 std::erase 移除 "-v" 参数，并返回是否找到

    // 参数检查：确保至少提供参数文件和梯度文件
    if (args.size() < 3)
        throw std::runtime_error("Usage: ./test [-v] [-n num] parameter.txt gradient.txt [output.txt]");
    
    // 解析 -n 参数：设置迭代次数，默认为 10 次
    int iterations = 10;
    auto n_pos = std::find(args.begin(), args.end(), "-n");
    
    if (n_pos != args.end()) {
        if (std::next(n_pos) == args.end())
            throw std::runtime_error("Usage: -n num");
        
        iterations = std::stoi(*std::next(n_pos));  // 将 "-n" 后的值转换为整数
        args.erase(n_pos, std::next(n_pos, 2));  // 删除 "-n" 和其后的值
    }
    
    // 检查是否有输出文件路径
    std::string output_file;
    if (args.size() > 3) {
        output_file = args[3];  // 如果有第四个参数，将其视为输出文件路径
    }
    
    // 加载参数文件：读取涡流的幅度和衰减率常数
    std::vector<double> amplitudes, rate_constants;
    std::ifstream param_file(args[1]);

    if (!param_file.is_open()) {
        throw std::runtime_error("Failed to open parameter file: " + args[1]);
    }

    double A, R;
    while (param_file >> A >> R) {
        amplitudes.push_back(A);        // 存储幅度
        rate_constants.push_back(R);    // 存储衰减率
    }

    if (amplitudes.empty() || rate_constants.empty()) {
        throw std::runtime_error("No valid parameters found in the file: " + args[1]);
    }
    
    // 加载梯度文件：读取期望的梯度波形
    std::vector<double> desired;
    std::ifstream grad_file(args[2]);
    double val;
    while (grad_file >> val) desired.push_back(val);  // 逐个读取梯度值
    
    std::vector<double> input = desired;  // 初始输入波形设为期望波形
    
    // 初始状态 (iteration 0)：计算初始预测波形
    std::vector<double> predicted = compute_predicted(input, amplitudes, rate_constants);
    double max_dev = 0;  // 用于存储最大绝对偏差
    for (size_t i = 0; i < desired.size(); ++i)
        max_dev = fmax(max_dev, abs(desired[i] - predicted[i]));  // 计算最大绝对偏差
    
    // 输出初始迭代的最大绝对偏差
    std::cout << "iteration 0, maximum absolute deviation = " << max_dev << std::endl;
    
    // 使用 terminal_graphics 库显示期望波形和预测波形
    TG::plot(2000, 300)
        .add_line(desired)
        .add_line(predicted, 3);
    
    // 主循环：根据指定的迭代次数进行补偿
    for (int iter = 1; iter <= iterations; ++iter) {
        // 更新输入波形：将误差加回到输入波形中
        for (size_t i = 0; i < input.size(); ++i)
            input[i] += desired[i] - predicted[i];
        
        // 计算新的预测波形
        predicted = compute_predicted(input, amplitudes, rate_constants);
        
        // 最终迭代显示：输出最大绝对偏差并显示波形
        if (iter == iterations) {
            double max_dev = 0;
            for (size_t i = 0; i < desired.size(); ++i)
                max_dev = fmax(max_dev, abs(desired[i] - predicted[i]));
            
            std::cout << "iteration " << iteration << ", maximum absolute deviation = " << std::fixed << std::setprecision(7) << max_dev << std::endl;
            
            TG::plot(2000, 300)
                .add_line(input)
                .add_line(predicted, 3);
        } else if (debug::verbose) {
            // 调试模式：输出每次迭代的最大绝对偏差
            double max_dev = 0;
            for (size_t i = 0; i < desired.size(); ++i)
                max_dev = fmax(max_dev, abs(desired[i] - predicted[i]));
            debug::log("iteration " + std::to_string(iter) + ", maximum absolute deviation = " + std::to_string(max_dev));
        }
        
        // 如果指定了输出文件路径，则在每次迭代后写入补偿后的输入波形
        if (!output_file.empty()) {
            std::ofstream out(output_file);
            if (out) {
                for (auto v : input) {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(7) << v << endl;
                    std::string formatted = oss.str();
                    out << formatted;
                }
            } else {
                std::cerr << "Failed to open output file: " << output_file << std::endl;
            }
        }
    }
}

// 计算预测梯度波形的函数
std::vector<double> compute_predicted(const std::vector<double>& input, const std::vector<double>& amplitudes,
                                      const std::vector<double>& rate_constants) {
    std::vector<double> predicted(input.size(), 0.0);  // 存储预测的梯度波形
    std::vector<double> currents(amplitudes.size(), 0.0);  // 存储每个涡流分量的大小
    double prev_G = 0.0;  // 上一时间点的输入梯度值

    for (size_t t = 0; t < input.size(); ++t) {
        const double current_G = input[t];  // 当前时间点的输入梯度值
        const double dG = current_G - prev_G;  // 梯度的变化量
        prev_G = current_G;  // 更新上一时间点的梯度值

        // 更新每个涡流分量的大小
        for (size_t i = 0; i < amplitudes.size(); ++i) {
            currents[i] = currents[i] + dG - currents[i] * rate_constants[i];
        }

        // 计算涡流的总影响
        double eddy_effect = 0.0;
        for (size_t i = 0; i < amplitudes.size(); ++i) {
            eddy_effect += currents[i] * amplitudes[i];
        }

        // 预测的梯度 = 输入梯度 - 涡流影响
        predicted[t] = current_G - eddy_effect;
    }
    return predicted;
}

// 主函数：程序入口
int main(int argc, char* argv[]) {
    try {
        std::vector<std::string> args(argv, argv + argc);  // 将命令行参数存储为字符串向量
        run(args);  // 调用主逻辑函数
    }
    catch (std::exception& excp) {
        std::cerr << "ERROR: " << excp.what() << std::endl;  // 捕获并输出标准异常
        return 1;
    }
    catch (...) {
        std::cerr << "ERROR: unknown exception thrown - aborting\n";  // 捕获其他未知异常
        return 1;
    }

    return 0;  // 程序正常结束
}
